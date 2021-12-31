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
layout(location = 4) in vec2 oldUV;

layout(location = 3) uniform vec3 color;
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
layout (binding = QUAD_NOISE_TEX) uniform sampler2D noiseTexture;
layout (binding = QUAD_PREV_SCREEN_TEX) uniform sampler2D prevMainTexture;

layout(location = 0) out vec4 c;

#include "raymarch.h"


void main()
{	
    Ray r;
    r.mOrigin = near.xyz / near.w;
    r.mDir = normalize((far.xyz / far.w) - r.mOrigin); 

    vec3 sunDir = length(skyParams.mSunSetting.xyz) > 0 ? normalize(skyParams.mSunSetting.xyz) : vec3(0, 1, 0);
    vec3 sky = texture(environmentTexture, r.mDir.xyz).xyz;
    
    Box b; 
    float width = 80000.0f;
    float height = renderParams.mCloudSettings.w;
    b.mMin = vec3(0.0f, 2000.0f, 0.0f) + vec3(-width, 0, -width);
    b.mMax = vec3(0.0f, 2000.0f, 0.0f) + vec3(width, height, width);

    float tMin = 0.0f;
    float tMax = 0.0f;
    const bool foundIntersection = r.mDir.y > 0 && intersect(b, r, tMin, tMax);
    
    vec3 sunPos = skyParams.mSunSetting.xyz * 800000.0f;
    
    bool hasClouds = false;
    vec4 cloudColor = vec4(0.0f);
    float transmittance = 1.0f;

    if(foundIntersection)
    {
        float offset = texture(noiseTexture, uv * renderParams.mScreenSettings.xy / BLUENOISE_RESOLUTION).x;
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

        // temporal reprojection + taa
        if (renderParams.mScreenSettings.z > 1 &&
            oldUV.x >= 0.0f && oldUV.x <= 1.0f &&
            oldUV.y >= 0.0f && oldUV.y <= 1.0f)
        {
            const vec3 prevFrame1 = texture(prevMainTexture, oldUV).xyz;
            const vec3 prevFrame2 = texture(prevMainTexture, oldUV + vec2(1 / renderParams.mScreenSettings.x, 0.0f)).xyz;
            const vec3 prevFrame3 = texture(prevMainTexture, oldUV + vec2(0.0f, 1 / renderParams.mScreenSettings.y)).xyz;
            const vec3 prevFrame4 = texture(prevMainTexture, oldUV - vec2(1 / renderParams.mScreenSettings.x, 0.0f)).xyz;
            const vec3 prevFrame5 = texture(prevMainTexture, oldUV - vec2(0.0f, 1 / renderParams.mScreenSettings.y)).xyz;

            const vec3 prevFrame = (prevFrame1 * 2 + prevFrame2 + prevFrame3 + prevFrame4 + prevFrame5) / 6;

            c = vec4(mix(prevFrame.xyz, c.xyz, 0.5f), 1.0f);
        }
    }
    else
    {
        c = vec4(sky, 1.0f);
    }
}