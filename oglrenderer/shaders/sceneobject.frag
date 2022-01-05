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
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
    RendererParams renderParams;
};


layout(binding = SCENE_OBJECT_IRRADIANCE) uniform samplerCube irradianceTex;
layout(binding = SCENE_OBJECT_PREFILTER_ENV) uniform samplerCube prefilterTex;
layout(binding = SCENE_OBJECT_PRECOMPUTED_GGX) uniform sampler2D precomputedGGXTex;
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

	// get PBR input parameters
	const float roughness = clamp(skyParams.mPrecomputeGGXSettings.y, 0.0f, 0.99f);
	const float ior = skyParams.mPrecomputeGGXSettings.z;
	const float metallic = skyParams.mPrecomputeGGXSettings.w;

	// indirect specular
	const float nDotV = max(dot(normal, viewDir), 0.0f);
	vec3 L = vec4(textureLod(prefilterTex, normal, roughness * float(PREFILTER_MIP_COUNT - 1)).xyz, 1.0f).xyz;
	vec2 ggx = texture(precomputedGGXTex, vec2(roughness, nDotV)).xy;
	float f0 = (ior - 1.0f) / (ior + 1.0f);
	f0 *= f0;
	f0 = mix(f0, 1.0f, metallic);
	const vec3 indirectSpecular = L * (f0 * ggx.x + ggx.y);
	
	// add them up
	c = vec4(mix(indirectDiffuse, vec3(0.0f), metallic) + indirectSpecular, 1.0f);
	
	// color correction for main pass if this is not pre-render
	if(renderParams.mSettings.z == 0)
	{
		c = pow(c, vec4(1.0f / 2.2f));
	}
}