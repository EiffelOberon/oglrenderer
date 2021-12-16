#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "cloud.h"
#include "deviceconstants.h"
#include "devicestructs.h"
#include "nishita.h"

layout(location = 1) in vec2 uv;

layout(location = 3) uniform vec3 color;
layout(std430, binding = CAMERA_PARAMS) uniform CameraParamsUniform
{
	CameraParams camParams;
};
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std430, binding = SKY_PARAMS) uniform SkyParamsUniform
{
	SkyParams skyParams;
};
layout(std430, binding = FBM_PARAMS) uniform FBMParamsUniform
{
	NoiseParams fbmParams;
};
layout(std430, binding = PERLIN_PARAMS) uniform PerlinParamsUniform
{
	NoiseParams perlinParams;
};
layout(std430, binding = WORLEY_PARAMS) uniform WorleyParamsUniform
{
	NoiseParams worleyParams;
};
layout (binding = CLOUD_TEXTURE) uniform sampler3D cloudTexture;
layout (binding = ENVIRONMENT_TEXTURE) uniform samplerCube environmentTexture;

layout(location = 0) out vec4 c;


#define CLOUD_COVERAGE .8

// Hash functions by Dave_Hoskins
float hash12(vec2 p)
{
	uvec2 q = uvec2(ivec2(p)) * uvec2(1597334673U, 3812015801U);
	uint n = (q.x ^ q.y) * 1597334673U;
	return float(n) * (1.0 / float(0xffffffffU));
}

vec2 hash22(vec2 p)
{
	uvec2 q = uvec2(ivec2(p))*uvec2(1597334673U, 3812015801U);
	q = (q.x ^ q.y) * uvec2(1597334673U, 3812015801U);
	return vec2(q) * (1.0 / float(0xffffffffU));
}


struct Box
{ 
    vec3 mMin;
    vec3 mMax;
}; 

struct Ray
{
    vec3 mOrigin;
    vec3 mDir;
};

bool intersect(
    const in Box b,
    const in Ray r,
    out float minDist,
    out float maxDist) 
{ 
    minDist = -1.0f;
    maxDist = -1.0f;
    float tmin = (b.mMin.x - r.mOrigin.x) / r.mDir.x; 
    float tmax = (b.mMax.x - r.mOrigin.x) / r.mDir.x; 
    if (tmin > tmax) 
    {
        float temp = tmin;
        tmin = tmax;
        tmax = temp; 
    }
    float tymin = (b.mMin.y - r.mOrigin.y) / r.mDir.y; 
    float tymax = (b.mMax.y - r.mOrigin.y) / r.mDir.y; 
    if (tymin > tymax) 
    {
        float temp = tymin;
        tymin = tymax;
        tymax = temp; 
    }
    if ((tmin > tymax) || (tymin > tmax)) 
    {
        return false; 
    }
    if (tymin > tmin)
    {
        tmin = tymin; 
    }
    if (tymax < tmax) 
    {
        tmax = tymax; 
    }
 
    float tzmin = (b.mMin.z - r.mOrigin.z) / r.mDir.z; 
    float tzmax = (b.mMax.z - r.mOrigin.z) / r.mDir.z; 
 
    if (tzmin > tzmax) 
    {
        float temp = tzmin;
        tzmin = tzmax;
        tzmax = temp; 
    }
    if ((tmin > tzmax) || (tzmin > tmax)) 
    {
        return false; 
    }
    if (tzmin > tmin) 
    {
        tmin = tzmin; 
    }
    if (tzmax < tmax) 
    {
        tmax = tzmax; 
    }
 
    minDist = tmin;
    maxDist = tmax;
    return true; 
} 

void main()
{	
    // create ray
    vec2 d = vec2((uv.x * 2.0f - 1.0f) * renderParams.mSettings.y, uv.y * 2.0f - 1.0f);
	vec3 V = camParams.mUp.xyz;
	vec3 W = normalize((camParams.mTarget - camParams.mEye).xyz);
	vec3 U = normalize(cross(W, V));
    V = normalize(cross(U, W));
    vec3 rayDir = normalize(d.x*U + d.y*V + W);
    vec3 sunDir = length(skyParams.mSunDir.xyz) > 0 ? normalize(skyParams.mSunDir.xyz) : vec3(0, 1, 0);

    vec3 sky = texture(environmentTexture, rayDir.xyz).xyz;

    Box b; 
    float width = 30000.0f;
    float height = renderParams.mCloudSettings.w;
    b.mMin = vec3(0.0f, 2000.0f, 0.0f) + vec3(-width, 0, -width);
    b.mMax = vec3(0.0f, 2000.0f, 0.0f) + vec3(width, height, width);
    //float width = 2.0f;
    //float height = renderParams.mCloudSettings.w;
    //b.mMin = vec3(0.0f, 0.0f, -2.0f) + vec3(-width, 0, -width);
    //b.mMax = vec3(0.0f, 0.0f, -2.0f) + vec3(width, 2, width);

    Ray r;
    r.mOrigin = camParams.mEye.xyz;
    r.mDir = rayDir;

    float tMin = 0.0f;
    float tMax = 0.0f;
    const bool foundIntersection = intersect(b, r, tMin, tMax);

    const float stepLength = ((tMax - tMin) / float(renderParams.mSteps.x));
    r.mOrigin = r.mOrigin + r.mDir * tMin;

    vec3 sunPos = skyParams.mSunDir.xyz * 800000.0f;

    bool hasClouds = false;
    vec4 cloudColor = vec4(1.0f);
    float transmittance = 1.0f;
    float densityScale = renderParams.mCloudSettings.z * 0.001f;
    if(foundIntersection && tMin > 0 && tMax > tMin)
    {
        for(int i = 0; i < renderParams.mSteps.x; ++i)
        {   
            vec3 uvw = (r.mOrigin - b.mMin) / (b.mMax - b.mMin);
            uvw.xz *= renderParams.mCloudMapping.xy;

            vec4 noise = texture(cloudTexture, uvw);
            const float coverage = hash12(uvw.xz) * 0.1 + (CLOUD_COVERAGE * .5 + .5);
            float lowFreqFBM = noise.y * 0.625f + noise.z * 0.25f + noise.w * 0.125f;
            float base = remap(noise.x, 1.0f - coverage, 1.0f, 0.0f, 1.0f) * coverage;
            base = remap(base, (lowFreqFBM - 1.0f) * 0.64f, 1.0f, 0.0f, 1.0f);
            base = max(0.0f, base);
            transmittance *= exp(-stepLength * base * densityScale);

            Ray shadowRay;
            shadowRay.mOrigin = r.mOrigin;
            shadowRay.mDir = normalize(sunPos - shadowRay.mOrigin);
            vec4 lightTransmittance = vec4(1.0f);
            for(int j = 0; j < renderParams.mSteps.y; ++j)
            {
                float shadowtMin = 0.0f;
                float shadowtMax = 0.0f;
                const bool foundIntersection = intersect(b, shadowRay, shadowtMin, shadowtMax);
                const float shadowStepLength = (height * 0.5f / float(renderParams.mSteps.y));
                if(foundIntersection)
                {
                    vec3 shadowUVW = (shadowRay.mOrigin - b.mMin) / (b.mMax - b.mMin);
                    shadowUVW.xz *= renderParams.mCloudMapping.xy;
                    
                    vec4 noise = texture(cloudTexture, shadowUVW);
                    const float coverage = hash12(uvw.xz) * 0.1 + (CLOUD_COVERAGE * .5 + .5);
                    float lowFreqFBM = noise.y * 0.625f + noise.z * 0.25f + noise.w * 0.125f;
                    float base = remap(noise.x, 1.0f - coverage, 1.0f, 0.0f, 1.0f) * coverage;
                    base = remap(base, (lowFreqFBM - 1.0f) * 0.64f, 1.0f, 0.0f, 1.0f);
                    base = max(0.0f, base);
                    lightTransmittance *= exp(-shadowStepLength * base * densityScale);
                }
                else
                {
                    break;
                }
                shadowRay.mOrigin = shadowRay.mOrigin + shadowStepLength * shadowRay.mDir;
            
                if(length(lightTransmittance) < 0.05f)
                {
                    break;
                }
            }
            
            cloudColor *= lightTransmittance;

            if(length(transmittance) < 0.05f)
            {
                break;
            }

            r.mOrigin = r.mOrigin + r.mDir * stepLength;
        }
    }

    cloudColor *= texture(environmentTexture, sunDir.xyz);
    transmittance = clamp(transmittance, 0.0f, 1.0f);
    if(transmittance < 1.0f)
    {
        hasClouds = true;
    }

    float t = 0.0f;
    if(foundIntersection && hasClouds)
    {
        c = vec4(mix(vec3(cloudColor.xyz), sky.xyz, transmittance), 1.0f);
    }
    else
    {
        c = vec4(sky, 1.0f);
    }
}