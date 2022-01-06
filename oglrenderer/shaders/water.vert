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
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
    RendererParams renderParams;
};


layout(binding = WATER_DISPLACEMENT1_TEX) uniform sampler2D displacement1;
layout(binding = WATER_DISPLACEMENT2_TEX) uniform sampler2D displacement2;
layout(binding = WATER_DISPLACEMENT3_TEX) uniform sampler2D displacement3;

layout(location = 1) out vec3 position;
layout(location = 2) out vec2 uv;

void main()
{
	const vec2 testUV1 = vertexPos.xz / OCEAN_DIMENSIONS_1;
	const vec2 testUV2 = vertexPos.xz / OCEAN_DIMENSIONS_2;
	const vec2 testUV3 = vertexPos.xz / OCEAN_DIMENSIONS_3;

	const vec3 displacementLambda = vec3(oceanParams.mReflection.w, oceanParams.mWaveSettings.x, oceanParams.mReflection.w);
    const vec3 d1 = displacementLambda * texture(displacement1, testUV1).xyz;
    const vec3 d2 = displacementLambda * texture(displacement2, testUV2).xyz;
    const vec3 d3 = displacementLambda * texture(displacement3, testUV3).xyz;
	vec3 newVertexPos = vertexPos + d1 + d2 + d3;
    if (renderParams.mSettings.z == 0)
    {
        const float distanceToCamera = clamp(length(newVertexPos - camParams.mEye.xyz), 0.4f, oceanParams.mTransmission.w) / oceanParams.mTransmission.w;
        newVertexPos = mix(newVertexPos, vertexPos, distanceToCamera);
    }
    else
    {
        // reduce displacement at far field for probe precomputation to avoid artifacts
        const float distanceToCamera = clamp(length(newVertexPos - camParams.mEye.xyz), 0.4f, 100.0f) / 100.0f;
        newVertexPos = mix(newVertexPos, vertexPos, distanceToCamera);
    }
	gl_Position =  viewProjectionMat.mProjectionMatrix * viewProjectionMat.mViewMatrix * vec4(newVertexPos, 1.0);
	
	position = newVertexPos;
	uv = vertexPos.xz;
}