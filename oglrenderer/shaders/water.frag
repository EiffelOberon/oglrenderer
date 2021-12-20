#version 450 core
#define GLSL_SHADER
#extension GL_EXT_scalar_block_layout : require

#include "cloud.h"
#include "deviceconstants.h"
#include "devicestructs.h"
#include "nishita.h"

layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec4 c;

void main()
{	
    c = vec4(1, 0, 0, 1);
}