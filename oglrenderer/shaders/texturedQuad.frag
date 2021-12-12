#version 450 core
#define GLSL_SHADER

#include "deviceconstants.h"

layout(location = 0) out vec4 c;

layout(binding = SCREEN_QUAD) uniform sampler2D mainTexture;

layout(location = 1) in vec2 uv;

void main()
{
	// floating point texture -> display sRGB so need to convert
	vec3 texResult = texture(mainTexture, uv).xyz;
	texResult = pow(texResult, vec3(1.0f / 2.2f));
	c = vec4(texResult, 1.0f);
}