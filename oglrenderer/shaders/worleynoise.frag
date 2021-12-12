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
	const vec2 iST = floor(st);
	const vec2 fST = fract(st);
	const float minDist = worley(iST, fST, renderParams.mTime);

	vec3 color = vec3(minDist);
	//color += 1.0f - step(0.02, minDist);
	//color += step(.98, fST.x) + step(.98, fST.y);
	c = vec4(color.xyz, 1.0f);
}