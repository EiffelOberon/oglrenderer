#include "guirenderer.h"


#include "imgui.h"

#include "camera.h"
#include "params.h"
#include "object.h"
#include "oceanfft.h"
#include "renderer.h"
#include "shaderbuffer.h"
#include "shaderprogram.h"


GUIRenderer::GUIRenderer(
    Renderer* renderer)
    : mRenderer(renderer)
    , mEditingObject(nullptr)
    , mEditingMaterialIdx(0)
{

}


GUIRenderer::~GUIRenderer()
{

}


void GUIRenderer::renderMenu(
    Params &params)
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Main"))
        {
            if (ImGui::MenuItem("Load Settings"))
            {
                params.load();
                mRenderer->updateUniform(SKY_PARAMS, params.mSkyParams);
                mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                mRenderer->updateUniform(WORLEY_PARAMS, params.mWorleyNoiseParams);
                mRenderer->updateUniform(PERLIN_PARAMS, params.mPerlinNoiseParams);
                mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
            }
            if (ImGui::MenuItem("Save Settings"))
            {
                params.save();
            }
            if (ImGui::MenuItem("Exit"))
            {
                exit(0);
                return;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Buffers"))
            {
                params.mShowBuffersWindow = !params.mShowBuffersWindow;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}


void GUIRenderer::renderSkyWindow(
    Object                   *root,
    GLuint                   cloudTexIds[4],
    OceanFFT                 *oceanFFTHighRes,
    OceanFFT                 *oceanFFTMidRes,
    OceanFFT                 *oceanFFTLowRes,
    ShaderBuffer             *materialBuffer,
    std::vector<Material>    &materials,
    std::vector<std::string> &materialNames,
    Params                   &params)
{
    if (params.mShowSkyWindow)
    {
        ImGui::Begin("Environment", &params.mShowSkyWindow);
        if (ImGui::BeginTabBar("Settings", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Object"))
            {
                if (materialNames.size() > 0)
                {
                    static ImGuiTreeNodeFlags baseFlags =
                        ImGuiTreeNodeFlags_OpenOnArrow |
                        ImGuiTreeNodeFlags_OpenOnDoubleClick |
                        ImGuiTreeNodeFlags_SpanAvailWidth;

                    int idx = 0;
                    drawNodeTreeGUI(idx, root, root);

                    ImGui::Separator();

                    ImGui::Text("Object: %s", mEditingObject ? mEditingObject->name() : "-");
                    if (mEditingObject)
                    {
                        ImGui::Text("Position");
                        if (ImGui::DragFloat("x", &mEditingObject->transform()[3][0], 0.1f))
                        {
                            mRenderer->updateDrawCalls(root);
                        }
                        if (ImGui::DragFloat("y", &mEditingObject->transform()[3][1], 0.1f))
                        {
                            mRenderer->updateDrawCalls(root);
                        }
                        if (ImGui::DragFloat("z", &mEditingObject->transform()[3][2], 0.1f))
                        {
                            mRenderer->updateDrawCalls(root);
                        }
                    }
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sky"))
            {
                const static char* items[] = { "Nishita", "Hosek" };
                const char* comboLabel = items[params.mSkyParams.mPrecomputeSettings.y];  // Label to preview before opening the combo (technically it could be anything)
                if (ImGui::BeginCombo("Sky model", comboLabel))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                    {
                        const bool selected = (params.mSkyParams.mPrecomputeSettings.y == n);
                        if (ImGui::Selectable(items[n], selected))
                        {
                            params.mSkyParams.mPrecomputeSettings.y = n;
                            mRenderer->resetEnvironment();
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
                if (ImGui::SliderFloat("x", &params.mSkyParams.mSunSetting.x, -1.0f, 1.0f))
                {
                    mRenderer->resetEnvironment();
                }

                if (ImGui::SliderFloat("y", &params.mSkyParams.mSunSetting.y, 0.0f, 1.0f))
                {
                    mRenderer->resetEnvironment();
                }

                if (ImGui::SliderFloat("z", &params.mSkyParams.mSunSetting.z, -1.0f, 1.0f))
                {
                    mRenderer->resetEnvironment();
                }

                if (ImGui::SliderFloat("intensity", &params.mSkyParams.mSunSetting.w, 0.0f, 100.0f))
                {
                    mRenderer->resetEnvironment();
                }
                ImGui::Text("Sky");

                if (ImGui::SliderFloat("Rayleigh", &params.mSkyParams.mNishitaSetting.x, 0.0f, 40.0f))
                {
                    mRenderer->resetEnvironment();
                }

                if (ImGui::SliderFloat("Mie", &params.mSkyParams.mNishitaSetting.y, 0.0f, 40.0f))
                {
                    mRenderer->resetEnvironment();
                }

                if (ImGui::SliderFloat("Fog min dist", &params.mSkyParams.mFogSettings.x, 100.0f, 10000.0f))
                {
                    mRenderer->updateUniform(SKY_PARAMS, params.mSkyParams);
                }

                if (ImGui::SliderFloat("Fog max dist", &params.mSkyParams.mFogSettings.y, 200.0f, 10000.0f))
                {
                    mRenderer->updateUniform(SKY_PARAMS, params.mSkyParams);
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
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image((ImTextureID)cloudTexIds[0], ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }

                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image((ImTextureID)cloudTexIds[1], ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }
                ImGui::SameLine();

                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image((ImTextureID)cloudTexIds[2], ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }
                ImGui::SameLine();

                // Cloud (perlin worley)
                {
                    ImVec2 pos = ImGui::GetCursorScreenPos();
                    ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                    ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                    ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                    ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                    ImGui::Image((ImTextureID)cloudTexIds[3], ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                }
                if (ImGui::Checkbox("Worley invert", &params.mWorleyNoiseParams.mInvert))
                {
                    mRenderer->updateUniform(WORLEY_PARAMS, params.mWorleyNoiseParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderInt("Perlin octaves", &params.mPerlinNoiseParams.mNoiseOctaves, 1, 8))
                {
                    mRenderer->updateUniform(PERLIN_PARAMS, params.mPerlinNoiseParams);
                    mRenderer->resetEnvironment();
                }

                if (ImGui::SliderFloat("Perlin freq", &params.mPerlinNoiseParams.mSettings.z, 0.0f, 100.0f, " %.3f", ImGuiSliderFlags_Logarithmic))
                {
                    mRenderer->updateUniform(PERLIN_PARAMS, params.mPerlinNoiseParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Absorption", &params.mRenderParams.mCloudAbsorption.x, 0.0f, 1.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Anisotropy", &params.mRenderParams.mCloudSettings.x, -1.0f, 1.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Cloud speed", &params.mRenderParams.mCloudSettings.y, 0.0f, 1.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Cloud coverage", &params.mRenderParams.mCloudMapping.z, 0.0f, 1.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Cloud density", &params.mRenderParams.mCloudSettings.z, 0.0001f, 100.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Cloud BBox height", &params.mRenderParams.mCloudSettings.w, 100.0f, 100000.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Cloud UV width", &params.mRenderParams.mCloudMapping.x, 1.0f, 1000.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderFloat("Cloud UV height", &params.mRenderParams.mCloudMapping.y, 1.0f, 1000.0f))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderInt("Max steps", &params.mRenderParams.mSteps.x, 4, 1024))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                if (ImGui::SliderInt("Max shadow steps", &params.mRenderParams.mSteps.y, 2, 32))
                {
                    mRenderer->updateUniform(RENDERER_PARAMS, params.mRenderParams);
                    mRenderer->resetEnvironment();
                }
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Ocean"))
            {
                ImGui::Checkbox("Enabled", &params.mRenderWater);

                if (ImGui::ColorEdit3("Reflection", &params.mOceanParams.mReflection[0], ImGuiColorEditFlags_None))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::ColorEdit3("Transmission", &params.mOceanParams.mTransmission[0], ImGuiColorEditFlags_None))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::ColorEdit3("Transmission2", &params.mOceanParams.mTransmission2[0], ImGuiColorEditFlags_None))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Roughness", &params.mOceanParams.mFoamSettings.w, 0.01f, 1.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("IOR", &params.mOceanParams.mFoamSettings.z, 1.0f, 10.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Exponent", &params.mOceanParams.mTransmission2.w, 0.001f, 8.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Wave amplitude", &params.mOceanParams.mWaveSettings.x, 0.01f, 10.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Wind speed", &params.mOceanParams.mWaveSettings.y, 0.0f, 60.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat2("Wind direction", &params.mOceanParams.mWaveSettings.z, -1.0f, 1.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Dampening distance", &params.mOceanParams.mTransmission.w, 1000.0f, 8000.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Choppiness", &params.mOceanParams.mReflection.w, 1.0f, 10.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Foam scale", &params.mOceanParams.mFoamSettings.x, 1.0f, 1000.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                if (ImGui::SliderFloat("Foam intensity", &params.mOceanParams.mFoamSettings.y, 0.0f, 1.0f))
                {
                    mRenderer->updateUniform(OCEAN_PARAMS, params.mOceanParams);
                }
                ImGui::Checkbox("Wireframe", &params.mOceanWireframe);

                if (params.mRenderWater)
                {

                    const float textureWidth = 100;
                    const float textureHeight = 100;
                    ImGui::Text("Ocean spectrum: %.0fx%.0f", textureWidth, textureHeight);
                    ImTextureID oceanSpectrumTexId = (ImTextureID)oceanFFTHighRes->h0TexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanSpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                    ImTextureID oceanHDxSpectrumTexId = (ImTextureID)oceanFFTHighRes->dxTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanHDxSpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();

                    ImTextureID oceanHDySpectrumTexId = (ImTextureID)oceanFFTHighRes->dyTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanHDySpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();


                    ImTextureID oceanHDzSpectrumTexId = (ImTextureID)oceanFFTHighRes->dzTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(oceanHDzSpectrumTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                    ImTextureID butterflyTexId = (ImTextureID)oceanFFTHighRes->butterflyTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(butterflyTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }

                    ImTextureID displacementTexId = (ImTextureID)oceanFFTHighRes->displacementTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(displacementTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();
                    displacementTexId = (ImTextureID)oceanFFTMidRes->displacementTexId();
                    {
                        ImVec2 pos = ImGui::GetCursorScreenPos();
                        ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
                        ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
                        ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
                        ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
                        ImGui::Image(displacementTexId, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
                    }
                    ImGui::SameLine();
                    displacementTexId = (ImTextureID)oceanFFTLowRes->displacementTexId();
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
            if (ImGui::BeginTabItem("Material"))
            {
                if (materialNames.size() > 0)
                {
                    const char* comboLabel = materialNames[mEditingMaterialIdx].c_str();
                    if (ImGui::BeginCombo("material", comboLabel))
                    {
                        for (int n = 0; n < materialNames.size(); n++)
                        {
                            const bool selected = (mEditingMaterialIdx == n);
                            if (ImGui::Selectable(materialNames[n].c_str(), selected))
                            {
                                mEditingMaterialIdx = n;
                            }

                            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                            if (selected)
                            {
                                ImGui::SetItemDefaultFocus();
                            }
                        }
                        ImGui::EndCombo();
                    }

                    if (ImGui::SliderFloat("roughness", &materials[mEditingMaterialIdx].mShadingParams.x, 0.01f, 1.0f))
                    {
                        materialBuffer->update(
                            sizeof(Material) * mEditingMaterialIdx + offsetof(Material, mShadingParams),
                            sizeof(glm::vec4),
                            &materials[mEditingMaterialIdx].mShadingParams);
                    }
                    if (ImGui::SliderFloat("ior", &materials[mEditingMaterialIdx].mShadingParams.z, 1.0f, 10.0f))
                    {
                        materialBuffer->update(
                            sizeof(Material) * mEditingMaterialIdx + offsetof(Material, mShadingParams),
                            sizeof(glm::vec4),
                            &materials[mEditingMaterialIdx].mShadingParams);
                    }
                    if (ImGui::SliderFloat("metallic", &materials[mEditingMaterialIdx].mShadingParams.y, 0.0f, 1.0f))
                    {
                        materialBuffer->update(
                            sizeof(Material) * mEditingMaterialIdx + offsetof(Material, mShadingParams),
                            sizeof(glm::vec4),
                            &materials[mEditingMaterialIdx].mShadingParams);
                    }
                }
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }

        ImGui::End();

    }

}


void GUIRenderer::renderPropertiesWindow(
    Camera    &cam,
    float     deltaTime,
    float     *frameTimes,
    float     *fpsRecords,
    ShaderMap &shaderMap,
    float     *shaderTimestamps,
    float     totalShaderTimes,
    uint32_t  drawCallTriangleCount,
    uint32_t  waterTriangleCount,
    Params    &params)
{
    if (params.mShowPropertiesWindow)
    {
        ImGui::Begin("Properties", &params.mShowPropertiesWindow);
        if (ImGui::BeginTabBar("Settings", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Performance"))
            {
                ImGui::Text("Frame time: %f ms", deltaTime);
                {
                    ImGui::PlotLines("Time", &frameTimes[0], FRAMETIMES_COUNT, 0, 0, 0.0f, 30.0f, ImVec2(0, 80));
                }

                ImGui::Text("Frames per sec: %.2f fps", (1.0f / (deltaTime * 0.001f)));

                {
                    ImGui::PlotLines("FPS", &fpsRecords[0], FRAMETIMES_COUNT, 0, 0, 0.0f, 300.0f, ImVec2(0, 80));
                }
                ImGui::Separator();
                ImGui::Text("Statistics");
                const uint32_t sceneTriCount = drawCallTriangleCount;
                const uint32_t waterTriCount = params.mRenderWater ? waterTriangleCount : 0;
                const uint32_t totalTriCount = sceneTriCount + waterTriCount;
                ImGui::Text("scene tri-count: %d", sceneTriCount);
                ImGui::Text("water tri-count: %d", waterTriCount);
                ImGui::Text("total tri-count: %d", totalTriCount);
                ImGui::Separator();
                ImGui::Text("GPU time");
                char buf[32];
                for (int i = 0; i < SHADER_COUNT; ++i)
                {
                    if (shaderTimestamps[i] > 0.01f)
                    {
                        sprintf(buf, "%.2f ms", shaderTimestamps[i]);
                        ImGui::ProgressBar(shaderTimestamps[i] / totalShaderTimes, ImVec2(0.f, 0.f), buf);
                        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);

                        sprintf(buf, "%s", shaderMap[i]->name());
                        ImGui::Text(buf);
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Camera"))
            {
                ImGui::Text("Position");
                ImGui::Text("x: %.2f y: %.2f z: %.2f", cam.getEye().x, cam.getEye().y, cam.getEye().z);
                ImGui::Text("Target");
                ImGui::Text("x: %.2f y: %.2f z: %.2f", cam.getTarget().x, cam.getTarget().y, cam.getTarget().z);
                ImGui::Text("Distance");
                ImGui::Text("dist: %.2f", length(cam.getTarget() - cam.getEye()));
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
    }
}


void GUIRenderer::renderBuffersWindow(
    GLuint   screenRenderTextures[2],
    GLuint   precomputedFresnelTexture,
    uint32_t frameCount,
    Params   &params)
{
    if (params.mShowBuffersWindow)
    {
        ImGui::Begin("Buffers", &params.mShowBuffersWindow);

        // the buffer we are rendering to
        const float textureWidth = 100 * params.mRenderParams.mSettings.y;
        const float textureHeight = 100;
        ImGui::Text("Main pass");
        ImTextureID screenBuffer = (ImTextureID)screenRenderTextures[frameCount % SCREEN_BUFFER_COUNT];
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(screenBuffer, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
        }

        ImGui::SameLine();

        // the buffer in previous iteration
        screenBuffer = (ImTextureID)screenRenderTextures[(frameCount + 1) % SCREEN_BUFFER_COUNT];
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(screenBuffer, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
        }

        ImGui::Text("Fresnel");

        screenBuffer = (ImTextureID)precomputedFresnelTexture;
        {
            ImVec2 pos = ImGui::GetCursorScreenPos();
            ImVec2 minUV = ImVec2(0.0f, 0.0f);              // Top-left
            ImVec2 maxUV = ImVec2(1.0f, 1.0f);              // Lower-right
            ImVec4 tint = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
            ImVec4 border = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
            ImGui::Image(screenBuffer, ImVec2(textureWidth, textureHeight), minUV, maxUV, tint, border);
        }

        ImGui::End();
    }

}


void GUIRenderer::drawNodeTreeGUI(
    int    &idx,
    Object *root,
    Object *node)
{
    // base flags for tree nodes
    static ImGuiTreeNodeFlags base_flags =
        ImGuiTreeNodeFlags_Framed |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    static int selectionMask = (1 << 2);
    ImGuiTreeNodeFlags nodeFlags = base_flags;
    if (node == mEditingObject)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Selected;
    }

    if (node->childCount() == 0)
    {
        nodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx((void*)(intptr_t)idx, nodeFlags, node->name());

        // check if item is clicked
        if (ImGui::IsItemClicked())
        {
            mEditingObject = node;
        }
        ++idx;
        if (ImGui::BeginDragDropSource())
        {
            ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
            ImGui::Text("This is a drag and drop source");
            ImGui::EndDragDropSource();
        }
    }
    else
    {
        // only render non root components
        nodeFlags |= (node == root || node->parent() == root) ? ImGuiTreeNodeFlags_DefaultOpen : 0;
        const bool nodeOpen =  ImGui::TreeNodeEx((void*)(intptr_t)idx, nodeFlags, node->name());

        // check if item is clicked
        if (ImGui::IsItemClicked())
        {
            mEditingObject = node;
        }
        ++idx;

        if (nodeOpen)
        {
            for (int i = 0; i < node->childCount(); ++i)
            {
                Object* child = node->child(i);
                drawNodeTreeGUI(idx, root, child);
            }

            ImGui::TreePop();
        }
    }
}

