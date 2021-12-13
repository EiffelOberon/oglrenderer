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

layout(location = 0) out vec4 c;

void main()
{	
    const vec3 st = vec3(uv, 0.0f);
	float noise = perlinWorley3D(st, renderParams.mSettings.x * renderParams.mCloudSettings.y, perlinParams.mSettings.z, perlinParams.mNoiseOctaves, true);
	

    // fake cloud coverage
    noise = remap(noise, renderParams.mCloudSettings.x, 1.0f, 0.0f, 1.0f);
	c = vec4(noise, noise, noise, 1.0f);
}