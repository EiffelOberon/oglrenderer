#pragma once

#include "shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void render();
    void resize(int width, int height);
};