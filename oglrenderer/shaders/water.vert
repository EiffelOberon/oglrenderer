#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexNormal;
layout(location = 2) in vec2 vertexUV;

layout(std430, binding = MVP_MATRIX) uniform Matrices
{
    MVPMatrix mvpMatrix;
};

layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 uv;

void main()
{
	gl_Position =  mvpMatrix.mProjectionMatrix * mvpMatrix.mModelViewMatrix * vec4(vertexPos, 1.0);
	normal = normalize((transpose(inverse(mvpMatrix.mModelViewMatrix)) * vec4(vertexNormal, 0.0f)).xyz);
	uv = vertexUV.xy;
}