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

    void updateCamera(const int deltaX, 
                      const int deltaY);

    void preRender();
    void render();
    void postRender();
    void renderGUI();
    void resize(int width, 
                int height);

private:

    template<class UniformType>
    void addUniform(
        uint32_t    bindingPt,
        UniformType& memoryBlock)
    {
        if (mUniforms.find(bindingPt) == mUniforms.end())
        {
            mUniforms[bindingPt] = std::make_unique<UniformBuffer>(sizeof(UniformType), bindingPt);
            mUniforms[bindingPt]->upload((void*)&memoryBlock);
        }
        else
        {
            // should not be adding multiple uniforms to the same binding point
            assert(false);
        }
    }


    template<class UniformType>
    void updateUniform(
        uint32_t     bindingPt,
        UniformType& memoryBlock)
    {
        mUniforms[bindingPt]->upload((void*)&memoryBlock);
    }


    template<class UniformType>
    void updateUniform(
        uint32_t     bindingPt,
        uint32_t     offsetInBytes,
        uint32_t     sizeInBytes,
        UniformType& memoryBlock)
    {
        mUniforms[bindingPt]->upload(offsetInBytes, sizeInBytes, (void*)&memoryBlock);
    }

    Texture3D mCloudTexture;

    ShaderProgram mPrecomputeCloudShader;
    ShaderProgram mPrerenderQuadShader;
    ShaderProgram mTexturedQuadShader;
    ShaderProgram mWorleyNoiseQuadShader;
    ShaderProgram mPerlinNoiseQuadShader;
    ShaderProgram mCloudNoiseQuadShader;
    Quad          mQuad;

    glm::vec2     mResolution;

    std::unique_ptr<RenderTexture> mRenderTexture;
    std::unique_ptr<RenderTexture> mWorleyNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mPerlinNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mCloudNoiseRenderTexture;

    Camera mCamera;
    CameraParams mCamParams;
    RendererParams mRenderParams;
    SkyParams mSkyParams;

    // noise 
    NoiseParams mWorleyNoiseParams;
    NoiseParams mPerlinNoiseParams;

    // gui
    bool mShowPerformanceWindow;
    bool mShowSkyWindow;

    float mDeltaTime;
    float mTime;
    uint32_t mFrameCount;
    std::chrono::steady_clock::time_point mRenderStartTime;
    std::chrono::steady_clock::time_point mRenderEndTime;

    std::map<uint32_t, std::unique_ptr<UniformBuffer>> mUniforms;

};