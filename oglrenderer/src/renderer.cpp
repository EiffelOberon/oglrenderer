#include "renderer.h"

#include "deviceconstants.h" 
#include "freeglut.h"
#include "glm/gtc/matrix_transform.hpp"

Renderer::Renderer()
    : mQuadShader("./spv/vert.spv", "./spv/frag.spv")
    , mQuad(GL_TRIANGLE_STRIP, 4)
{
    // quad initialization
    mQuad.update(0, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0, 0));
    mQuad.update(1, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 1));
    mQuad.update(2, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0));
    mQuad.update(3, glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1, 1));
    mQuad.upload();

    // initialize uniforms for quad shader
    glm::mat4 orthogonalMatrix = glm::orthoLH(0, 1, 0, 1, 0, 1);
    mQuadShader.addUniform<glm::mat4>(QUAD_MVP, orthogonalMatrix);
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

    mQuadShader.use();
    mQuad.draw();
    mQuadShader.disable();

    glutSwapBuffers();
}