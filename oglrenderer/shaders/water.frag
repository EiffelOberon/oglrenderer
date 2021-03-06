#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "bsdf.h"
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
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
    RendererParams renderParams;
};


layout(binding = WATER_DISPLACEMENT1_TEX) uniform sampler2D displacement1;
layout(binding = WATER_DISPLACEMENT2_TEX) uniform sampler2D displacement2;
layout(binding = WATER_DISPLACEMENT3_TEX) uniform sampler2D displacement3;
layout(binding = WATER_ENV_TEX) uniform samplerCube environmentTex;
layout(binding = WATER_FOAM_TEX) uniform sampler2D foamTex;

layout(binding = WATER_IRRADIANCE) uniform samplerCube irradianceTex;
layout(binding = WATER_PREFILTER_ENV) uniform samplerCube prefilterTex;
layout(binding = WATER_PRECOMPUTED_GGX) uniform sampler2D precomputedGGXTex;

layout(location = 0) out vec4 c;

void main()
{
	const vec2 wave = oceanParams.mWaveSettings.zw * oceanParams.mWaveSettings.y;
	const vec2 testUV1 = (uv + wave * renderParams.mSettings.x) / OCEAN_DIMENSIONS_1;
	const vec2 testUV2 = (uv + wave * renderParams.mSettings.x) / OCEAN_DIMENSIONS_2;
	const vec2 testUV3 = (uv + wave * renderParams.mSettings.x) / OCEAN_DIMENSIONS_3;

	// calculate normal per pixel
	const vec3 displacementLambda = vec3(oceanParams.mReflection.w, oceanParams.mWaveSettings.x, oceanParams.mReflection.w);
    const vec3 d1 = displacementLambda * texture(displacement1, testUV1).xyz;
    const vec3 d2 = displacementLambda * texture(displacement2, testUV2).xyz;
    const vec3 d3 = displacementLambda * texture(displacement3, testUV3).xyz;
	const vec3 d = d1 + d2 + d3;

	const vec3 neighborX = vec3(1, 0, 0) + 
        displacementLambda * texture(displacement1, testUV1 + vec2(1.0f / OCEAN_DIMENSIONS_1, 0)).xyz +
        displacementLambda * texture(displacement2, testUV2 + vec2(1.0f / OCEAN_DIMENSIONS_2, 0)).xyz +
        displacementLambda * texture(displacement3, testUV3 + vec2(1.0f / OCEAN_DIMENSIONS_3, 0)).xyz;
	const vec3 neighborY = vec3(0, 0, 1) + 
        displacementLambda * texture(displacement1, testUV1 + vec2(0.0f, 1.0f / OCEAN_DIMENSIONS_1)).xyz +
        displacementLambda * texture(displacement2, testUV2 + vec2(0.0f, 1.0f / OCEAN_DIMENSIONS_2)).xyz +
        displacementLambda * texture(displacement3, testUV3 + vec2(0.0f, 1.0f / OCEAN_DIMENSIONS_3)).xyz;

	const vec3 dDdx = (neighborX - vec3(1, 0, 0)) - (d);
	const vec3 dDdy = (neighborY - vec3(0, 0, 1)) - (d);
	
	const float jxx = 1 + dDdx.x;
	const float jyy = 1 + dDdy.z;
	const float jyx = dDdy.x;

	float jacobian = jxx * jyy - jyx * jyx;

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

	vec3 radiance = vec3(0.0f);
	vec3 rayDir = normalize(reflect(-viewDir, n));
	rayDir.y = max(rayDir.y, 0.0f);
	
	distanceToCamera = clamp(length(position - camParams.mEye.xyz), 0.0f, oceanParams.mTransmission.w) / oceanParams.mTransmission.w;
    const float waveHeight = mix(clamp(d.y, 0.0f, oceanParams.mWaveSettings.x) / oceanParams.mWaveSettings.x, 0, distanceToCamera);

	// transmission color
    vec3 transmission = mix(oceanParams.mTransmission.xyz, oceanParams.mTransmission2.xyz, pow(waveHeight, oceanParams.mTransmission2.w));
    
	// direct specular and indirect specular components
	const vec3 sunColor = (skyParams.mPrecomputeSettings.y == NISHITA_SKY) ? 
						  texture(environmentTex, skyParams.mSunSetting.xyz).xyz : 
						  vec3(1.0f);
	vec3 indirectReflection;

    // clamp nDotV to avoid numerical error
    const float nDotV = max(dot(n, viewDir), 0.01f);
    // get PBR input parameters
    const float roughness = 0.2f;
    const float ior = 1.3f;
    const float metallic = 0.0f;

    const vec3 ggx = texture(precomputedGGXTex, vec2(roughness, nDotV)).xyz;
	//if(renderParams.mSettings.z == 0)
	//{
	//	// indirect specular
	//	vec3 L = vec4(textureLod(prefilterTex, rayDir, roughness * float(PREFILTER_MIP_COUNT - 1)).xyz, 1.0f).xyz;
	//	
	//	// calculate fresnel
	//	float f0 = (ior - 1.0f) / (ior + 1.0f);
	//	f0 *= f0;
	//	f0 = mix(f0, 1.0f, metallic);
	//	indirectReflection = L;
	//
	//	radiance += mix(transmission, oceanParams.mReflection.xyz * indirectReflection * (f0 * ggx.x + ggx.y), ggx.z);
	//}
	//else
	{	
		//indirectReflection = max(texture(environmentTex, rayDir).xyz, 0.0f);
		radiance += mix(transmission, oceanParams.mReflection.xyz * max(texture(environmentTex, rayDir).xyz, 0.0f), ggx.z);
	}

	if(jacobian < oceanParams.mFoamSettings.y)
	{
		const float foam = pow(texture(foamTex, testUV1 / oceanParams.mFoamSettings.x).x, 2.2f);
		radiance = vec3(mix(radiance, vec3((foam * oceanParams.mReflection.w) * luminance(texture(irradianceTex, n).xyz)), foam));
	}

	// direct specular + indirect specular + transmission
	vec3 directSpecular = pow(clamp(dot(reflect(-sunDir, n), viewDir), 0.0f, 1.0f), skyParams.mSunSetting.w) * sunColor;
	radiance += directSpecular;

    const float distance = length(camParams.mEye.xyz - position);
    float alpha = 1 - clamp((distance - skyParams.mFogSettings.x) / (skyParams.mFogSettings.y - skyParams.mFogSettings.x), 0.0f, 1.0f);
	c = vec4(radiance, alpha);
	
	// color correction for main pass if this is not pre-render
	if(renderParams.mSettings.z == 0)
	{
		c = pow(c, vec4(1.0f / 2.2f));
	}
}