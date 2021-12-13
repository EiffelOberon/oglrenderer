#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexUV;

layout(std430, binding = ORTHO_MATRIX) uniform Matrices
{
    mat4 mvp;
};

layout(location = 1) out vec2 uv;

void main()
{
	gl_Position =  mvp * vec4(vertexPos, 1.0);
	uv = vertexUV.xy;
}