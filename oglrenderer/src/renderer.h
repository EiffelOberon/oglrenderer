#pragma once

#include <memory.h>

#include "glm/glm.hpp"

#include "camera.h"
#include "quad.h"
#include "rendertexture.h"
#include "shader.h"
#include "shaderprogram.h"
#include "texture.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    void render();
    void postRender();
    void resize(int width, int height);

private:
    ShaderProgram mPrerenderQuadShader;
    ShaderProgram mTexturedQuadShader;
    Quad          mQuad;

    glm::vec2     mResolution;

    std::unique_ptr<RenderTexture> mRenderTexture;

    Camera mCamera;
};