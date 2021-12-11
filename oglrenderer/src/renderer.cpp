#include "renderer.h"
#include "freeglut.h"

Renderer::Renderer()
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


void Renderer::render()
{
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);



    glutSwapBuffers();
}