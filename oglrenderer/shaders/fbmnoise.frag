#version 450 core
#define GLSL_SHADER

#include "deviceconstants.h"
#include "devicestructs.h"
#include "fbm.h"

layout(location = 1) in vec2 uv;
layout(std140, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std140, binding = FBM_PARAMS) uniform NoiseParamsUniform
{
	NoiseParams noiseParams;
};

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;

void main()
{	
    vec2 st = uv * noiseParams.mSettings.xy;

    const float noise = fbmWorley(noiseParams.mNoiseOctaves, st, renderParams.mTime, true);
	c = vec4(noise, noise, noise, 1.0f);
}