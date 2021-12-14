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

layout(location = 0) out vec4 c;

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

	vec3 rayleigh;
	vec3 mie;
	vec3 sky;
	
	nishita_sky(max(1.0f, camParams.mEye.y), 20.0f, sunDir, rayDir.xyz,  rayleigh, mie, sky);

    Box b; 
    //float width = 1000000.0f;
    //float height = renderParams.mCloudSettings.w;
    //b.mMin = vec3(0.0f, 8000.0f, 0.0f) + vec3(-width, 0, -width);
    //b.mMax = vec3(0.0f, 8000.0f, 0.0f) + vec3(width, height, width);
    float width = 2.0f;
    float height = renderParams.mCloudSettings.w;
    b.mMin = vec3(0.0f, 0.0f, -2.0f) + vec3(-width, 0, -width);
    b.mMax = vec3(0.0f, 0.0f, -2.0f) + vec3(width, 2, width);

    Ray r;
    r.mOrigin = camParams.mEye.xyz;
    r.mDir = rayDir;

    float tMin = 0.0f;
    float tMax = 0.0f;
    const bool foundIntersection = intersect(b, r, tMin, tMax);

    const float stepLength = ((tMax - tMin) / float(renderParams.mSteps.x));
    r.mOrigin = r.mOrigin + r.mDir * tMin;

    vec4 cloudColor = vec4(0.0f);
    vec4 transmittance = vec4(1.0f);
    float densityScale = renderParams.mCloudSettings.z;
    if(foundIntersection && tMin > 0 && tMax > tMin)
    {
        for(int i = 0; i < renderParams.mSteps.x; ++i)
        {   
            vec3 uvw = (r.mOrigin - b.mMin) / (b.mMax - b.mMin);
            uvw.xz *= renderParams.mCloudMapping.xy;
            
            vec4 noise = clamp(texture(cloudTexture, uvw), 0.0f, 1.0f);
            float lowFreqFBM = noise.y * 0.625f + noise.z * 0.25f + noise.w * 0.125f;
            float base = remap(noise.x, (1.0f - lowFreqFBM), 1.0f, 0.0f, 1.0f);
            transmittance *= exp(-stepLength * base * densityScale);

            Ray shadowRay;
            shadowRay.mOrigin = r.mOrigin;
            shadowRay.mDir = sunDir;
            vec4 lightTransmittance = vec4(1.0f);
            for(int j = 0; j < renderParams.mSteps.y; ++j)
            {
                float shadowtMin = 0.0f;
                float shadowtMax = 0.0f;
                const bool foundIntersection = intersect(b, shadowRay, shadowtMin, shadowtMax);
                const float shadowStepLength = (length(b.mMax - b.mMin) / float(renderParams.mSteps.y));
                if(foundIntersection)
                {
                    vec3 shadowUVW = (shadowRay.mOrigin - b.mMin) / (b.mMax - b.mMin);
                    shadowUVW.xz *= renderParams.mCloudMapping.xy;
                    
                    vec4 noise = texture(cloudTexture, shadowUVW);
                    float lowFreqFBM = noise.y * 0.625f + noise.z * 0.25f + noise.w * 0.125f;
                    float base = remap(noise.x, (1.0f - lowFreqFBM), 1.0f, 0.0f, 1.0f);
                    lightTransmittance *= exp(-shadowStepLength * base * densityScale);
                    shadowRay.mOrigin = shadowRay.mOrigin + shadowStepLength * shadowRay.mDir;
                }
                else
                {
                    break;
                }
            }
            cloudColor += vec4(1.0f, 1.0f, 1.0f, 1.0f) * clamp(dot(shadowRay.mDir, r.mDir), 0.0f, 1.0f) * lightTransmittance;

            if(length(transmittance) < 0.05f)
            {
                break;
            }

            r.mOrigin = r.mOrigin + r.mDir * stepLength;
        }
    }
    transmittance = clamp(transmittance, 0.0f, 1.0f);

    float t = 0.0f;
    if(foundIntersection)
    {
        c = vec4(mix(cloudColor.xyz, sky, transmittance.xyz), 1.0f);
    }
    else
    {
        c = vec4(sky, 1.0f);
    }
}