#version 450 core

layout(location = 0) in vec3 vertex_position;

layout(std140, binding = 0) uniform Matrices
{
    mat4 mvp;
};

void main()
{
	gl_Position =  mvp * vec4(vertex_position, 1.0);
}