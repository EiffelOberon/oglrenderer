#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "deviceconstants.h"
#include "devicestructs.h"

layout(location = 1) in vec2 uv;

layout(location = 3) uniform vec3 color;
layout(std430, binding = CAMERA_PARAMS) uniform CameraParamsUniform
{
	CameraParams camParams;
};
layout(std430, binding = RENDERER_PARAMS) uniform RendererParamsUniform
{
	RendererParams renderParams;
};
layout(std430, binding = SKY_PARAMS) uniform SkyParamsUniform
{
	SkyParams skyParams;
};

layout (binding = PRECOMPUTE_IRRADIANCE_SKY_TEX)   uniform samplerCube skyTexture;

layout(location = 0) out vec4 c;


vec3 uvToXYZ(
	const int face, 
	const vec2 uv)
{
	if(face == 0)
	{
		return vec3(1.0, -uv.y, -uv.x);
	}
	else if(face == 1)
	{
		return vec3(-1.f, -uv.y, uv.x);
	}
	else if(face == 2)
	{
		return vec3(+uv.x, 1.f, +uv.y);
	}
	else if(face == 3)
	{
		return vec3(+uv.x, -1.f,-uv.y);
	}
	else if(face == 4)
	{
		return vec3(+uv.x, -uv.y, 1.f);
	}
	else
	{	
		return vec3(-uv.x, -uv.y, -1.f);
	}
}

void main()
{	
    // create ray
	vec2 st = uv * 2.0f - 1.0f;
    vec3 normal = normalize(uvToXYZ(skyParams.mPrecomputeSettings.x, st));

    vec3 irradiance = vec3(0.0f);

    vec3 up = vec3(0.0f, 1.0f, 0.0f);
    vec3 right = normalize(cross(up, normal));
    up = normalize(cross(normal, right));

    float sampleDelta = 0.025f;
    float nrSamples = 0.0f;
    for (float phi = 0.0f; phi < 2.0f * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            // spherical to cartesian (in tangent space), then convert tangent space to world
            const vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            const vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;

            irradiance += texture(skyTexture, sampleVec).rgb * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    c = vec4(PI * irradiance * (1.0f / float(nrSamples)), 1.0f);
}