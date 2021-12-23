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
layout(std430, binding = CAMERA_PARAMS) uniform CameraParamsUniform
{
	CameraParams camParams;
};
layout(std430, binding = OCEAN_PARAMS) uniform OceanParamsUniform
{
    OceanParams oceanParams;
};


layout(binding = WATER_DISPLACEMENT_TEX) uniform sampler2D displacement;

layout(location = 1) out vec3 position;
layout(location = 2) out vec2 uv;

void main()
{
	const vec2 testUV = (vertexPos.xz / OCEAN_RESOLUTION);
	const float distanceToCamera = clamp(length(vertexPos.xyz - camParams.mEye.xyz), 0.0f, oceanParams.mTransmission.w) / oceanParams.mTransmission.w;

    const vec3 d = mix(texture(displacement, testUV).xyz, vec3(0, 0, 0), distanceToCamera);
	const vec3 newVertexPos = vertexPos + d;

	gl_Position =  mvpMatrix.mProjectionMatrix * mvpMatrix.mViewMatrix * vec4(newVertexPos, 1.0);
	
	position = newVertexPos;
	uv = testUV;
}