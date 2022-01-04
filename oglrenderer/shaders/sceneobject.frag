#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
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

layout(binding = SCENE_OBJECT_IRRADIANCE) uniform samplerCube irradianceTex;
layout(binding = SCENE_OBJECT_SKY) uniform samplerCube skyTex;

layout(location = 0) out vec4 c;

void main()
{	
	// diffuse irradiance
    vec3 indirectDiffuse = texture(irradianceTex, normal).xyz;
	
	// direct diffuse
	const vec3 viewDir = normalize(camParams.mEye.xyz - position);
	const vec3 sunDir = normalize(skyParams.mSunSetting.xyz);
	
	const vec3 sunColor = (skyParams.mPrecomputeSettings.y == NISHITA_SKY) ? 
						  texture(skyTex, sunDir).xyz : 
						  vec3(1.0f);
	vec3 directDiffuse = sunColor * max(0.0f, dot(normal, sunDir));

	// add them up
	c = vec4(indirectDiffuse + directDiffuse, 1.0f);
}