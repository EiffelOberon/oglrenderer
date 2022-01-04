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
    ViewProjectionMatrix viewProjectionMat;
};
layout(std430, binding = PREV_MVP_MATRIX) uniform PrevMVPParams
{
    ViewProjectionMatrix prevViewProjectionMat;
};

layout(location = 1) out vec2 uv;
layout(location = 2) out vec4 near;
layout(location = 3) out vec4 far;
layout(location = 4) out vec2 oldUV;

void main()
{
	gl_Position =  orthoMatrix * vec4(vertexPos, 1.0);
	uv = vertexUV.xy;
	
    // current frame
	mat4 invMVP = inverse(viewProjectionMat.mProjectionMatrix * viewProjectionMat.mViewMatrix);
	near = invMVP * vec4(gl_Position.xy / gl_Position.w, -1.0f, 1.0f);
	far  = invMVP * vec4(gl_Position.xy / gl_Position.w , 1.0f, 1.0f);

    vec4 oldScreenPos = (prevViewProjectionMat.mProjectionMatrix * prevViewProjectionMat.mViewMatrix * far);
    oldScreenPos /= oldScreenPos.w;
    oldUV = oldScreenPos.xy * 0.5f + 0.5f;
}