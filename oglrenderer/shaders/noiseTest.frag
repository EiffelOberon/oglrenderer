#version 450 core
#define GLSL_SHADER

#include "deviceconstants.h"
#include "devicestructs.h"
#include "nishita.h"

layout(location = 1) in vec2 uv;

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;


void main()
{	
	c = vec4(uv.xy, 0.0f, 1.0f);
}