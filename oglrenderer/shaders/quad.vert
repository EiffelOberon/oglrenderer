#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"

layout(location = 0) in vec3 vertexPos;
layout(location = 1) in vec3 vertexUV;

layout(std430, binding = ORTHO_MATRIX) uniform OrthoMatrixParams
{
    mat4 orthoMatrix;
};
layout(std430, binding = MVP_MATRIX) uniform MVPParams
{
    MVPMatrix mvpMatrix;
};

layout(location = 1) out vec2 uv;
layout(location = 2) out vec4 near;
layout(location = 3) out vec4 far;

void main()
{
	gl_Position =  orthoMatrix * vec4(vertexPos, 1.0);
	uv = vertexUV.xy;
	
	const mat4 invMVP = inverse(mvpMatrix.mProjectionMatrix * mvpMatrix.mViewMatrix);
	near = invMVP * vec4(gl_Position.xy / gl_Position.w, -1.0f, 1.0f);
	far  = invMVP * vec4(gl_Position.xy / gl_Position.w , 1.0f, 1.0f);
}