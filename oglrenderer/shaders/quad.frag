#version 450 core

#include "deviceconstants.h"
#include "nishita.h"

layout(location = 1) in vec2 uv;

layout(location = 3) uniform vec3 color;
layout(std140, binding = CAMERA_PARAMS) uniform CameraParams
{
	vec4 mCameraPos;
	vec4 mCameraTarget;
	vec4 mCameraUp;
};

layout(location = 0) out vec4 c;


void main()
{	
    // create ray
    vec2 d = uv * 2.f - 1.f;
	vec3 V = mCameraUp.xyz;
	vec3 W = normalize((mCameraTarget - mCameraPos).xyz);
	vec3 U = cross(W, V);
    vec3 rayDir = normalize(d.x*U + d.y*V + W);
    
	vec3 rayleigh;
	vec3 mie;
	vec3 sky;
	
	nishita_sky(max(1.0f, mCameraPos.y), 20.0f, vec3(0, 1, 0), rayDir.xyz,  rayleigh, mie, sky);
	c = vec4(sky, 1.0f);
}