#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "cloud.h"
#include "deviceconstants.h"
#include "devicestructs.h"
#include "nishita.h"

layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 near;
layout(location = 3) in vec4 far;

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
layout (binding = QUAD_CLOUD_TEX) uniform sampler3D cloudTexture;
layout (binding = QUAD_ENV_TEX) uniform samplerCube environmentTexture;

layout(location = 0) out vec4 c;

#include "raymarch.h"


void main()
{	
    Ray r;
    r.mOrigin = near.xyz / near.w;
    r.mDir = normalize((far.xyz / far.w) - r.mOrigin); 

    vec3 sunDir = length(skyParams.mSunSetting.xyz) > 0 ? normalize(skyParams.mSunSetting.xyz) : vec3(0, 1, 0);
    vec3 sky = texture(environmentTexture, r.mDir.xyz).xyz;
    
    c = vec4(sky, 1.0f);
    /*
    // disabled - we are not doing raymarching in real time anymore
    Box b; 
    float width = 30000.0f;
    float height = renderParams.mCloudSettings.w;
    b.mMin = vec3(0.0f, 2000.0f, 0.0f) + vec3(-width, 0, -width);
    b.mMax = vec3(0.0f, 2000.0f, 0.0f) + vec3(width, height, width);

    float tMin = 0.0f;
    float tMax = 0.0f;
    const bool foundIntersection = intersect(b, r, tMin, tMax);
    
    vec3 sunPos = skyParams.mSunSetting.xyz * 800000.0f;
    
    bool hasClouds = false;
    vec4 cloudColor = vec4(1.0f);
    float transmittance = 1.0f;

    if(foundIntersection)
    {
        raymarchCloud(
            r, 
            b,
            height,
            renderParams.mCloudSettings.z,
            renderParams.mCloudMapping.xy, 
            renderParams.mSteps.x, 
            renderParams.mSteps.y, 
            tMin, 
            tMax, 
            sunPos, 
            hasClouds, 
            cloudColor, 
            transmittance);
    }

    cloudColor *= texture(environmentTexture, sunDir.xyz);
    cloudColor.xyz = max(vec3(renderParams.mCloudSettings.x), cloudColor.xyz);

    transmittance = clamp(transmittance, 0.0f, 1.0f);
    if(transmittance < 1.0f)
    {
        hasClouds = true;
    }

    if(foundIntersection && hasClouds)
    {
        c = vec4(mix(vec3(cloudColor.xyz), sky.xyz, transmittance), 1.0f);
    }
    else
    {
        c = vec4(sky, 1.0f);
    }
    */
}