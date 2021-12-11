#include "renderer.h"
#include "freeglut.h"

Renderer::Renderer()
    : mQuadShader("./spv/vert.spv", "./spv/frag.spv")
{

}

Renderer::~Renderer()
{

}


void Renderer::resize(
    int width, 
    int height)
{

}


void Renderer::update()
{
    render();
}


void Renderer::render()
{
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    glutSwapBuffers();
}