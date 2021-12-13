#version 450 core
#define GLSL_SHADER

#include "deviceconstants.h"
#include "devicestructs.h"
#include "perlin.h"

layout(location = 1) in vec2 uv;
layout(std140, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std140, binding = PERLIN_PARAMS) uniform PerlinParamsUniform
{
	NoiseParams noiseParams;
};

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;

void main()
{	
    const vec2 st = uv * noiseParams.mSettings.xy;
	float noise = perlin3D2(vec3(st, renderParams.mTime));
	c = vec4(noise, noise, noise, 1.0f);
}