#include "renderer.h"

#include "freeglut.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"


Renderer::Renderer()
    : mPrecomputeCloudShader("./spv/precomputecloud.spv")
    , mPrerenderQuadShader("./spv/vert.spv", "./spv/frag.spv")
    , mTexturedQuadShader("./spv/vert.spv", "./spv/texturedQuadFrag.spv")
    , mCloudNoiseQuadShader("./spv/vert.spv", "./spv/cloudnoisefrag.spv")
    , mPerlinNoiseQuadShader("./spv/vert.spv", "./spv/perlinnoisefrag.spv")
    , mWorleyNoiseQuadShader("./spv/vert.spv", "./spv/worleynoisefrag.spv")
    , mCloudTexture(256, 256, 256, 32, false, CLOUD_TEXTURE)
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mRenderTexture(nullptr)
    , mCloudNoiseRenderTexture(nullptr)
    , mWorleyNoiseRenderTexture(nullptr)
    , mCamera()
    , mShowPerformanceWindow(true)
    , mShowSkyWindow(true)
    , mDeltaTime(0.0f)
    , mTime(0.0f)
    , mFrameCount(0)
{
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
    addUniform(RENDERER_PARAMS, mRenderParams);
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
        mRenderTexture = std::make_unique<RenderTexture>(1, width * 0.25f, height * 0.25f);
        mCloudNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);
        mWorleyNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);
        mPerlinNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);

        // update imgui display size
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.DisplaySize = ImVec2(width, height);
    }
}


void Renderer::preRender()
{
    mRenderStartTime = std::chrono::high_resolution_clock::now();

    if (mFrameCount % 16 == 0)
    {
        mCloudTexture.bind(false);
        const float workGroupSize = float(CLOUD_RESOLUTION) / float(PRECOMPUTE_CLOUD_LOCAL_SIZE);
        mPrecomputeCloudShader.dispatch(true, workGroupSize, workGroupSize, workGroupSize);
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

        mCloudNoiseRenderTexture->bind();
        mCloudNoiseQuadShader.use();
        mQuad.draw();
        mCloudNoiseQuadShader.disable();
        mCloudNoiseRenderTexture->unbind();
    }

    // render quarter sized render texture
    glViewport(0, 0, mResolution.x * 0.25f, mResolution.y * 0.25f);
    mRenderTexture->bind();
    mCloudTexture.bind();
    mPrerenderQuadShader.use();
    mQuad.draw();
    mPrerenderQuadShader.disable();
    mRenderTexture->unbind();

    // render final quad
    glViewport(0, 0, mResolution.x, mResolution.y);
    mTexturedQuadShader.use();
    mRenderTexture->bindTexture(0);
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
            updateUniform<SkyParams>(SKY_PARAMS, mSkyParams);
        }

        if (ImGui::SliderFloat("y", &mSkyParams.mSunDir.y, 0.0f, 1.0f))
        {
            updateUniform<SkyParams>(SKY_PARAMS, mSkyParams);
        }

        if (ImGui::SliderFloat("z", &mSkyParams.mSunDir.z, -1.0f, 1.0f))
        {
            updateUniform<SkyParams>(SKY_PARAMS, mSkyParams);
        }
        // FBM
        float my_tex_w = 100;
        float my_tex_h = 100;

        ImGui::Text("Cloud Noise: %.0fx%.0f", my_tex_w, my_tex_h);

        // Worley
        ImTextureID worleyNoiseId = (ImTextureID)mWorleyNoiseRenderTexture->getTextureId(0);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(worleyNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        ImGui::SameLine();

        ImTextureID perlinNoiseId = (ImTextureID)mPerlinNoiseRenderTexture->getTextureId(0);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(perlinNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        ImGui::SameLine();

        // Cloud
        ImTextureID cloudyNoiseId = (ImTextureID)mCloudNoiseRenderTexture->getTextureId(0);
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

        if (ImGui::SliderFloat("Perlin freq", &mPerlinNoiseParams.mSettings.z, 0.0f, 1.0f))
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
        if (ImGui::SliderFloat("Cloud density", &mRenderParams.mCloudSettings.z, 0.1f, 100.0f))
        {
            updateUniform(RENDERER_PARAMS, mRenderParams);
        }


        ImGui::End();

    }

    bool test = true;
    ImGui::ShowDemoWindow(&test);
}