#version 450 core

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;

layout(location = 1) in vec2 uv;

void main()
{
	c = vec4(uv.xy, 0.0, 1.0f);
}