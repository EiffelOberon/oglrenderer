#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

#include "glm/glm.hpp"

#include "devicestructs.h"

class Camera;
class Params;
class Object;
class OceanFFT;
class Renderer;
class ShaderBuffer;
class ShaderProgram;

typedef  std::unordered_map<uint32_t, std::unique_ptr<ShaderProgram>> ShaderMap;


class GUIRenderer
{
public:
    GUIRenderer(
        Renderer *renderer);
    ~GUIRenderer();

    void renderMenu(
        Params& params);

    void renderSkyWindow(
        Object                   *root,
        GLuint                   cloudTexIds[4],
        OceanFFT                 *oceanFFTHighRes,
        OceanFFT                 *oceanFFTMidRes,
        OceanFFT                 *oceanFFTLowRes,
        ShaderBuffer             *materialBuffer,
        std::vector<Material>    &materials,
        std::vector<std::string> &materialNames,
        Params                   &params);

    void renderPropertiesWindow(
        Camera    &cam,
        float     deltaTime,
        float     *frameTimes,
        float     *fpsRecords,
        ShaderMap &shaderMap,
        float     *shaderTimestamps,
        float     totalShaderTimes,
        uint32_t  drawCallTriangleCount,
        uint32_t  waterTriangleCount,
        Params    &params);

    void renderBuffersWindow(
        GLuint   screenRenderTextures[2], 
        GLuint   precomputedFresnelTexture,
        uint32_t frameCount,
        Params   &params);

    void drawNodeTreeGUI(
        int    &idx,
        Object *root,
        Object *node);

private:
    Renderer *mRenderer;
    Object   *mEditingObject;
    uint32_t mEditingMaterialIdx;
};