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
    ViewProjectionMatrix viewProjectionMat;
};
layout(std430, binding = CAMERA_PARAMS) uniform CameraParamsUniform
{
	CameraParams camParams;
};
layout(std430, binding = OCEAN_PARAMS) uniform OceanParamsUniform
{
    OceanParams oceanParams;
};
layout(std430, binding = SCENE_OBJECT_PARAMS) uniform SceneObjectParamsUniform
{
    SceneObjectParams sceneObjectParams;
};

layout(std430, binding = SCENE_MODEL_MATRIX) buffer SceneModelMatBuffer
{
    mat4 m[];
};

layout(location = 1) out vec3 normal;
layout(location = 2) out vec2 uv;

void main()
{
	gl_Position =  viewProjectionMat.mProjectionMatrix * viewProjectionMat.mViewMatrix * m[sceneObjectParams.mIndices.x] * vec4(vertexPos, 1.0);
    normal = (transpose(inverse(m[sceneObjectParams.mIndices.x])) * vec4(vertexNormal, 0.0f)).xyz;
	uv = vertexUV;
}