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
layout(binding = WATER_DISPLACEMENT_TEX) uniform sampler2D displacement;
layout(binding = WATER_ENV_TEX) uniform samplerCube environmentTex;

layout(location = 0) out vec4 c;

void main()
{	
	// calculate normal per pixel
    const vec3 d= texture(displacement, uv).xyz;
	const vec3 neighborX = vec3(1, 0, 0) + texture(displacement, uv + vec2(1.0f / OCEAN_RESOLUTION, 0)).xyz;
	const vec3 neighborY = vec3(0, 0, 1) + texture(displacement, uv + vec2(0.0f, 1.0f / OCEAN_RESOLUTION)).xyz;
	const vec3 tangent = normalize(neighborX - d);
	const vec3 bitangent = normalize(neighborY - d);
	vec3 n = normalize(cross(tangent, bitangent));
	if(dot(vec3(0, 1, 0), n) < 0)
	{
		n *= -1;
	}
	
	// calculate directional vectors
	const vec3 viewDir = normalize(camParams.mEye.xyz - position);
	const vec3 sunDir = normalize(skyParams.mSunSetting.xyz);
	const vec3 halfDir = normalize(viewDir + sunDir);

	const float r0 = pow((1.3f - 1.0f) / (1.3f + 1.0f), 2.0f);
	const float f = clamp(r0 + (1.0f - r0) * pow(1 - dot(viewDir, n), 5.0f), 0.0f, 1.0f);

	vec3 radiance = vec3(0.0f);
	vec3 rayDir = normalize(reflect(-viewDir, n));
	rayDir.y = max(rayDir.y, 0.1f);

	// transmission color
	vec3 transmission = mix(oceanParams.mTransmission.xyz, oceanParams.mTransmission2.xyz, pow(abs(dot(viewDir, n)), oceanParams.mTransmission2.w));

	// direct specular and indirect specular components
	const vec3 directSpecular = pow(clamp(dot(reflect(-sunDir, n), viewDir), 0.0f, 1.0f), 40.0f) * texture(environmentTex, skyParams.mSunSetting.xyz).xyz;
	const vec3 indirectReflection = max(texture(environmentTex, rayDir).xyz - directSpecular, 0.0f);

	// direct specular + indirect specular + transmission
	radiance += directSpecular;
	radiance += mix(transmission, indirectReflection, f);

	c = vec4(radiance, 1.0f);
}