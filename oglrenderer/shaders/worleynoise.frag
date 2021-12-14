#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"
#include "cloud.h"
#include "worley.h"

layout(location = 1) in vec2 uv;
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std430, binding = WORLEY_PARAMS) uniform NoiseParamsUniform
{
	NoiseParams noiseParams;
};
layout(std430, binding = PERLIN_PARAMS) uniform PerlinParamsUniform
{
	NoiseParams perlinParams;
};

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;

void main()
{	
    const vec3 st = vec3(uv, 0.0f);
	const float noise = worleyFBM(st * perlinParams.mSettings.z, renderParams.mSettings.x * renderParams.mCloudSettings.y  * perlinParams.mSettings.z, 1.0f, noiseParams.mInvert);
	
    //const float noise = worley3D(st * perlinParams.mSettings.z, renderParams.mSettings.x * renderParams.mCloudSettings.y * perlinParams.mSettings.z, true);
	c = vec4(vec3(noise), 1.0f);
}