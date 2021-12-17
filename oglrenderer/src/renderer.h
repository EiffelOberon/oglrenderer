#pragma once

#include <chrono>
#include <memory.h>

#include "glm/glm.hpp"

#include "camera.h"
#include "deviceconstants.h" 
#include "devicestructs.h"
#include "oceanfft.h"
#include "quad.h"
#include "rendertexture.h"
#include "shader.h"
#include "shaderbuffer.h"
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

    // initialize uniform white noise [0, 1]
    void updateOceanNoiseTexture();

    // methods for saving/loading settings
    void saveStates();
    void loadStates();

    // textures
    Texture3D mCloudTexture;
    Texture   mOceanH0SpectrumTexture;
    Texture   mOceanHDxSpectrumTexture;
    Texture   mOceanHDySpectrumTexture;
    Texture   mOceanHDzSpectrumTexture;
    std::unique_ptr<Texture> mOceanNoiseTexture;
    Texture   mButterFlyTexture;

    // shader buffers
    ShaderBuffer mButterflyIndicesBuffer;

    // shaders
    ShaderProgram mPrecomputeEnvironmentShader;
    ShaderProgram mPrecomputeCloudShader;
    ShaderProgram mPrecomputeOceanH0Shader;
    ShaderProgram mPrecomputeOceanHShader;
    ShaderProgram mPrecomputeButterflyTexShader;
    ShaderProgram mPrerenderQuadShader;
    ShaderProgram mTexturedQuadShader;
    ShaderProgram mWorleyNoiseQuadShader;
    ShaderProgram mPerlinNoiseQuadShader;
    ShaderProgram mCloudNoiseQuadShader;
    
    // quad geometry
    Quad          mQuad;

    // resolution
    glm::vec2     mResolution;
    glm::vec2     mEnvironmentResolution;

    // multiplier for lower resolution for down-res background
    float         mLowResFactor;
    
    // update boolean
    bool          mUpdateEnvironment;

    // 2D textures to display
    std::unique_ptr<RenderTexture> mRenderTexture;
    std::unique_ptr<RenderTexture> mWorleyNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mPerlinNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mCloudNoiseRenderTexture[4];

    // environment cubemap texture
    std::unique_ptr<RenderCubemapTexture> mRenderCubemapTexture;

    // states
    Camera mCamera;
    CameraParams mCamParams;
    RendererParams mRenderParams;
    SkyParams mSkyParams;
    OceanParams mOceanParams;

    // noise 
    NoiseParams mWorleyNoiseParams;
    NoiseParams mPerlinNoiseParams;

    OceanFFT mOceanFFT;

    // gui
    bool mShowOceanWindow;
    bool mShowPerformanceWindow;
    bool mShowSkyWindow;

    float mDeltaTime;
    float mTime;
    uint32_t mFrameCount;
    std::chrono::steady_clock::time_point mRenderStartTime;
    std::chrono::steady_clock::time_point mRenderEndTime;

    std::map<uint32_t, std::unique_ptr<UniformBuffer>> mUniforms;
};