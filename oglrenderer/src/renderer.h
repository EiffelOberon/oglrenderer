#pragma once

#include "shader.h"
#include "shaderprogram.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void update();
    void render();
    void resize(int width, int height);

private:
    ShaderProgram mQuadShader;
};