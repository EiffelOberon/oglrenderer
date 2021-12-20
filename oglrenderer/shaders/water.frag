#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"

layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(std430, binding = SKY_PARAMS) uniform SkyParamsUniform
{
	SkyParams skyParams;
};
layout(binding = WATER_DISPLACEMENT_TEX) uniform sampler2D displacement;

layout(location = 0) out vec4 c;

void main()
{	
    float nDotL = abs(dot(normal, skyParams.mSunDir.xyz));
	c = vec4(uv.xy, 0.0f, 1.0f);
}