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

layout(binding = WATER_DISPLACEMENT1_TEX) uniform sampler2D displacement1;
layout(binding = WATER_DISPLACEMENT2_TEX) uniform sampler2D displacement2;
layout(binding = WATER_DISPLACEMENT3_TEX) uniform sampler2D displacement3;

layout(binding = WATER_ENV_TEX) uniform samplerCube environmentTex;

layout(location = 0) out vec4 c;

void main()
{	
	const vec2 testUV1 = uv / OCEAN_RESOLUTION_1;
	const vec2 testUV2 = uv / OCEAN_RESOLUTION_2;
	const vec2 testUV3 = uv / OCEAN_RESOLUTION_3;

	// calculate normal per pixel
    const vec3 d1 = texture(displacement1, testUV1).xyz;
	const vec3 d2 = texture(displacement2, testUV2).xyz;
	const vec3 d3 = texture(displacement3, testUV3).xyz;
	const vec3 d = d1 + d2 + d3;

	const vec3 neighborX = vec3(1, 0, 0) + 
						   texture(displacement1, testUV1 + vec2(1.0f / OCEAN_RESOLUTION_1, 0)).xyz +
						   texture(displacement2, testUV2 + vec2(1.0f / OCEAN_RESOLUTION_2, 0)).xyz +
						   texture(displacement3, testUV3 + vec2(1.0f / OCEAN_RESOLUTION_3, 0)).xyz;
	const vec3 neighborY = vec3(0, 0, 1) + 
	                       texture(displacement1, testUV1 + vec2(0.0f, 1.0f / OCEAN_RESOLUTION_1)).xyz +
						   texture(displacement2, testUV2 + vec2(0.0f, 1.0f / OCEAN_RESOLUTION_2)).xyz +
						   texture(displacement3, testUV3 + vec2(0.0f, 1.0f / OCEAN_RESOLUTION_3)).xyz;

	const vec3 tangent = normalize(neighborX - d);
	const vec3 bitangent = normalize(neighborY - d);
	vec3 n = normalize(cross(tangent, bitangent));
	if(dot(vec3(0, 1, 0), n) < 0)
	{
		n *= -1;
	}
	float distanceToCamera = clamp(length(position - camParams.mEye.xyz), 0.4f, oceanParams.mTransmission.w) / oceanParams.mTransmission.w;
	n = mix(n, vec3(0, 1, 0), distanceToCamera);
	
	// calculate directional vectors
	const vec3 viewDir = normalize(camParams.mEye.xyz - position);
	const vec3 sunDir = normalize(skyParams.mSunSetting.xyz);
	const vec3 halfDir = normalize(viewDir + sunDir);

	const float r0 = pow((1.3f - 1.0f) / (1.3f + 1.0f), 2.0f);
	const float f = clamp(r0 + (1.0f - r0) * pow(1 - dot(viewDir, n), 5.0f), 0.0f, 1.0f);

	vec3 radiance = vec3(0.0f);
	vec3 rayDir = normalize(reflect(-viewDir, n));
	rayDir.y = max(rayDir.y, 0.1f);
	
	distanceToCamera = clamp(length(position - camParams.mEye.xyz), 0.0f, oceanParams.mTransmission.w) / oceanParams.mTransmission.w;
    const float waveHeight = mix(clamp(d.y, 0.0f, oceanParams.mWaveSettings.x) / oceanParams.mWaveSettings.x, 0, distanceToCamera);

	// transmission color
    vec3 transmission = mix(oceanParams.mTransmission.xyz, oceanParams.mTransmission2.xyz, pow(waveHeight, oceanParams.mTransmission2.w));
    
	// direct specular and indirect specular components
	const vec3 sunColor = (skyParams.mPrecomputeSettings.y == NISHITA_SKY) ? 
						  texture(environmentTex, skyParams.mSunSetting.xyz).xyz : 
						  vec3(1.0f);
	const vec3 directSpecular = pow(clamp(dot(reflect(-sunDir, n), viewDir), 0.0f, 1.0f), skyParams.mSunSetting.w) * sunColor;
	const vec3 indirectReflection = max(texture(environmentTex, rayDir).xyz, 0.0f);

	// direct specular + indirect specular + transmission
	radiance += directSpecular;
	radiance += mix(transmission, oceanParams.mReflection.xyz * indirectReflection, f);

    const float distance = length(camParams.mEye.xyz - position);
    float alpha = 1 - clamp(distance - skyParams.mFogSettings.x, 0.0f, 1.0f) / (skyParams.mFogSettings.y - skyParams.mFogSettings.x);
	c = vec4(radiance, alpha);
}