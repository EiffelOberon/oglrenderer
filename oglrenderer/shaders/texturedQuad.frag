#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"

layout(location = 0) out vec4 c;

layout(binding = SCREEN_QUAD_TEX) uniform sampler2D mainTexture;

layout(location = 1) in vec2 uv;


vec3 ACESTonemap(
    const vec3 x)
{
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}


void main()
{
	// floating point texture -> display sRGB so need to convert
	vec3 texResult = texture(mainTexture, uv).xyz;
    texResult = ACESTonemap(texResult);
	texResult = pow(texResult, vec3(1.0f / 2.2f));
	c = vec4(texResult, 1.0f);
}