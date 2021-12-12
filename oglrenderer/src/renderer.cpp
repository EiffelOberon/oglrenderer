#include "renderer.h"

#include "deviceconstants.h" 
#include "devicestructs.h"
#include "freeglut.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"


Renderer::Renderer()
    : mPrerenderQuadShader("./spv/vert.spv", "./spv/frag.spv")
    , mTexturedQuadShader("./spv/vert.spv", "./spv/texturedQuadFrag.spv")
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mRenderTexture(nullptr)
    , mCamera()
{
    // quad initialization
    mQuad.update(0, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec2(0, 0));
    mQuad.update(1, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0, 1));
    mQuad.update(2, glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1, 0));
    mQuad.update(3, glm::vec3(1.0f, 1.0f, 0.0f), glm::vec2(1, 1));
    mQuad.upload();

    // initialize uniforms for quad shader
    glm::mat4 orthogonalMatrix = glm::orthoLH(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
    mPrerenderQuadShader.addUniform<glm::mat4>(ORTHO_MATRIX, orthogonalMatrix);

    // initialize scene camera
    glm::vec3 eye = mCamera.getEye();
    glm::vec3 target = mCamera.getTarget();
    glm::vec3 up = mCamera.getUp();
    CameraParams camParams;
    camParams.mEye = glm::vec4(eye, 0.0f);
    camParams.mTarget = glm::vec4(target, 0.0f);
    camParams.mUp = glm::vec4(up, 0.0f);
    mPrerenderQuadShader.addUniform<CameraParams>(CAMERA_PARAMS, camParams);

    // for final resolution quad
    mTexturedQuadShader.addUniform<glm::mat4>(ORTHO_MATRIX, orthogonalMatrix);
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


    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
}


void Renderer::postRender()
{

}