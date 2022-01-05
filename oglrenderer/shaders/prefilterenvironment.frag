#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "bsdf.h"
#include "deviceconstants.h"
#include "devicestructs.h"
#include "random.h"

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

layout (binding = PREFILTER_ENVIRONMENT_SKY_TEX)   uniform samplerCube skyTexture;

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
	vec3 wo = normal;

	float roughness = skyParams.mPrecomputeSettings.z;
	float alpha = roughness * roughness;

	vec3 L = vec3(0.0f);
	float weight = 0.0f;
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        // random variables
        vec2 u = hammersley(i, SAMPLE_COUNT);
		
		// this is the magic of the separable BSDF convolution, assume normal is equal to view vector
		vec3 wh = GGX_sample(u, wo, vec2(alpha, alpha));
		vec3 wi = reflect(-wo, wh);

		const float nDotL = max(dot(wo, wi), 0.0f);
		if(nDotL > 0.0f)
		{
			L += texture(skyTexture, wi).xyz * nDotL;
			weight += nDotL;
		}
    }

	L /= weight;

	c = vec4(L, 1);
}