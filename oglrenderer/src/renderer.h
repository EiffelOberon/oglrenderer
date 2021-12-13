#pragma once

#include <chrono>
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
    ShaderProgram mFBMNoiseQuadShader;
    ShaderProgram mWorleyNoiseQuadShader;
    ShaderProgram mPerlinNoiseQuadShader;
    Quad          mQuad;

    glm::vec2     mResolution;

    std::unique_ptr<RenderTexture> mRenderTexture;
    std::unique_ptr<RenderTexture> mFBMNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mWorleyNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mPerlinNoiseRenderTexture;

    Camera mCamera;
    CameraParams mCamParams;
    RendererParams mRenderParams;
    SkyParams mSkyParams;

    // noise 
    NoiseParams mFBMNoiseParams;
    NoiseParams mWorleyNoiseParams;
    NoiseParams mPerlinNoiseParams;

    // gui
    bool mShowPerformanceWindow;
    bool mShowSkyWindow;

    float mDeltaTime;
    float mTime;
};