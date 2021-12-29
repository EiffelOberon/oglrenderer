#include "renderer.h"

#include <random>

#include "freeglut.h"
#include "FreeImage/FreeImage.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "ini.h"
#include "oceanfft.h"


Renderer::Renderer()
    : mCloudTexture(CLOUD_RESOLUTION, CLOUD_RESOLUTION, CLOUD_RESOLUTION, 32, false)
    , mOceanFFTHighRes(nullptr)
    , mOceanFFTMidRes(nullptr)
    , mOceanFFTLowRes(nullptr)
    , mOceanFoamTexture(nullptr)
    , mEnvironmentResolution(1024, 1024)
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mClipmap(6)
    , mClipmapLevel(0)
    , mRenderTexture(nullptr)
    , mRenderCubemapTexture(nullptr)
    , mWorleyNoiseRenderTexture(nullptr)
    , mCamera()
    , mShowPropertiesWindow(true)
    , mShowPerformanceWindow(true)
    , mShowSkyWindow(true)
    , mOceanWireframe(false)
    , mUpdateEnvironment(true)
    , mRenderWater(true)
    , mDeltaTime(0.0f)
    , mLowResFactor(0.5f)
    , mTime(0.0f)
    , mFrameCount(0)
    , mWaterGrid()
{
    mShaders[BUTTERFLY_SHADER] = std::make_unique<ShaderProgram>("./spv/butterflyoperation.spv");
    mShaders[INVERSION_SHADER] = std::make_unique<ShaderProgram>("./spv/inversion.spv");
    mShaders[PRECOMP_BUTTERFLY_SHADER] = std::make_unique<ShaderProgram>("./spv/precomputebutterfly.spv");
    mShaders[PRECOMP_CLOUD_SHADER] = std::make_unique<ShaderProgram>("./spv/precomputecloud.spv");
    mShaders[PRECOMP_ENV_SHADER] = std::make_unique<ShaderProgram>("./spv/vert.spv", "./spv/precomputeenvironment.spv");
    mShaders[PRECOMP_OCEAN_H0_SHADER] = std::make_unique<ShaderProgram>("./spv/oceanheightfield.spv");
    mShaders[PRECOMP_OCEAN_H_SHADER] = std::make_unique<ShaderProgram>("./spv/oceanhfinal.spv");
    mShaders[PRE_RENDER_QUAD_SHADER] = std::make_unique<ShaderProgram>("./spv/vert.spv", "./spv/frag.spv");
    mShaders[TEXTURED_QUAD_SHADER] = std::make_unique<ShaderProgram>("./spv/vert.spv", "./spv/texturedQuadFrag.spv");
    mShaders[CLOUD_NOISE_SHADER] = std::make_unique<ShaderProgram>("./spv/vert.spv", "./spv/cloudnoisefrag.spv");
    mShaders[PERLIN_NOISE_SHADER] = std::make_unique<ShaderProgram>("./spv/vert.spv", "./spv/perlinnoisefrag.spv");
    mShaders[WORLEY_NOISE_SHADER] = std::make_unique<ShaderProgram>("./spv/vert.spv", "./spv/worleynoisefrag.spv");
    mShaders[WATER_SHADER] = std::make_unique<ShaderProgram>("./spv/watervert.spv", "./spv/waterfrag.spv");

    // cloud noise textures
    mCloudNoiseRenderTexture[0] = nullptr;
    mCloudNoiseRenderTexture[1] = nullptr;
    mCloudNoiseRenderTexture[2] = nullptr;
    mCloudNoiseRenderTexture[3] = nullptr;

    // quad initialization
    mQuad.update(0, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0, 0));
    mQuad.update(1, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 1));
    mQuad.update(2, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0));
    mQuad.update(3, glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1, 1));
    mQuad.upload();

    // initialize uniforms for quad shader
    glm::mat4 orthogonalMatrix = glm::orthoLH(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    addUniform(ORTHO_MATRIX, orthogonalMatrix);

    // initialize scene camera
    glm::vec3 eye = mCamera.getEye();
    glm::vec3 target = mCamera.getTarget();
    glm::vec3 up = mCamera.getUp();
    mCamParams.mEye = glm::vec4(eye, 0.0f);
    mCamParams.mTarget = glm::vec4(target, 0.0f);
    mCamParams.mUp = glm::vec4(up, 0.0f);
    addUniform(CAMERA_PARAMS, mCamParams);

    // initialize sun
    mSkyParams.mSunSetting = glm::vec4(0.0f, 1.0f, 0.0f, 20.0f);
    mSkyParams.mNishitaSetting = glm::vec4(20.0f, 20.0f, 0.0f, 0.0f);
    mSkyParams.mFogSettings = glm::vec4(3000.0f, 5000.0f, 0.0f, 0.0f);
    mSkyParams.mPrecomputeSettings.x = 0;
    mSkyParams.mPrecomputeSettings.y = 0;
    addUniform(SKY_PARAMS, mSkyParams);

    // initialize noise
    mPerlinNoiseParams.mSettings = glm::vec4(1.0f, 1.0f, 0.4f, 1.0f);
    mPerlinNoiseParams.mNoiseOctaves = 7;
    addUniform(PERLIN_PARAMS, mPerlinNoiseParams);
    mWorleyNoiseParams.mSettings = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    mWorleyNoiseParams.mInvert   = true;
    addUniform(WORLEY_PARAMS, mWorleyNoiseParams);

    // initialize render params
    mRenderParams.mSettings.x = 0.0f;
    mRenderParams.mSettings.y = (1600.0f / 900.0f);
    mRenderParams.mCloudSettings.x = 0.4f;
    mRenderParams.mCloudSettings.y = 0.01f;
    mRenderParams.mCloudSettings.z = 1.0f;
    mRenderParams.mCloudSettings.w = 10000.0f;
    mRenderParams.mCloudMapping.x = 4.0f;
    mRenderParams.mCloudMapping.y = 4.0f;
    mRenderParams.mSteps.x = 1024;
    mRenderParams.mSteps.y = 8;
    addUniform(RENDERER_PARAMS, mRenderParams);

    // initialize ocean params
    mOceanParams.mHeightSettings = glm::ivec4(OCEAN_RESOLUTION_1, OCEAN_DIMENSIONS_1, 0, 0);
    mOceanParams.mPingPong = glm::ivec4(0, 0, 0, 0);
    mOceanParams.mWaveSettings = glm::vec4(4.0f, 40.0f, 1.0f, 1.0f);
    mOceanParams.mReflection = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    mOceanParams.mTransmission = glm::vec4(0.0f, 0.0f, 1.0f, 2000.0f);
    mOceanParams.mTransmission2 = glm::vec4(0.0f, 0.0f, 1.0f, 4.0f);
    mOceanParams.mFoamSettings = glm::vec4(1.0f, 0.7f, 0.0f, 0.0f);
    addUniform(OCEAN_PARAMS, mOceanParams);

    loadStates();

    // cubemap environment
    mRenderCubemapTexture = std::make_unique<RenderCubemapTexture>(mEnvironmentResolution.x);

    // ocean related noise texture and other shader buffers
    mOceanFFTHighRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_1, OCEAN_DIMENSIONS_1);
    mOceanFFTMidRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_2, OCEAN_DIMENSIONS_2);
    mOceanFFTLowRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_3, OCEAN_DIMENSIONS_3);

    // compute water geometry
    //updateWaterGrid();
    mClipmap.generateGeometry();

    glm::mat4 projMatrix = glm::perspective(glm::radians(60.0f), 1600.0f / 900.0f, 0.1f, 10000.0f);
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mMVPMatrix.mProjectionMatrix = projMatrix;
    mMVPMatrix.mViewMatrix = viewMatrix;
    addUniform(MVP_MATRIX, mMVPMatrix);

    // hosek
    mHosekSkyModel = std::make_unique<Hosek>(glm::vec3(mSkyParams.mSunSetting.x, mSkyParams.mSunSetting.y, mSkyParams.mSunSetting.z), 512);

    // load textures
    FreeImage_Initialise();
    bool result = loadFoam(mOceanFoamTexture, "./resources/foamDiffuse.jpg");
    assert(result);
    FreeImage_DeInitialise();
}

Renderer::~Renderer()
{

}


bool Renderer::loadFoam(
    std::unique_ptr<Texture> &tex,
    const std::string        &fileName)
{
    FIBITMAP* dib(0);

    //check the file signature and deduce its format
    FREE_IMAGE_FORMAT fif = (FREE_IMAGE_FORMAT) FreeImage_GetFileType(fileName.c_str(), 0);
    //if still unknown, try to guess the file format from the file extension
    if (fif == FIF_UNKNOWN)
    {
        fif = FreeImage_GetFIFFromFilename(fileName.c_str());
    }
    if (fif == FIF_UNKNOWN)
    {
        assert(false);
        return false;
    }

    //check that the plugin has reading capabilities and load the file
    if (FreeImage_FIFSupportsReading(fif))
    {
        dib = FreeImage_Load(fif, fileName.c_str());
    }

    if (!dib)
    {
        assert(false);
        return false;
    }
    //retrieve the image data
    BYTE* bits = FreeImage_GetBits(dib);
    uint32_t width = FreeImage_GetWidth(dib);
    uint32_t height = FreeImage_GetHeight(dib);
    if ((bits == 0) || (width == 0) || (height == 0))
    {
        assert(false);
        return false;
    }

    tex = std::make_unique<Texture>((int)width, (int)height, GL_LINEAR_MIPMAP_LINEAR, true, 8, false, false, (void*)bits);
    tex->generateMipmap();
    FreeImage_Unload(dib);
    return true;
}



void Renderer::updateCamera(
    const int deltaX, 
    const int deltaY)
{
    mCamera.update(deltaX * 2.0f * M_PI / mResolution.x, deltaY * M_PI / mResolution.y);
    mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mCamParams);

    // update MVP
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mMVPMatrix.mViewMatrix = viewMatrix;
    updateUniform(MVP_MATRIX, mMVPMatrix);
}


void Renderer::updateCameraZoom(
    const int dir)
{
    mCamera.updateZoom(dir);
    mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mCamParams);

    // update MVP
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mMVPMatrix.mViewMatrix = viewMatrix;
    updateUniform(MVP_MATRIX, mMVPMatrix);
}


void Renderer::updateWaterGrid()
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    int count = 0;
    // patch count along each axis (square)
    int columnCount = (pow(2,9)) - 1; 
    float dimension = (pow(2,11)) - 1;
    float offset = dimension / columnCount;
    for (int row = 0; row < columnCount; ++row)
    {
        for (int column = 0; column < columnCount; ++column)
        {
            Vertex v;
            v.mPosition = glm::vec3(column * offset - dimension * 0.5f, 0.0f, row * offset - dimension * 0.5f);
            v.mNormal = glm::vec3(0, 1, 0);
            v.mUV = glm::vec2(column / float(columnCount - 1), row / float(columnCount - 1));
            vertices.push_back(v);
        }
    }

    for (int row = 0; row < (columnCount-1); ++row)
    {
        for (int column = 0; column < (columnCount-1); ++column)
        {
            indices.push_back(row * columnCount + column);
            indices.push_back(row * columnCount + (column + 1));
            indices.push_back((row + 1) * columnCount + column);

            indices.push_back((row + 1) * columnCount + column);
            indices.push_back(row * columnCount + (column + 1));
            indices.push_back((row + 1) * columnCount + (column + 1));
        }
    }

    mWaterGrid.update(vertices.size() * sizeof(Vertex), indices.size() * sizeof(uint32_t), vertices.data(), indices.data());
}


void Renderer::resize(
    int width, 
    int height)
{
    if ((std::abs(mResolution.x - width) > 0.1f) || (std::abs(mResolution.y - height) > 0.1f))
    {
        // update resolution
        mResolution = glm::vec2(width, height);
        mRenderParams.mSettings.y = (mResolution.x / mResolution.y);
        updateUniform(RENDERER_PARAMS, mRenderParams);

        // reallocate render texture
        mRenderTexture = std::make_unique<RenderTexture>(1, width * mLowResFactor, height * mLowResFactor);
        mCloudNoiseRenderTexture[0] = std::make_unique<RenderTexture>(1, 100, 100);
        mCloudNoiseRenderTexture[1] = std::make_unique<RenderTexture>(1, 100, 100);
        mCloudNoiseRenderTexture[2] = std::make_unique<RenderTexture>(1, 100, 100);
        mCloudNoiseRenderTexture[3] = std::make_unique<RenderTexture>(1, 100, 100);
        mWorleyNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);
        mPerlinNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);
        
        // update imgui display size
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.DisplaySize = ImVec2(float(width), float(height));

        // update MVP
        glm::mat4 perspectiveMatrix = glm::perspective(glm::radians(60.0f), mResolution.x / mResolution.y, 0.1f, 10000.0f);
        mMVPMatrix.mProjectionMatrix = perspectiveMatrix;
        updateUniform(MVP_MATRIX, mMVPMatrix);
    }
}


void Renderer::preRender()
{
    mRenderStartTime = std::chrono::high_resolution_clock::now();

    if (mFrameCount % 16 == 0)
    {
        mCloudTexture.bindImageTexture(PRECOMPUTE_CLOUD_CLOUD_TEX, GL_WRITE_ONLY);
        const int workGroupSize = int(float(CLOUD_RESOLUTION) / float(PRECOMPUTE_CLOUD_LOCAL_SIZE));
        mShaders[PRECOMP_CLOUD_SHADER]->dispatch(true, workGroupSize, workGroupSize, workGroupSize);

        // render quarter sized render texture
        glViewport(0, 0, 100, 100);
        {

            mPerlinNoiseRenderTexture->bind();
            mShaders[PERLIN_NOISE_SHADER]->use();
            mQuad.draw();
            mShaders[PERLIN_NOISE_SHADER]->disable();
            mPerlinNoiseRenderTexture->unbind();

            mWorleyNoiseRenderTexture->bind();
            mShaders[WORLEY_NOISE_SHADER]->use();
            mQuad.draw();
            mShaders[WORLEY_NOISE_SHADER]->disable();
            mWorleyNoiseRenderTexture->unbind();

            for (int i = 0; i < 4; ++i)
            {
                mWorleyNoiseParams.mTextureIdx = i;
                updateUniform(WORLEY_PARAMS, mWorleyNoiseParams);

                mCloudTexture.bindTexture(CLOUD_NOISE_CLOUD_TEX);
                mCloudNoiseRenderTexture[i]->bind();
                mShaders[CLOUD_NOISE_SHADER]->use();
                mQuad.draw();
                mShaders[CLOUD_NOISE_SHADER]->disable();
                mCloudNoiseRenderTexture[i]->unbind();
            }
        }
    }

    // ocean waves precomputation
    if (mRenderWater)
    {
        mOceanFFTHighRes->precompute(*this, mOceanParams);
        mOceanFFTMidRes->precompute(*this, mOceanParams);
        mOceanFFTLowRes->precompute(*this, mOceanParams);
    }

    // render quarter sized render texture
    if (mUpdateEnvironment)
    {
        glViewport(0, 0, int(mEnvironmentResolution.x), int(mEnvironmentResolution.y));

        if (mSkyParams.mPrecomputeSettings.y == NISHITA_SKY)
        {
            mCloudTexture.bindTexture(PRECOMPUTE_ENVIRONENT_CLOUD_TEX);
            mShaders[PRECOMP_ENV_SHADER]->use();
            for (int i = 0; i < 6; ++i)
            {
                mSkyParams.mPrecomputeSettings.x = i;
                updateUniform(SKY_PARAMS, mSkyParams);

                mRenderCubemapTexture->bind(i);
                mQuad.draw();
            }
            mShaders[PRECOMP_ENV_SHADER]->disable();
            mRenderCubemapTexture->unbind();
        }
        else if(mSkyParams.mPrecomputeSettings.y == HOSEK_SKY)
        {
            updateUniform(SKY_PARAMS, mSkyParams);
            mHosekSkyModel->update(glm::vec3(mSkyParams.mSunSetting.x, mSkyParams.mSunSetting.y, mSkyParams.mSunSetting.z));
        }
        mUpdateEnvironment = false;
    }
}


void Renderer::render()
{
    if (!mRenderTexture ||
        mResolution.x <= 0.1f ||
        mResolution.y <= 0.1f)
    {
        return;
    }

    mRenderParams.mSettings.x = mTime * 0.001f;
    updateUniform(RENDERER_PARAMS, 0, sizeof(float), mRenderParams);

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render quarter sized render texture
    glViewport(0, 0, mResolution.x * mLowResFactor, mResolution.y * mLowResFactor);
    mRenderTexture->bind();
    mCloudTexture.bindTexture(QUAD_CLOUD_TEX);
    switch (mSkyParams.mPrecomputeSettings.y)
    {
    case NISHITA_SKY: mRenderCubemapTexture->bindTexture(QUAD_ENV_TEX, 0); break;
    case HOSEK_SKY:   mHosekSkyModel->bind(QUAD_ENV_TEX);                  break;
    }
    mShaders[PRE_RENDER_QUAD_SHADER]->use();
    mQuad.draw();
    mShaders[PRE_RENDER_QUAD_SHADER]->disable();
    mRenderTexture->unbind();

    // render final quad
    glViewport(0, 0, int(mResolution.x), int(mResolution.y));
    mShaders[TEXTURED_QUAD_SHADER]->use();
    mRenderTexture->bindTexture(SCREEN_QUAD_TEX, 0);
    mQuad.draw();
    mShaders[TEXTURED_QUAD_SHADER]->disable();
    
    // enable depth mask
    glEnable (GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    if (mRenderWater)
    {
        mShaders[WATER_SHADER]->use();
        switch (mSkyParams.mPrecomputeSettings.y)
        {
        case NISHITA_SKY: mRenderCubemapTexture->bindTexture(WATER_ENV_TEX, 0); break;
        case HOSEK_SKY:   mHosekSkyModel->bind(WATER_ENV_TEX);                  break;
        }
        mOceanFFTHighRes->bind(WATER_DISPLACEMENT1_TEX);
        mOceanFFTMidRes->bind(WATER_DISPLACEMENT2_TEX);
        mOceanFFTLowRes->bind(WATER_DISPLACEMENT3_TEX);
        mOceanFoamTexture->bindTexture(WATER_FOAM_TEX);

        if (mOceanWireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        mClipmap.draw();
        if (mOceanWireframe)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        mShaders[WATER_SHADER]->disable();
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
}


void Renderer::postRender()
{
    mRenderEndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> elapsed = (mRenderEndTime - mRenderStartTime);
    mDeltaTime = elapsed.count();

    mTime += (mDeltaTime);
    if (mTime > 3600000.0f)
    {
        mTime = fmodf(mTime, 3600000.0f);
    }

    ++mFrameCount;
    if (mFrameCount >= 1000000)
    {
        mFrameCount = 0;
    }
}


void Renderer::renderGUI()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Main"))
        {
            if (ImGui::MenuItem("Load Settings"))
            {
                loadStates();
            }
            if (ImGui::MenuItem("Save Settings"))
            {
                saveStates();
            }
            if (ImGui::MenuItem("Exit"))
            {
                exit(0);
                return;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }


    if (mShowPerformanceWindow)
    {
        ImGui::Begin("Performance", &mShowPerformanceWindow);
        ImGui::Text("Frame time: %f ms", mDeltaTime);
        ImGui::Text("Frames per sec: %f fps", (1.0f / (mDeltaTime * 0.001f)));
        ImGui::End();
    }


    if (mShowSkyWindow)
    {
        ImGui::Begin("Environment", &mShowSkyWindow);
        if (ImGui::BeginTabBar("Settings", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Sky"))
            {
                const static char* items[] = { "Nishita", "Hosek" };
                const char* comboLabel = items[mSkyParams.mPrecomputeSettings.y];  // Label to preview before opening the combo (technically it could be anything)
                if (ImGui::BeginCombo("Sky model", comboLabel))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                    {
                        const bool selected = (mSkyParams.mPrecomputeSettings.y == n);
                        if (ImGui::Selectable(items[n], selected))
                        {
                            mSkyParams.mPrecomputeSettings.y = n;
                            mUpdateEnvironment = true;
                        }

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (selected)
                        {
                            ImGui::SetItemDefaultFocus();
                        }
                    }
                    ImGui::EndCombo();
                }

                ImGui::Text("Sun direction");
                if (ImGui::SliderFloat("x", &mSkyParams.mSunSetting.x, -1.0f, 1.0f))
                {
                    mUpdateEnvironment = true;
                }

                if (ImGui::SliderFloat("y", &mSkyParams.mSunSetting.y, 0.0f, 1.0f))
                {
                    mUpdateEnvironment = true;
                }

                if (ImGui::SliderFloat("z", &mSkyParams.mSunSetting.z, -1.0f, 1.0f))
                {
                    mUpdateEnvironment = true;
                }

                if (ImGui::SliderFloat("intensity", &mSkyParams.mSunSetting.w, 0.0f, 100.0f))
                {
                    mUpdateEnvironment = true;
                }
                ImGui::Text("Sky");

                if (ImGui::SliderFloat("Rayleigh", &mSkyParams.mNishitaSetting.x, 0.0f, 40.0f))
                {
                    mUpdateEnvironment = true;
                }

                if (ImGui::SliderFloat("Mie", &mSkyParams.mNishitaSetting.y, 0.0f, 40.0f))
                {
                    mUpdateEnvironment = true;
                }
                
                if (ImGui::SliderFloat("Fog min dist", &mSkyParams.mFogSettings.x, 100.0f, 10000.0f))
                {
                    updateUniform(SKY_PARAMS, mSkyParams);
                }

                if (ImGui::SliderFloat("Fog max dist", &mSkyParams.mFogSettings.y, 200.0f, 10000.0f))
                {
                    updateUniform(SKY_PARAMS, mSkyParams);
                }

                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Cloud"))
            {
                // FBM
                const float textureWidth = 100;
                const float textureHeight = 100;

                ImGui::NewLine();
                ImGui::Text("Cloud Noise: %.0fx%.0f", textureWidth, textureHeight);

                // Worley
                ImTextureID cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[0]->getTextureId(0);
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image(cloudyNoiseId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }

                cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[1]->getTextureId(0);
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image(cloudyNoiseId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }
                ImGui::SameLine();

                cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[2]->getTextureId(0);
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image(cloudyNoiseId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }
                ImGui::SameLine();

                // Cloud (perlin worley)
                cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[3]->getTextureId(0);
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image(cloudyNoiseId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }
                if (ImGui::Checkbox("Worley invert", &mWorleyNoiseParams.mInvert))
                {
                    updateUniform(WORLEY_PARAMS, mWorleyNoiseParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderInt("Perlin octaves", &mPerlinNoiseParams.mNoiseOctaves, 1, 8))
                {
                    updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
                    mUpdateEnvironment = true;
                }

                if (ImGui::SliderFloat("Perlin freq", &mPerlinNoiseParams.mSettings.z, 0.0f, 100.0f, " %.3f", ImGuiSliderFlags_Logarithmic))
                {
                    updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderFloat("Cloud cutoff", &mRenderParams.mCloudSettings.x, 0.0f, 1.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderFloat("Cloud speed", &mRenderParams.mCloudSettings.y, 0.0f, 1.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderFloat("Cloud density", &mRenderParams.mCloudSettings.z, 0.0001f, 100.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderFloat("Cloud BBox height", &mRenderParams.mCloudSettings.w, 100.0f, 100000.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderFloat("Cloud UV width", &mRenderParams.mCloudMapping.x, 1.0f, 1000.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderFloat("Cloud UV height", &mRenderParams.mCloudMapping.y, 1.0f, 1000.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                if (ImGui::SliderInt("Max steps", &mRenderParams.mSteps.x, 4, 1024))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }

                if (ImGui::SliderInt("Max shadow steps", &mRenderParams.mSteps.y, 2, 32))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateEnvironment = true;
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Ocean"))
            {
                ImGui::Checkbox("Enabled", &mRenderWater);

                if (ImGui::ColorEdit3("Reflection", &mOceanParams.mReflection[0], ImGuiColorEditFlags_None))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::ColorEdit3("Transmission", &mOceanParams.mTransmission[0], ImGuiColorEditFlags_None))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::ColorEdit3("Transmission2", &mOceanParams.mTransmission2[0], ImGuiColorEditFlags_None))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Exponent", &mOceanParams.mTransmission2.w, 0.001f, 8.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Wave amplitude", &mOceanParams.mWaveSettings.x, 0.01f, 10.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Wind speed", &mOceanParams.mWaveSettings.y, 0.0f, 60.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat2("Wind direction", &mOceanParams.mWaveSettings.z, -1.0f, 1.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Dampening distance", &mOceanParams.mTransmission.w, 1000.0f, 8000.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Choppiness", &mOceanParams.mReflection.w, 1.0f, 10.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Foam scale", &mOceanParams.mFoamSettings.x, 1.0f, 1000.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                if (ImGui::SliderFloat("Foam intensity", &mOceanParams.mFoamSettings.y, 0.0f, 1.0f))
                {
                    updateUniform(OCEAN_PARAMS, mOceanParams);
                }
                ImGui::Checkbox("Wireframe", &mOceanWireframe);

                if (mRenderWater)
                {

                    const float textureWidth = 100;
                    const float textureHeight = 100;
                    ImGui::Text("Ocean spectrum: %.0fx%.0f", textureWidth, textureHeight);
                    ImTextureID oceanSpectrumTexId = (ImTextureID)mOceanFFTHighRes->h0TexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanSpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                    ImTextureID oceanHDxSpectrumTexId = (ImTextureID)mOceanFFTHighRes->dxTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanHDxSpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();

                    ImTextureID oceanHDySpectrumTexId = (ImTextureID)mOceanFFTHighRes->dyTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanHDySpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();


                    ImTextureID oceanHDzSpectrumTexId = (ImTextureID)mOceanFFTHighRes->dzTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanHDzSpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                    ImTextureID butterflyTexId = (ImTextureID)mOceanFFTHighRes->butterflyTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(butterflyTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                    ImTextureID displacementTexId = (ImTextureID)mOceanFFTHighRes->displacementTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(displacementTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();
                    displacementTexId = (ImTextureID)mOceanFFTMidRes->displacementTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(displacementTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();
                    displacementTexId = (ImTextureID)mOceanFFTLowRes->displacementTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(displacementTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }


        ImGui::End();

    }

    // ocean window
    if (mShowPropertiesWindow)
    {
        ImGui::Begin("Properties", &mShowPropertiesWindow);
        ImGui::Text("Camera");
        ImGui::NewLine();
        ImGui::Text("Position");
        ImGui::Text("x: %.2f y: %.2f z: %.2f", mCamera.getEye().x, mCamera.getEye().y, mCamera.getEye().z);
        ImGui::Text("Target");
        ImGui::Text("x: %.2f y: %.2f z: %.2f", mCamera.getTarget().x, mCamera.getTarget().y, mCamera.getTarget().z);
        ImGui::Text("Distance");
        ImGui::Text("dist: %.2f", length(mCamera.getTarget() - mCamera.getEye()));
        ImGui::End();
    }


    bool test = true;
    ImGui::ShowDemoWindow(&test);
}


void Renderer::saveStates()
{
    // create a file instance
    mINI::INIFile file("oglrenderer.ini");
    mINI::INIStructure ini;

    ini["skyparams"]["x"] = std::to_string(mSkyParams.mSunSetting.x);
    ini["skyparams"]["y"] = std::to_string(mSkyParams.mSunSetting.y);
    ini["skyparams"]["z"] = std::to_string(mSkyParams.mSunSetting.z);
    ini["skyparams"]["sunintensity"] = std::to_string(mSkyParams.mSunSetting.w);
    ini["skyparams"]["nishitarayleigh"] = std::to_string(mSkyParams.mNishitaSetting.x);
    ini["skyparams"]["nishitamie"] = std::to_string(mSkyParams.mNishitaSetting.y);
    ini["skyparams"]["fogmin"] = std::to_string(mSkyParams.mFogSettings.x);
    ini["skyparams"]["fogmax"] = std::to_string(mSkyParams.mFogSettings.y);

    ini["renderparams"]["cutoff"] = std::to_string(mRenderParams.mCloudSettings.x);
    ini["renderparams"]["speed"] = std::to_string(mRenderParams.mCloudSettings.y);
    ini["renderparams"]["density"] = std::to_string(mRenderParams.mCloudSettings.z);
    ini["renderparams"]["height"] = std::to_string(mRenderParams.mCloudSettings.w);
    ini["renderparams"]["cloudu"] = std::to_string(mRenderParams.mCloudMapping.x);
    ini["renderparams"]["cloudv"] = std::to_string(mRenderParams.mCloudMapping.y);
    ini["renderparams"]["maxsteps"] = std::to_string(mRenderParams.mSteps.x);
    ini["renderparams"]["maxshadowsteps"] = std::to_string(mRenderParams.mSteps.y);

    ini["perlinparams"]["frequency"] = std::to_string(mPerlinNoiseParams.mSettings.z);
    ini["perlinparams"]["octaves"] = std::to_string(mPerlinNoiseParams.mNoiseOctaves);
    ini["worleyparams"]["invert"] = std::to_string(mWorleyNoiseParams.mInvert);

    ini["oceanparams"]["enabled"] = std::to_string((int)mRenderWater);

    ini["oceanparams"]["reflectionX"] = std::to_string(mOceanParams.mReflection.x);
    ini["oceanparams"]["reflectionY"] = std::to_string(mOceanParams.mReflection.y);
    ini["oceanparams"]["reflectionZ"] = std::to_string(mOceanParams.mReflection.z);
    ini["oceanparams"]["choppiness"] = std::to_string(mOceanParams.mReflection.w);

    ini["oceanparams"]["transmissionX"] = std::to_string(mOceanParams.mTransmission.x);
    ini["oceanparams"]["transmissionY"] = std::to_string(mOceanParams.mTransmission.y);
    ini["oceanparams"]["transmissionZ"] = std::to_string(mOceanParams.mTransmission.z);
    ini["oceanparams"]["dampeningdistance"] = std::to_string(mOceanParams.mTransmission.w);

    ini["oceanparams"]["transmission2X"] = std::to_string(mOceanParams.mTransmission2.x);
    ini["oceanparams"]["transmission2Y"] = std::to_string(mOceanParams.mTransmission2.y);
    ini["oceanparams"]["transmission2Z"] = std::to_string(mOceanParams.mTransmission2.z);
    ini["oceanparams"]["exponent"] = std::to_string(mOceanParams.mTransmission2.w);

    ini["oceanparams"]["amplitude"] = std::to_string(mOceanParams.mWaveSettings.x);
    ini["oceanparams"]["speed"] = std::to_string(mOceanParams.mWaveSettings.y);
    ini["oceanparams"]["dirX"] = std::to_string(mOceanParams.mWaveSettings.z);
    ini["oceanparams"]["dirY"] = std::to_string(mOceanParams.mWaveSettings.w);

    ini["oceanparams"]["foamscale"] = std::to_string(mOceanParams.mFoamSettings.x);
    ini["oceanparams"]["foamintensity"] = std::to_string(mOceanParams.mFoamSettings.y);

    // generate an INI file (overwrites any previous file)
    file.generate(ini);
}


void Renderer::loadStates()
{
    // read file into struct
    mINI::INIFile file("oglrenderer.ini");
    mINI::INIStructure ini;
    if (file.read(ini))
    {
        // read a value
        if (ini.has("skyparams"))
        {
            mSkyParams.mSunSetting.x = std::stof(ini["skyparams"]["x"]);
            mSkyParams.mSunSetting.y = std::stof(ini["skyparams"]["y"]);
            mSkyParams.mSunSetting.z = std::stof(ini["skyparams"]["z"]);
            mSkyParams.mSunSetting.w = std::stof(ini["skyparams"]["sunintensity"]);
            mSkyParams.mNishitaSetting.x = std::stof(ini["skyparams"]["nishitarayleigh"]);
            mSkyParams.mNishitaSetting.y = std::stof(ini["skyparams"]["nishitamie"]);
            mSkyParams.mFogSettings.x = std::stof(ini["skyparams"]["fogmin"]);
            mSkyParams.mFogSettings.y = std::stof(ini["skyparams"]["fogmax"]);
        }

        updateUniform(SKY_PARAMS, mSkyParams);

        if (ini.has("renderparams"))
        {
            mRenderParams.mCloudSettings.x = std::stof(ini["renderparams"]["cutoff"]);
            mRenderParams.mCloudSettings.y = std::stof(ini["renderparams"]["speed"]);
            mRenderParams.mCloudSettings.z = std::stof(ini["renderparams"]["density"]);
            mRenderParams.mCloudSettings.w = std::stof(ini["renderparams"]["height"]);
            mRenderParams.mCloudMapping.x = std::stof(ini["renderparams"]["cloudu"]);
            mRenderParams.mCloudMapping.y = std::stof(ini["renderparams"]["cloudv"]);

            mRenderParams.mSteps.x = std::stoi(ini["renderparams"]["maxsteps"]);
            mRenderParams.mSteps.y = std::stoi(ini["renderparams"]["maxshadowsteps"]);
        }

        updateUniform(RENDERER_PARAMS, mRenderParams);

        if (ini.has("worleyparams"))
        {
            mWorleyNoiseParams.mInvert = bool(std::stoi(ini["worleyparams"]["invert"]));
        }

        updateUniform(WORLEY_PARAMS, mWorleyNoiseParams);

        if (ini.has("perlinparams"))
        {
            mPerlinNoiseParams.mNoiseOctaves = std::stoi(ini["perlinparams"]["octaves"]);
            mPerlinNoiseParams.mSettings.z = std::stof(ini["perlinparams"]["frequency"]);
        }

        updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);

        if(ini.has("oceanparams"))
        {
            mRenderWater = std::stoi(ini["oceanparams"]["enabled"]);

            mOceanParams.mReflection.x = std::stof(ini["oceanparams"]["reflectionX"]);
            mOceanParams.mReflection.y = std::stof(ini["oceanparams"]["reflectionY"]);
            mOceanParams.mReflection.z = std::stof(ini["oceanparams"]["reflectionZ"]);
            mOceanParams.mReflection.w = std::stof(ini["oceanparams"]["choppiness"]);

            mOceanParams.mTransmission.x = std::stof(ini["oceanparams"]["transmissionX"]);
            mOceanParams.mTransmission.y = std::stof(ini["oceanparams"]["transmissionY"]);
            mOceanParams.mTransmission.z = std::stof(ini["oceanparams"]["transmissionZ"]);
            mOceanParams.mTransmission.w = std::stof(ini["oceanparams"]["dampeningdistance"]);
            
            mOceanParams.mTransmission2.x = std::stof(ini["oceanparams"]["transmission2X"]);
            mOceanParams.mTransmission2.y = std::stof(ini["oceanparams"]["transmission2Y"]);
            mOceanParams.mTransmission2.z = std::stof(ini["oceanparams"]["transmission2Z"]);
            mOceanParams.mTransmission2.w = std::stof(ini["oceanparams"]["exponent"]);

            mOceanParams.mWaveSettings.x = std::stof(ini["oceanparams"]["amplitude"]);
            mOceanParams.mWaveSettings.y = std::stof(ini["oceanparams"]["speed"]);
            mOceanParams.mWaveSettings.z = std::stof(ini["oceanparams"]["dirX"]);
            mOceanParams.mWaveSettings.w = std::stof(ini["oceanparams"]["dirY"]);

            mOceanParams.mFoamSettings.x = std::stof(ini["oceanparams"]["foamscale"]);
            mOceanParams.mFoamSettings.y = std::stof(ini["oceanparams"]["foamintensity"]);
        }

        updateUniform(OCEAN_PARAMS, mOceanParams);
    }
}