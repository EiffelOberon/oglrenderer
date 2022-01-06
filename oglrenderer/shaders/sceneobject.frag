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
layout(std430, binding = SCENE_MATERIAL) buffer MaterialsBuffer
{
    Material materials[];
};

layout(binding = SCENE_OBJECT_IRRADIANCE) uniform samplerCube irradianceTex;
layout(binding = SCENE_OBJECT_PREFILTER_ENV) uniform samplerCube prefilterTex;
layout(binding = SCENE_OBJECT_PRECOMPUTED_GGX) uniform sampler2D precomputedGGXTex;
layout(binding = SCENE_OBJECT_SKY) uniform samplerCube skyTex;
layout(binding = SCENE_OBJECT_DIFFUSE) uniform sampler2D diffuseTex;

layout(location = 0) out vec4 c;

void main()
{	
	// diffuse irradiance
	const int matId = renderParams.mScreenSettings.w;
	vec3 albedo = materials[matId].mTexture1.x == INVALID_TEX_ID ? 
				  vec3(1.0f) :
				  pow(texture(diffuseTex, uv).xyz, vec3(2.2f));
    vec3 indirectDiffuse = albedo * texture(irradianceTex, normal).xyz;
	
	// direct diffuse
	const vec3 viewDir = normalize(camParams.mEye.xyz - position);
	const vec3 sunDir = normalize(skyParams.mSunSetting.xyz);
	const vec3 sunColor = (skyParams.mPrecomputeSettings.y == NISHITA_SKY) ? 
						  texture(skyTex, sunDir).xyz : 
						  vec3(1.0f);
	vec3 directDiffuse = sunColor * albedo * max(0.0f, dot(normal, sunDir));

	// get PBR input parameters
	const vec4 shadingParams = materials[matId].mShadingParams;
	const float roughness = clamp(shadingParams.x, 0.0f, 0.99f);
	const float ior = shadingParams.z;
	const float metallic = shadingParams.y;

	// indirect specular
	const vec3 rayDir = normalize(reflect(-viewDir, normal));
	const float nDotV = max(dot(normal, viewDir), 0.0f);

    // calculate box corrected ray dir for cubemap
    const vec3 correctedRayDir = normalize((position + rayDir * 10000.0f) - vec3(PRECOMPUTE_CAM_POS_X, PRECOMPUTE_CAM_POS_Y, PRECOMPUTE_CAM_POS_Z));
	vec3 L = vec4(textureLod(prefilterTex, correctedRayDir, roughness * float(PREFILTER_MIP_COUNT - 1)).xyz, 1.0f).xyz;
	vec2 ggx = texture(precomputedGGXTex, vec2(roughness, nDotV)).xy;
	float f0 = (ior - 1.0f) / (ior + 1.0f);
	f0 *= f0;
	f0 = mix(f0, 1.0f, metallic);
    const vec3 indirectSpecular = L * (f0 * ggx.x + ggx.y)* mix(vec3(1.0f), albedo, metallic);
	
	// add them up
	c = vec4(mix(indirectDiffuse, vec3(0.0f), metallic) + indirectSpecular, 1.0f);
	
	// color correction for main pass if this is not pre-render
	if(renderParams.mSettings.z == 0)
	{
		c = pow(c, vec4(1.0f / 2.2f));
	}
}