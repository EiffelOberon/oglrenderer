#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"
#include "cloud.h"
#include "fbm.h"
#include "perlin.h"
#include "worley.h"

layout(location = 1) in vec2 uv;
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std430, binding = FBM_PARAMS) uniform FBMParamsUniform
{
	NoiseParams fbmParams;
};
layout(std430, binding = PERLIN_PARAMS) uniform PerlinParamsUniform
{
	NoiseParams perlinParams;
};
layout(std430, binding = WORLEY_PARAMS) uniform WorleyParamsUniform
{
	NoiseParams worleyParams;
};
layout (binding = CLOUD_NOISE_CLOUD_TEX) uniform sampler3D cloudTexture;

layout(location = 0) out vec4 c;

void main()
{	
    const vec3 st = vec3(uv, 0.0f);
	vec4 noise = texture(cloudTexture, st);
	float result = 0.0f;
	switch(worleyParams.mTextureIdx)
	{
	case 0:	result = noise.x; break;
	case 1:	result = noise.y; break;
	case 2:	result = noise.z; break;
	case 3: result = noise.w; break;
	}
	c = vec4(result, result, result, 1.0f);
}