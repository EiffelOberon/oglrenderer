#version 450 core

layout(location = 3) uniform vec3 color;
layout(location = 0) out vec4 c;

void main()
{
	c = vec4(1.0, 0.0, 1.0, 1.0f);
}