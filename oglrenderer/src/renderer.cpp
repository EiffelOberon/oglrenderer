#include "renderer.h"

#include "freeglut.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"


Renderer::Renderer()
    : mPrerenderQuadShader("./spv/vert.spv", "./spv/frag.spv")
    , mTexturedQuadShader("./spv/vert.spv", "./spv/texturedQuadFrag.spv")
    , mFBMNoiseQuadShader("./spv/vert.spv", "./spv/fbmnoisefrag.spv")
    , mWorleyNoiseQuadShader("./spv/vert.spv", "./spv/worleynoisefrag.spv")
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mRenderTexture(nullptr)
    , mFBMNoiseRenderTexture(nullptr)
    , mWorleyNoiseRenderTexture(nullptr)
    , mCamera()
    , mShowPerformanceWindow(true)
    , mShowSkyWindow(true)
{
    // quad initialization
    mQuad.update(0, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0, 0));
    mQuad.update(1, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 1));
    mQuad.update(2, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0));
    mQuad.update(3, glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1, 1));
    mQuad.upload();

    // initialize uniforms for quad shader
    glm::mat4 orthogonalMatrix = glm::orthoLH(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    mPrerenderQuadShader.addUniform(ORTHO_MATRIX, orthogonalMatrix);

    // initialize scene camera
    glm::vec3 eye = mCamera.getEye();
    glm::vec3 target = mCamera.getTarget();
    glm::vec3 up = mCamera.getUp();
    mCamParams.mEye = glm::vec4(eye, 0.0f);
    mCamParams.mTarget = glm::vec4(target, 0.0f);
    mCamParams.mUp = glm::vec4(up, 0.0f);
    mPrerenderQuadShader.addUniform(CAMERA_PARAMS, mCamParams);

    // initialize sun
    mSkyParams.mSunDir = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
    mPrerenderQuadShader.addUniform(SKY_PARAMS, mSkyParams);

    // initialize renderer
    mRenderParams.mTime = 0.0f;

    // initialize noise
    mFBMNoiseParams.mSettings = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
    mFBMNoiseParams.mNoiseOctaves = 1;
    mFBMNoiseQuadShader.addUniform(FBM_PARAMS, mFBMNoiseParams);
    mWorleyNoiseParams.mSettings = glm::vec4(3.0f, 3.0f, 0.0f, 1.0f);
    mWorleyNoiseQuadShader.addUniform(WORLEY_PARAMS, mWorleyNoiseParams);
    mWorleyNoiseQuadShader.addUniform(RENDERER_PARAMS, mRenderParams);

    // for final resolution quad
    mTexturedQuadShader.addUniform(ORTHO_MATRIX, orthogonalMatrix);
    mFBMNoiseQuadShader.addUniform(ORTHO_MATRIX, orthogonalMatrix);
    mWorleyNoiseQuadShader.addUniform(ORTHO_MATRIX, orthogonalMatrix);

    mTime = 0.0f;
    mDeltaTime = 0.0f;
}

Renderer::~Renderer()
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

        // reallocate render texture
        mRenderTexture = std::make_unique<RenderTexture>(1, width * 0.25f, height * 0.25f);
        mFBMNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);
        mWorleyNoiseRenderTexture = std::make_unique<RenderTexture>(1, 100, 100);

        // update imgui display size
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        io.DisplaySize = ImVec2(width, height);
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

    const auto start = std::chrono::high_resolution_clock::now();

    mTime += (mDeltaTime);
    if (mTime > 60000.0f)
    {
        mTime = fmodf(mTime, 60000.0f);
    }
    mRenderParams.mTime = mTime;
    mWorleyNoiseQuadShader.updateUniform(RENDERER_PARAMS, 0, sizeof(float), mRenderParams);

    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render quarter sized render texture
    glViewport(0, 0, 100, 100);
    {
        mFBMNoiseRenderTexture->bind();
        mFBMNoiseQuadShader.use();
        mQuad.draw();
        mFBMNoiseQuadShader.disable();
        mFBMNoiseRenderTexture->unbind();

        mWorleyNoiseRenderTexture->bind();
        mWorleyNoiseQuadShader.use();
        mQuad.draw();
        mWorleyNoiseQuadShader.disable();
        mWorleyNoiseRenderTexture->unbind();
    }

    // render quarter sized render texture
    glViewport(0, 0, mResolution.x * 0.25f, mResolution.y * 0.25f);
    mRenderTexture->bind();
    mPrerenderQuadShader.use();
    mQuad.draw();
    mPrerenderQuadShader.disable();
    mRenderTexture->unbind();

    // render final quad
    glViewport(0, 0, mResolution.x, mResolution.y);
    mTexturedQuadShader.use();
    mRenderTexture->bindTexture2D(0);
    mQuad.draw();
    mTexturedQuadShader.disable();

    const auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> elapsed = (end - start);
    mDeltaTime = elapsed.count();
}


void Renderer::postRender()
{
    if (mShowPerformanceWindow)
    {
        ImGui::Begin("Performance", &mShowPerformanceWindow);
        ImGui::Text("Frame time: %f ms", mDeltaTime);
        ImGui::Text("Frames per sec: %f fps", (1.0f / mDeltaTime));
        ImGui::End();
    }


    if (mShowSkyWindow)
    {
        ImGui::Begin("Sky", &mShowSkyWindow);
        ImGui::Text("Sun Direction");
        if (ImGui::SliderFloat("x", &mSkyParams.mSunDir.x, -1.0f, 1.0f))
        {
            mPrerenderQuadShader.updateUniform<SkyParams>(SKY_PARAMS, mSkyParams);
        }

        if (ImGui::SliderFloat("y", &mSkyParams.mSunDir.y, 0.0f, 1.0f))
        {
            mPrerenderQuadShader.updateUniform<SkyParams>(SKY_PARAMS, mSkyParams);
        }

        if (ImGui::SliderFloat("z", &mSkyParams.mSunDir.z, -1.0f, 1.0f))
        {
            mPrerenderQuadShader.updateUniform<SkyParams>(SKY_PARAMS, mSkyParams);
        }

        // FBM
        ImTextureID fbmNoiseId = (ImTextureID) mFBMNoiseRenderTexture->getTextureId(0);
        float my_tex_w = 100;
        float my_tex_h = 100;
        ImGui::Text("Cloud noise (FBM): %.0fx%.0f", my_tex_w, my_tex_h);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(fbmNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        if (ImGui::SliderFloat("FBM width", &mFBMNoiseParams.mSettings.x, 0.1f, 4.0f))
        {
            mFBMNoiseQuadShader.updateUniform<NoiseParams>(FBM_PARAMS, mFBMNoiseParams);
        }
        if (ImGui::SliderFloat("FBM height", &mFBMNoiseParams.mSettings.y, 0.1f, 4.0f))
        {
            mFBMNoiseQuadShader.updateUniform<NoiseParams>(FBM_PARAMS, mFBMNoiseParams);
        }
        if (ImGui::SliderInt("octaves", &mFBMNoiseParams.mNoiseOctaves, 1, 8))
        {
            mFBMNoiseQuadShader.updateUniform<NoiseParams>(FBM_PARAMS, mFBMNoiseParams);
        }

        // Worley
        ImTextureID worleyNoiseId = (ImTextureID)mWorleyNoiseRenderTexture->getTextureId(0);
        ImGui::Text("Cloud noise (Worley): %.0fx%.0f", my_tex_w, my_tex_h);
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(worleyNoiseId, ImVec2(my_tex_w, my_tex_h), minUV, maxUV, tint, border);
        }
        if (ImGui::SliderFloat("Worley width", &mWorleyNoiseParams.mSettings.x, 1.0f, 10.0f))
        {
            mWorleyNoiseQuadShader.updateUniform<NoiseParams>(WORLEY_PARAMS, mWorleyNoiseParams);
        }
        if (ImGui::SliderFloat("Worley height", &mWorleyNoiseParams.mSettings.y, 1.0f, 10.0f))
        {
            mWorleyNoiseQuadShader.updateUniform<NoiseParams>(WORLEY_PARAMS, mWorleyNoiseParams);
        }
        ImGui::End();

    }

    bool test = true;
    ImGui::ShowDemoWindow(&test);
}