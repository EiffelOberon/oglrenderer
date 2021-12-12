#pragma once

#include <memory.h>

#include "glm/glm.hpp"

#include "camera.h"
#include "deviceconstants.h" 
#include "devicestructs.h"
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
    ShaderProgram mNoiseTestQuadShader;
    Quad          mQuad;

    glm::vec2     mResolution;

    std::unique_ptr<RenderTexture> mRenderTexture;
    std::unique_ptr<RenderTexture> mCloudNoiseRenderTexture;

    Camera mCamera;
    CameraParams mCamParams;

    SkyParams mSkyParams;

    // gui
    bool mShowSkyWindow;
};