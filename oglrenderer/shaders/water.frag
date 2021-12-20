#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"

layout(location = 1) in vec3 position;
layout(location = 2) in vec2 uv;

layout(std430, binding = CAMERA_PARAMS) uniform CameraParamsUniform
{
	CameraParams camParams;
};
layout(std430, binding = SKY_PARAMS) uniform SkyParamsUniform
{
	SkyParams skyParams;
};
layout(binding = WATER_NORMAL_TEX) uniform sampler2D normalTex;
layout(binding = WATER_ENV_TEX) uniform samplerCube environmentTex;

layout(location = 0) out vec4 c;

void main()
{	
	const vec3 n = normalize(texture(normalTex, uv.xy).xyz);
	
	const vec3 viewDir = normalize(position - camParams.mEye.xyz);
	const vec3 rayDir = reflect(viewDir, n);

	vec3 env = texture(environmentTex, rayDir).xyz;

    float nDotL = max(dot(n, skyParams.mSunDir.xyz), 0.0f);
	c = vec4(vec3(env) * nDotL, 1.0f);
}