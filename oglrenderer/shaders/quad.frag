#version 450 core

#include "deviceconstants.h"

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
	c = vec4(uv.xy, 0.0, 1.0f);
}