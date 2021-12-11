#version 450 core

#include "deviceconstants.h"

layout(location = 0) out vec4 c;

layout(binding = SCREEN_QUAD) uniform sampler2D mainTexture;

layout(location = 1) in vec2 uv;

void main()
{
	// floating point texture, nothing to worry about
	vec3 texResult = texture(mainTexture, uv).xyz;
	c = vec4(texResult, 1.0f);
}