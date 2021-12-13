#version 450 core
#define GLSL_SHADER

#include "deviceconstants.h"
#include "devicestructs.h"
#include "worley.h"

layout(location = 1) in vec2 uv;
layout(std140, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std140, binding = WORLEY_PARAMS) uniform NoiseParamsUniform
{
	NoiseParams noiseParams;
};

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;

void main()
{	
    const vec2 st = uv * noiseParams.mSettings.xy;
	const float minDist = worley(st, renderParams.mTime, noiseParams.mInvert);

	vec3 color = vec3(minDist);
	c = vec4(color.xyz, 1.0f);
}