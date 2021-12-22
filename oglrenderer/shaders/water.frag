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
layout(std430, binding = OCEAN_PARAMS) uniform OceanParamsUniform
{
    OceanParams oceanParams;
};
layout(binding = WATER_NORMAL_TEX) uniform sampler2D normalTex;
layout(binding = WATER_ENV_TEX) uniform samplerCube environmentTex;

layout(location = 0) out vec4 c;

void main()
{	
	vec3 n = normalize(texture(normalTex, uv.xy).xyz);
	
	const vec3 viewDir = normalize(camParams.mEye.xyz - position);
	const vec3 sunDir = normalize(skyParams.mSunSetting.xyz);
	const float cosTheta = clamp(dot(viewDir, n), 0.0f, 1.0f);
	const float r0 = pow((1.3f - 1.0f) / (1.3f + 1.0f), 2.0f);
	const float f = clamp(r0 + (1.0f - r0) * pow(1 - cosTheta, 5.0f), 0.0f, 1.0f);

	vec3 radiance = vec3(0.0f);
	vec3 rayDir = normalize(reflect(-viewDir, n));
    vec3 indirectReflection = vec3(0.0f);
	if(rayDir.y > 0)
	{
        indirectReflection = texture(environmentTex, rayDir).xyz;
	}

	const float directSpecular = pow(clamp(dot(reflect(-sunDir, n), viewDir), 0.0f, 1.0f), 20.0f);
	// direct specular + indirect specular + transmission
    radiance += (directSpecular);
    radiance += mix(mix(oceanParams.mTransmission.xyz, oceanParams.mTransmission2.xyz, pow(cosTheta, oceanParams.mTransmission2.w)), indirectReflection, f);
	c = vec4(radiance, 1.0f);
}