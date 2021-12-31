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

layout (binding = PRECOMPUTE_ENVIRONMENT_CLOUD_TEX) uniform sampler3D cloudTexture;
layout (binding = PRECOMPUTE_ENVIRONMENT_NOISE_TEX) uniform sampler2D noiseTexture;
layout (binding = PRECOMPUTE_ENVIRONMENT_SKY_TEX)   uniform samplerCube skyTexture;

layout(location = 0) out vec4 c;

#include "raymarch.h"

vec3 uvToXYZ(
	const int face, 
	const vec2 uv)
{
	if(face == 0)
	{
		return vec3(1.0, -uv.y, -uv.x);
	}
	else if(face == 1)
	{
		return vec3(-1.f, -uv.y, uv.x);
	}
	else if(face == 2)
	{
		return vec3(+uv.x, 1.f, +uv.y);
	}
	else if(face == 3)
	{
		return vec3(+uv.x, -1.f,-uv.y);
	}
	else if(face == 4)
	{
		return vec3(+uv.x, -uv.y, 1.f);
	}
	else
	{	
		return vec3(-uv.x, -uv.y, -1.f);
	}
}

void main()
{	
    // create ray
	vec2 st = uv * 2.0f - 1.0f;
    vec3 rayDir = normalize(uvToXYZ(skyParams.mPrecomputeSettings.x, st));
    vec3 sunDir = length(skyParams.mSunSetting.xyz) > 0 ? normalize(skyParams.mSunSetting.xyz) : vec3(0, 1, 0);
	const vec3 sky = texture(skyTexture, rayDir.xyz).xyz;

    Ray r;
    r.mOrigin = vec3(0, 0, 0);
    r.mDir = rayDir; 

    Box b; 
    float width = 80000.0f;
    float height = renderParams.mCloudSettings.w;
    b.mMin = vec3(0.0f, 2000.0f, 0.0f) + vec3(-width, 0, -width);
    b.mMax = vec3(0.0f, 2000.0f, 0.0f) + vec3(width, height, width);

    float tMin = 0.0f;
    float tMax = 0.0f;
    const bool foundIntersection = intersect(b, r, tMin, tMax);
    
    vec3 sunPos = skyParams.mSunSetting.xyz * 800000.0f;
    
    bool hasClouds = false;
    vec4 cloudColor = vec4(0.0f);
    float transmittance = 1.0f;

    if(foundIntersection)
    {
        float offset = texture(noiseTexture, uv).x;
        offset = fract(offset + renderParams.mScreenSettings.z * 1.61803398875f);
        raymarchCloud(
            r, 
            b,
            offset,
            height,
            renderParams.mCloudSettings.z,
            renderParams.mCloudMapping.xy, 
            renderParams.mSteps.x, 
            renderParams.mSteps.y, 
            tMin, 
            tMax, 
            sunPos, 
            skyParams.mSunLuminance,
            hasClouds, 
            cloudColor, 
            transmittance);
    }

    transmittance = clamp(transmittance, 0.0f, 1.0f);
    if(transmittance < 1.0f)
    {
        hasClouds = true;
    }

    if(foundIntersection && hasClouds)
    {
        c = vec4(sky.xyz * transmittance + cloudColor.xyz * (1 - transmittance), 1.0f);
    }
    else
    {
        c = vec4(sky, 1.0f);
    }
}