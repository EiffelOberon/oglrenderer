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
}