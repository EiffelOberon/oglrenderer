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
layout(std430, binding = OCEAN_PARAMS) uniform OceanParamsUniform
{
    OceanParams oceanParams;
};


layout(binding = WATER_DISPLACEMENT_TEX) uniform sampler2D displacement;

layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 uv;

void main()
{
    const float height = texture(displacement, vertexUV.xy).x;
	const vec3 newVertexPos = vec3(vertexPos.x, height, vertexPos.z);

	gl_Position =  mvpMatrix.mProjectionMatrix * mvpMatrix.mModelViewMatrix * vec4(newVertexPos, 1.0);
	normal = vec3(0, 1, 0);
	uv = vertexUV.xy;
}