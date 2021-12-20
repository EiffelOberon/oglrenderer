#include "renderer.h"

#include <random>

#include "freeglut.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "ini.h"


Renderer::Renderer()
    : mButterflyOpShader("./spv/butterflyoperation.spv")
    , mInversionShader("./spv/inversion.spv")
    , mPrecomputeButterflyTexShader("./spv/precomputebutterfly.spv")
    , mPrecomputeCloudShader("./spv/precomputecloud.spv")
    , mPrecomputeEnvironmentShader("./spv/vert.spv", "./spv/precomputeenvironment.spv")
    , mPrecomputeOceanH0Shader("./spv/oceanheightfield.spv")
    , mPrecomputeOceanHShader("./spv/oceanhfinal.spv")
    , mPrerenderQuadShader("./spv/vert.spv", "./spv/frag.spv")
    , mTexturedQuadShader("./spv/vert.spv", "./spv/texturedQuadFrag.spv")
    , mCloudNoiseQuadShader("./spv/vert.spv", "./spv/cloudnoisefrag.spv")
    , mPerlinNoiseQuadShader("./spv/vert.spv", "./spv/perlinnoisefrag.spv")
    , mWorleyNoiseQuadShader("./spv/vert.spv", "./spv/worleynoisefrag.spv")
    , mCloudTexture(CLOUD_RESOLUTION, CLOUD_RESOLUTION, CLOUD_RESOLUTION, 32, false)
    , mOceanFFT(OCEAN_RESOLUTION)
    , mOceanDisplacementTexture(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_LINEAR, 32, false)
    , mOceanH0SpectrumTexture(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_NEAREST, 32, false)
    , mOceanHDxSpectrumTexture(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_NEAREST, 32, false)
    , mOceanHDySpectrumTexture(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_NEAREST, 32, false)
    , mOceanHDzSpectrumTexture(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_NEAREST, 32, false)
    , mOceanNoiseTexture(nullptr)
    , mPingPongTexture(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_NEAREST, 32, false)
    , mButterFlyTexture((int)(log(float(OCEAN_RESOLUTION)) / log(2.0f)), OCEAN_RESOLUTION, GL_NEAREST, 32, false, nullptr)
    , mButterflyIndicesBuffer(OCEAN_RESOLUTION * sizeof(int))
    , mEnvironmentResolution(2048.0f, 2048.0f)
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mRenderTexture(nullptr)
    , mRenderCubemapTexture(nullptr)
    , mWorleyNoiseRenderTexture(nullptr)
    , mCamera()
    , mShowOceanWindow(true)
    , mShowPerformanceWindow(true)
    , mShowSkyWindow(true)
    , mUpdateEnvironment(true)
    , mDeltaTime(0.0f)
    , mLowResFactor(0.5f)
    , mTime(0.0f)
    , mFrameCount(0)
    , mWaterGrid()
{
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
    mSkyParams.mSunDir = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    mSkyParams.mPrecomputeSettings.x = 0;
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
    mOceanParams.mHeightSettings = glm::ivec4(OCEAN_RESOLUTION, 1024, 0, 0);
    mOceanParams.mPingPong = glm::ivec4(0, 0, 0, 0);
    mOceanParams.mWaveSettings = glm::vec4(4.0f, 40.0f, 1.0f, 1.0f);
    addUniform(OCEAN_PARAMS, mOceanParams);

    loadStates();

    // cubemap environment
    mRenderCubemapTexture = std::make_unique<RenderCubemapTexture>(mEnvironmentResolution.x);

    // ocean related noise texture and other shader buffers
    updateOceanNoiseTexture();
    mButterflyIndicesBuffer.upload(mOceanFFT.bitReversedIndices());

    // compute butterfly indices
    mButterflyIndicesBuffer.bind(BUTTERFLY_INDICES);
    mButterFlyTexture.bindImageTexture(PRECOMPUTE_BUTTERFLY_OUTPUT, GL_WRITE_ONLY);
    const int workGroupSize = int(float(OCEAN_RESOLUTION) / float(PRECOMPUTE_OCEAN_WAVES_LOCAL_SIZE));
    mPrecomputeButterflyTexShader.dispatch(true, mOceanFFT.passes(), workGroupSize, 1);

    updateWaterGrid();
}

Renderer::~Renderer()
{

}


void Renderer::updateCamera(
    const int deltaX, 
    const int deltaY)
{
    mCamera.update(deltaX, deltaY);
    mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mCamParams);
}


void Renderer::updateOceanNoiseTexture()
{
    float* randomNumbers = new float[OCEAN_RESOLUTION * OCEAN_RESOLUTION * 4];

    for (int i = 0; i < OCEAN_RESOLUTION * OCEAN_RESOLUTION * 4; ++i)
    {
        randomNumbers[i] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    }

    mOceanNoiseTexture = std::make_unique<Texture>(OCEAN_RESOLUTION, OCEAN_RESOLUTION, GL_NEAREST, 32, false, randomNumbers);
    delete[] randomNumbers;
}


void Renderer::updateWaterGrid()
{

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
    }
}


void Renderer::preRender()
{
    mRenderStartTime = std::chrono::high_resolution_clock::now();

    if (mFrameCount % 16 == 0)
    {
        mCloudTexture.bindImageTexture(PRECOMPUTE_CLOUD_CLOUD_TEX, GL_WRITE_ONLY);
        const int workGroupSize = int(float(CLOUD_RESOLUTION) / float(PRECOMPUTE_CLOUD_LOCAL_SIZE));
        mPrecomputeCloudShader.dispatch(true, workGroupSize, workGroupSize, workGroupSize);

        // render quarter sized render texture
        glViewport(0, 0, 100, 100);
        {
            mPerlinNoiseRenderTexture->bind();
            mPerlinNoiseQuadShader.use();
            mQuad.draw();
            mPerlinNoiseQuadShader.disable();
            mPerlinNoiseRenderTexture->unbind();

            mWorleyNoiseRenderTexture->bind();
            mWorleyNoiseQuadShader.use();
            mQuad.draw();
            mWorleyNoiseQuadShader.disable();
            mWorleyNoiseRenderTexture->unbind();

            for (int i = 0; i < 4; ++i)
            {
                mWorleyNoiseParams.mTextureIdx = i;
                updateUniform(WORLEY_PARAMS, mWorleyNoiseParams);

                mCloudTexture.bindTexture(CLOUD_NOISE_CLOUD_TEX);
                mCloudNoiseRenderTexture[i]->bind();
                mCloudNoiseQuadShader.use();
                mQuad.draw();
                mCloudNoiseQuadShader.disable();
                mCloudNoiseRenderTexture[i]->unbind();
            }
        }
    }

    // ocean waves precomputation
    if(mOceanNoiseTexture != nullptr)
    {
        const int workGroupSize = int(float(OCEAN_RESOLUTION) / float(PRECOMPUTE_OCEAN_WAVES_LOCAL_SIZE));
        // pass 1
        mOceanNoiseTexture->bindTexture(OCEAN_HEIGHTFIELD_NOISE);
        mOceanH0SpectrumTexture.bindImageTexture(OCEAN_HEIGHTFIELD_H0K, GL_WRITE_ONLY);
        mPrecomputeOceanH0Shader.dispatch(true, workGroupSize, workGroupSize, 1);

        // pass 2
        mOceanH0SpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H0K, GL_READ_ONLY);
        mOceanHDxSpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H_X, GL_WRITE_ONLY);
        mOceanHDySpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H_Y, GL_WRITE_ONLY);
        mOceanHDzSpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H_Z, GL_WRITE_ONLY);
        mPrecomputeOceanHShader.dispatch(true, workGroupSize, workGroupSize, 1);

        mButterFlyTexture.bindImageTexture(BUTTERFLY_INPUT_TEX, GL_READ_ONLY);
        mOceanHDySpectrumTexture.bindImageTexture(BUTTERFLY_PINGPONG_TEX0, GL_READ_WRITE);
        mPingPongTexture.bindImageTexture(BUTTERFLY_PINGPONG_TEX1, GL_READ_WRITE);

        for (int i = 0; i < mOceanFFT.passes(); ++i)
        {
            mOceanParams.mPingPong.y = i;
            mOceanParams.mPingPong.z = 0;
            updateUniform(OCEAN_PARAMS, offsetof(OceanParams, mPingPong), sizeof(glm::ivec4), mOceanParams.mPingPong);
            
            mButterflyOpShader.dispatch(true, workGroupSize, workGroupSize, 1);

            mOceanParams.mPingPong.x++;
            mOceanParams.mPingPong.x = mOceanParams.mPingPong.x % 2;
        }

        for (int i = 0; i < mOceanFFT.passes(); ++i)
        {
            mOceanParams.mPingPong.y = i;
            mOceanParams.mPingPong.z = 1;
            updateUniform(OCEAN_PARAMS, offsetof(OceanParams, mPingPong), sizeof(glm::ivec4), mOceanParams.mPingPong);
            
            mButterflyOpShader.dispatch(true, workGroupSize, workGroupSize, 1);

            mOceanParams.mPingPong.x++;
            mOceanParams.mPingPong.x = mOceanParams.mPingPong.x % 2;
        }

        updateUniform(OCEAN_PARAMS, offsetof(OceanParams, mPingPong), sizeof(glm::ivec4), mOceanParams.mPingPong);
        mOceanHDySpectrumTexture.bindImageTexture(INVERSION_PINGPONG_TEX0, GL_READ_ONLY);
        mPingPongTexture.bindImageTexture(INVERSION_PINGPONG_TEX1, GL_READ_ONLY);
        mOceanDisplacementTexture.bindImageTexture(INVERSION_OUTPUT_TEX, GL_WRITE_ONLY);
        mInversionShader.dispatch(true, workGroupSize, workGroupSize, 1);
    }

    // render quarter sized render texture
    if (mUpdateEnvironment)
    {
        glViewport(0, 0, int(mEnvironmentResolution.x), int(mEnvironmentResolution.y));
        mPrecomputeEnvironmentShader.use();
        for (int i = 0; i < 6; ++i)
        {
            mSkyParams.mPrecomputeSettings.x = i;
            updateUniform(SKY_PARAMS, mSkyParams);

            mRenderCubemapTexture->bind(i);
            mQuad.draw();
        }
        mPrecomputeEnvironmentShader.disable();
        mRenderCubemapTexture->unbind();
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

    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render quarter sized render texture
    glViewport(0, 0, mResolution.x * mLowResFactor, mResolution.y * mLowResFactor);
    mRenderTexture->bind();
    mCloudTexture.bindTexture(QUAD_CLOUD_TEX);
    mRenderCubemapTexture->bindTexture(QUAD_ENV_TEX, 0);
    mPrerenderQuadShader.use();
    mQuad.draw();
    mPrerenderQuadShader.disable();
    mRenderTexture->unbind();

    // render final quad
    glViewport(0, 0, int(mResolution.x), int(mResolution.y));
    mTexturedQuadShader.use();
    mRenderTexture->bindTexture(SCREEN_QUAD_TEX, 0);
    mQuad.draw();
    mTexturedQuadShader.disable();
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
        ImGui::Begin("Sky", &mShowSkyWindow);
        ImGui::Text("Sun Direction");
        if (ImGui::SliderFloat("x", &mSkyParams.mSunDir.x, -1.0f, 1.0f))
        {
            mUpdateEnvironment = true;
        }

        if (ImGui::SliderFloat("y", &mSkyParams.mSunDir.y, 0.0f, 1.0f))
        {
            mUpdateEnvironment = true;
        }

        if (ImGui::SliderFloat("z", &mSkyParams.mSunDir.z, -1.0f, 1.0f))
        {
            mUpdateEnvironment = true;
        }
        // FBM
        float my_tex_w = 100;
        float my_tex_h = 100;

        ImGui::Text("Cloud Noise: %.0fx%.0f", my_tex_w, my_tex_h);

        // Worley
        ImTextureID cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[0]->getTextureId(0);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(cloudyNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }

        cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[1]->getTextureId(0);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(cloudyNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        ImGui::SameLine();

        cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture[2]->getTextureId(0);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(cloudyNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
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
            ImGui::Image(cloudyNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        if (ImGui::Checkbox("Worley invert", &mWorleyNoiseParams.mInvert))
        {
            updateUniform(WORLEY_PARAMS, mWorleyNoiseParams);
        }
        if (ImGui::SliderInt("Perlin octaves", &mPerlinNoiseParams.mNoiseOctaves, 1, 8))
        {
            updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
        }

        if (ImGui::SliderFloat("Perlin freq", &mPerlinNoiseParams.mSettings.z, 0.0f, 100.0f, " %.3f", ImGuiSliderFlags_Logarithmic))
        {
            updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
        }
        if (ImGui::SliderFloat("Cloud cutoff", &mRenderParams.mCloudSettings.x, 0.0f, 1.0f))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }
        if (ImGui::SliderFloat("Cloud speed", &mRenderParams.mCloudSettings.y, 0.0f, 1.0f))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }
        if (ImGui::SliderFloat("Cloud density", &mRenderParams.mCloudSettings.z, 0.0001f, 100.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }
        if (ImGui::SliderFloat("Cloud BBox height", &mRenderParams.mCloudSettings.w, 100.0f, 100000.0f))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }
        if (ImGui::SliderFloat("Cloud UV width", &mRenderParams.mCloudMapping.x, 1.0f, 1000.0f))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }
        if (ImGui::SliderFloat("Cloud UV height", &mRenderParams.mCloudMapping.y, 1.0f, 1000.0f))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }
        if (ImGui::SliderInt("Max steps", &mRenderParams.mSteps.x, 4, 1024))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }

        if (ImGui::SliderInt("Max shadow steps", &mRenderParams.mSteps.y, 2, 32))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }


        ImGui::End();

    }

    // ocean window
    if (mShowOceanWindow)
    {
        ImGui::Begin("Ocean", &mShowOceanWindow);

        if (ImGui::SliderInt("Patch dimension", &mOceanParams.mHeightSettings.y, 64, 1024))
        {
            updateUniform(OCEAN_PARAMS, mOceanParams);
        }
        if (ImGui::SliderFloat("Wave amplitude", &mOceanParams.mWaveSettings.x, 0.0f, 8.0f))
        {
            updateUniform(OCEAN_PARAMS, mOceanParams);
        }
        if (ImGui::SliderFloat("Wind speed", &mOceanParams.mWaveSettings.y, 0.0f, 80.0f))
        {
            updateUniform(OCEAN_PARAMS, mOceanParams);
        }
        if (ImGui::SliderFloat2("Wind direction", &mOceanParams.mWaveSettings.z, -1.0f, 1.0f))
        {
            updateUniform(OCEAN_PARAMS, mOceanParams);
        }

        float my_tex_w = 100;
        float my_tex_h = 100;
        ImGui::Text("Ocean spectrum: %.0fx%.0f", my_tex_w, my_tex_h);
        ImTextureID oceanSpectrumTexId = (ImTextureID)mOceanH0SpectrumTexture.texId();
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(oceanSpectrumTexId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }

        ImTextureID oceanHDxSpectrumTexId = (ImTextureID)mOceanHDxSpectrumTexture.texId();
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(oceanHDxSpectrumTexId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        ImGui::SameLine();

        ImTextureID oceanHDySpectrumTexId = (ImTextureID)mOceanHDySpectrumTexture.texId();
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(oceanHDySpectrumTexId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        ImGui::SameLine();


        ImTextureID oceanHDzSpectrumTexId = (ImTextureID)mOceanHDzSpectrumTexture.texId();
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(oceanHDzSpectrumTexId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }

        ImTextureID butterflyTexId = (ImTextureID)mButterFlyTexture.texId();
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(butterflyTexId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }

        ImTextureID displacementTexId = (ImTextureID)mOceanDisplacementTexture.texId();
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(displacementTexId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }

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

    ini["skyparams"]["x"] = std::to_string(mSkyParams.mSunDir.x);
    ini["skyparams"]["y"] = std::to_string(mSkyParams.mSunDir.y);
    ini["skyparams"]["z"] = std::to_string(mSkyParams.mSunDir.z);

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
        std::string& amountOfApples = ini["fruits"]["apples"];
        mSkyParams.mSunDir.x = std::stof(ini["skyparams"]["x"]);
        mSkyParams.mSunDir.y = std::stof(ini["skyparams"]["y"]);
        mSkyParams.mSunDir.z = std::stof(ini["skyparams"]["z"]);

        updateUniform(SKY_PARAMS, mSkyParams);

        mRenderParams.mCloudSettings.x = std::stof(ini["renderparams"]["cutoff"]);
        mRenderParams.mCloudSettings.y = std::stof(ini["renderparams"]["speed"]);
        mRenderParams.mCloudSettings.z = std::stof(ini["renderparams"]["density"]);
        mRenderParams.mCloudSettings.w = std::stof(ini["renderparams"]["height"]);
        mRenderParams.mCloudMapping.x = std::stof(ini["renderparams"]["cloudu"]);
        mRenderParams.mCloudMapping.y = std::stof(ini["renderparams"]["cloudv"]);

        mRenderParams.mSteps.x = std::stoi(ini["renderparams"]["maxsteps"]);
        mRenderParams.mSteps.y = std::stoi(ini["renderparams"]["maxshadowsteps"]);

        updateUniform(RENDERER_PARAMS, mRenderParams);

        mWorleyNoiseParams.mInvert = bool(std::stoi(ini["worleyparams"]["invert"]));

        updateUniform(WORLEY_PARAMS, mWorleyNoiseParams);

        mPerlinNoiseParams.mNoiseOctaves = std::stoi(ini["perlinparams"]["octaves"]);
        mPerlinNoiseParams.mSettings.z = std::stof(ini["perlinparams"]["frequency"]);

        updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
    }
}