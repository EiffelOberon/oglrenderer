#pragma once

#include <chrono>
#include <deque>
#include <unordered_map>
#include <memory.h>

#include "glm/glm.hpp"

#include "camera.h"
#include "clipmap.h"
#include "deviceconstants.h" 
#include "devicestructs.h"
#include "hosek.h"
#include "quad.h"
#include "rendertexture.h"
#include "shader.h"
#include "shaderbuffer.h"
#include "shaderprogram.h"
#include "texture.h"
#include "timequery.h"
#include "vertexbuffer.h"

class OceanFFT;
class Renderer
{
public:
    Renderer();
    ~Renderer();

    void updateCamera(const int deltaX, 
                      const int deltaY);
    void updateCameraZoom(const int dir);
    void preRender();
    void render();
    void postRender();
    void renderGUI();
    void resize(int width, 
                int height);

    void dispatch(
        const uint32_t shaderIdx,
        const bool     insertImageBarrier,
        const uint32_t workGroupX,
        const uint32_t workGroupY,
        const uint32_t workGroupZ)
    {
        mShaders[shaderIdx]->dispatch(insertImageBarrier, workGroupX, workGroupY, workGroupZ);
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

    bool loadTexture(
        std::unique_ptr<Texture> &tex,
        const bool               mipmap,
        const bool               alpha,
        const std::string        &fileName);

    bool loadModel(
        const std::string& fileName);

    // initialize uniform white noise [0, 1]
    void updateWaterGrid();
    void renderWater(const bool precompute);

    // methods for saving/loading settings
    void saveStates();
    void loadStates();

    // textures
    Texture3D                mCloudTexture;

    // shaders
    std::unordered_map<uint32_t, std::unique_ptr<ShaderProgram>> mShaders;
        
    // quad geometry
    Quad          mQuad;

    // resolution
    glm::vec2     mResolution;
    glm::vec2     mEnvironmentResolution;
    glm::vec2     mIrradianceResolution;
    glm::vec2     mPrefilterCubemapResolution;

    // multiplier for lower resolution for down-res background
    float         mLowResFactor;
    
    // update boolean
    bool          mUpdateSky;
    bool          mUpdateIrradiance;
    bool          mOceanWireframe;

    // bit flags to represent what sides of the cube are updated
    uint32_t      mIrradianceSideUpdated;
    uint32_t      mSkySideUpdated;
    uint32_t      mCloudNoiseUpdated;

    std::vector<std::unique_ptr<TimeQuery>> mTimeQueries;
    float         mShaderTimestamps[SHADER_COUNT];
    float         mTotalShaderTimes;

    // 2D textures to display
    std::vector<std::unique_ptr<RenderTexture>> mScreenRenderTextures;
    std::unique_ptr<RenderTexture> mWorleyNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mPerlinNoiseRenderTexture;
    std::unique_ptr<RenderTexture> mCloudNoiseRenderTexture[4];

    // foam textures
    std::unique_ptr<Texture> mOceanFoamTexture;
    std::unique_ptr<Texture> mBlueNoiseTexture;

    // environment cubemap texture
    std::unique_ptr<RenderCubemapTexture> mSkyCubemap;
    std::unique_ptr<RenderCubemapTexture> mFinalSkyCubemap;
    std::unique_ptr<RenderCubemapTexture> mIrradianceCubemap;
    std::unique_ptr<RenderCubemapTexture> mPrefilterCubemap;

    std::unique_ptr<Texture> mPrecomputedFresnelTexture;

    // states
    Camera mCamera;
    CameraParams mCamParams;
    CameraParams mPrecomputeCamParams;
    RendererParams mRenderParams;
    SkyParams mSkyParams;
    OceanParams mOceanParams;
    SceneObjectParams mSceneObjectParams;

    // shader buffers
    std::unique_ptr<ShaderBuffer> mModelMatsBuffer;
    std::unique_ptr<ShaderBuffer> mMaterialBuffer;

    // ocean geometry
    VertexBuffer mWaterGrid;
    std::vector<std::unique_ptr<VertexBuffer>> mDrawCalls;

    // noise 
    NoiseParams mWorleyNoiseParams;
    NoiseParams mPerlinNoiseParams;

    // ocean fft
    std::unique_ptr<OceanFFT> mOceanFFTHighRes;
    std::unique_ptr<OceanFFT> mOceanFFTMidRes;
    std::unique_ptr<OceanFFT> mOceanFFTLowRes;

    // gui
    bool mShowPropertiesWindow;
    bool mShowSkyWindow;
    bool mShowBuffersWindow;

    // booleans for rendering
    bool mRenderWater;

    // MVP matrix
    ViewProjectionMatrix mViewProjectionMat;
    ViewProjectionMatrix mPreviousViewProjectionMat;
    ViewProjectionMatrix mPrecomputeMatrix;
    std::vector<glm::mat4> mModelMats;

    // variables for recording time
    float mDeltaTime;
    float mTime;
    float mFrameTimes[FRAMETIMES_COUNT];
    float mFpsRecords[FRAMETIMES_COUNT];
    float mMinFps;
    float mMaxFps;
    uint32_t mFrameCount;
    std::chrono::steady_clock::time_point mRenderStartTime;
    std::chrono::steady_clock::time_point mRenderEndTime;

    // all uniform buffers
    std::map<uint32_t, std::unique_ptr<UniformBuffer>> mUniforms;

    // hosek sky model
    std::unique_ptr<Hosek> mHosekSkyModel;

    // clipmap for water plane
    Clipmap mClipmap;
    int mClipmapLevel;

    // generic textures
    uint32_t                 mEditingMaterialIdx;
    std::vector<std::string> mMaterialNames;
    std::vector<Material>    mMaterials;
    std::vector<std::unique_ptr<Texture>> mTextures;
};