#version 450 core
#define GLSL_SHADER

#include "deviceconstants.h"
#include "devicestructs.h"
#include "fbm.h"
#include "perlin.h"
#include "worley.h"

layout(location = 1) in vec2 uv;
layout(std140, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std140, binding = FBM_PARAMS) uniform FBMParamsUniform
{
	NoiseParams fbmParams;
};
layout(std140, binding = PERLIN_PARAMS) uniform PerlinParamsUniform
{
	NoiseParams perlinParams;
};
layout(std140, binding = WORLEY_PARAMS) uniform WorleyParamsUniform
{
	NoiseParams worleyParams;
};

layout(location = 0) out vec4 c;

void main()
{	
    const vec2 st = uv * fbmParams.mSettings.xy;
	//float noise = perlin3D2(vec3(st, renderParams.mTime));
	c = vec4(st.xy, 0.0f, 1.0f);
}