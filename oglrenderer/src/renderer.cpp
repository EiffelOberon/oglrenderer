#include "renderer.h"

#include <random>

#define TINYOBJLOADER_IMPLEMENTATION
#include "freeglut.h"
#include "FreeImage/FreeImage.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
#include "ini.h"
#include "tinyobjloader/tiny_obj_loader.h"

#include "nishita.h"
#include "oceanfft.h"


Renderer::Renderer()
    : mCloudTexture(CLOUD_RESOLUTION, CLOUD_RESOLUTION, CLOUD_RESOLUTION, 32, false)
    , mOceanFFTHighRes(nullptr)
    , mOceanFFTMidRes(nullptr)
    , mOceanFFTLowRes(nullptr)
    , mOceanFoamTexture(nullptr)
    , mEnvironmentResolution(ENVIRONMENT_RESOLUTION, ENVIRONMENT_RESOLUTION)
    , mIrradianceResolution(IRRADIANCE_RESOLUTION, IRRADIANCE_RESOLUTION)
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mClipmap(6)
    , mClipmapLevel(0)
    , mSkyCubemap(nullptr)
    , mFinalSkyCubemap(nullptr)
    , mIrradianceCubemap(nullptr)
    , mWorleyNoiseRenderTexture(nullptr)
    , mCamera()
    , mShowBuffersWindow(false)
    , mShowPropertiesWindow(true)
    , mShowSkyWindow(true)
    , mOceanWireframe(false)
    , mUpdateSky(true)
    , mUpdateIrradiance(true)
    , mCloudNoiseUpdated(0)
    , mIrradianceSideUpdated(0)
    , mSkySideUpdated(0)
    , mRenderWater(true)
    , mDeltaTime(0.0f)
    , mLowResFactor(0.5f)
    , mTime(0.0f)
    , mTotalShaderTimes(0.0f)
    , mFrameCount(0)
    , mWaterGrid()
    , mMinFps(FLT_MAX)
    , mMaxFps(FLT_MIN)
{
    mTimeQueries.push_back(std::make_unique<TimeQuery>(SHADER_COUNT));
    mTimeQueries.push_back(std::make_unique<TimeQuery>(SHADER_COUNT));

    mShaders[BUTTERFLY_SHADER] = std::make_unique<ShaderProgram>("butterfly", "./spv/butterflyoperation.spv");
    mShaders[INVERSION_SHADER] = std::make_unique<ShaderProgram>("fft", "./spv/inversion.spv");
    mShaders[PRECOMP_BUTTERFLY_SHADER] = std::make_unique<ShaderProgram>("precomputebutterfly", "./spv/precomputebutterfly.spv");
    mShaders[PRECOMP_CLOUD_SHADER] = std::make_unique<ShaderProgram>("precomputecloud", "./spv/precomputecloud.spv");
    mShaders[PRECOMP_ENV_SHADER] = std::make_unique<ShaderProgram>("precomputeenv", "./spv/vert.spv", "./spv/precomputeenvironment.spv");
    mShaders[PRECOMP_SKY_SHADER] = std::make_unique<ShaderProgram>("precomputesky", "./spv/vert.spv", "./spv/precomputesky.spv");
    mShaders[PRECOMP_OCEAN_H0_SHADER] = std::make_unique<ShaderProgram>("oceanspectrum", "./spv/oceanheightfield.spv");
    mShaders[PRECOMP_OCEAN_H_SHADER] = std::make_unique<ShaderProgram>("heightf", "./spv/oceanhfinal.spv");
    mShaders[PRE_RENDER_QUAD_SHADER] = std::make_unique<ShaderProgram>("quad", "./spv/vert.spv", "./spv/frag.spv");
    mShaders[TEXTURED_QUAD_SHADER] = std::make_unique<ShaderProgram>("post", "./spv/vert.spv", "./spv/texturedQuadFrag.spv");
    mShaders[CLOUD_NOISE_SHADER] = std::make_unique<ShaderProgram>("cloudnoise", "./spv/vert.spv", "./spv/cloudnoisefrag.spv");
    mShaders[PERLIN_NOISE_SHADER] = std::make_unique<ShaderProgram>("perlin", "./spv/vert.spv", "./spv/perlinnoisefrag.spv");
    mShaders[WORLEY_NOISE_SHADER] = std::make_unique<ShaderProgram>("worley", "./spv/vert.spv", "./spv/worleynoisefrag.spv");
    mShaders[WATER_SHADER] = std::make_unique<ShaderProgram>("water", "./spv/watervert.spv", "./spv/waterfrag.spv");
    mShaders[TEMPORAL_QUAD_SHADER] = std::make_unique<ShaderProgram>("temporal", "./spv/temporalvert.spv", "./spv/temporalfrag.spv");
    mShaders[PRECOMP_IRRADIANCE_SHADER] = std::make_unique<ShaderProgram>("precomputeirradiance", "./spv/vert.spv", "./spv/precomputeirradiancefrag.spv");
    mShaders[SCENE_OBJECT_SHADER] = std::make_unique<ShaderProgram>("sceneobject", "./spv/sceneobjvert.spv", "./spv/sceneobjfrag.spv");

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
    mRenderParams.mCloudSettings.x = 0.0f;
    mRenderParams.mCloudSettings.y = 0.01f;
    mRenderParams.mCloudSettings.z = 1.0f;
    mRenderParams.mCloudSettings.w = 10000.0f;
    mRenderParams.mCloudMapping.x = 4.0f;
    mRenderParams.mCloudMapping.y = 4.0f;
    mRenderParams.mCloudMapping.z = 0.8f;
    mRenderParams.mCloudAbsorption.x = 1.0f;
    mRenderParams.mSteps.x = 1024;
    mRenderParams.mSteps.y = 8;
    mRenderParams.mScreenSettings.x = 1600;
    mRenderParams.mScreenSettings.y = 900;
    mRenderParams.mScreenSettings.z = 0;
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

    mSceneObjectParams.mIndices = glm::ivec4(0);
    addUniform(SCENE_OBJECT_PARAMS, mSceneObjectParams);

    loadStates();

    // cubemap environment
    mSkyCubemap = std::make_unique<RenderCubemapTexture>(mEnvironmentResolution.x);
    mFinalSkyCubemap = std::make_unique<RenderCubemapTexture>(mEnvironmentResolution.x);
    mIrradianceCubemap = std::make_unique<RenderCubemapTexture>(mIrradianceResolution.x);

    // ocean related noise texture and other shader buffers
    mOceanFFTHighRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_1, OCEAN_DIMENSIONS_1);
    mOceanFFTMidRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_2, OCEAN_DIMENSIONS_2);
    mOceanFFTLowRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_3, OCEAN_DIMENSIONS_3);

    // compute water geometry
    //updateWaterGrid();
    mClipmap.generateGeometry();

    // compute camera and projection matrix for normal camera
    glm::mat4 projMatrix = glm::perspective(glm::radians(60.0f), 1600.0f / 900.0f, 0.1f, 10000.0f);
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mViewProjectionMat.mProjectionMatrix = projMatrix;
    mViewProjectionMat.mViewMatrix = viewMatrix;
    addUniform(MVP_MATRIX, mViewProjectionMat);

    mPreviousViewProjectionMat = mViewProjectionMat;
    addUniform(PREV_MVP_MATRIX, mPreviousViewProjectionMat);

    mPrecomputeMatrix.mProjectionMatrix = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 10000.0f);
    mPrecomputeMatrix.mViewMatrix = glm::lookAt(glm::vec3(0, 1, 0), glm::vec3(0, 1, 0) + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));

    // hosek
    mHosekSkyModel = std::make_unique<Hosek>(glm::vec3(mSkyParams.mSunSetting.x, mSkyParams.mSunSetting.y, mSkyParams.mSunSetting.z), 512);

    // load textures
    FreeImage_Initialise();
    bool result = loadTexture(mOceanFoamTexture, true, false, "./resources/foamDiffuse.jpg");
    assert(result);
    result = result && loadTexture(mBlueNoiseTexture, false, true, "./resources/blueNoise512.png");
    assert(result);
    FreeImage_DeInitialise();

    // load models
    std::string inputfile = "./models/sphere.obj";
    assert(loadModel(inputfile));
}

Renderer::~Renderer()
{

}


bool Renderer::loadTexture(
    std::unique_ptr<Texture> &tex,
    const bool               mipmap,
    const bool               alpha,
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
    
    tex = std::make_unique<Texture>((int)width, (int)height, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST, mipmap, 8, false, alpha, (void*)bits);
    tex->generateMipmap();
    FreeImage_Unload(dib);
    return true;
}


bool Renderer::loadModel(
    const std::string &fileName)
{
    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = "./models/";

    tinyobj::ObjReader reader;

    if (!reader.ParseFromFile(fileName, reader_config))
    {
        if (!reader.Error().empty())
        {
            std::cerr << "TinyObjReader: " << reader.Error();
        }
        return false;
    }

    if (!reader.Warning().empty()) 
    {
        std::cout << "TinyObjReader: " << reader.Warning();
    }

    auto& attrib = reader.GetAttrib();
    auto& shapes = reader.GetShapes();
    auto& materials = reader.GetMaterials();

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
            vertices.reserve(vertices.capacity() + fv);
            indices.reserve(vertices.capacity() + fv);

            Vertex currentVertex;
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                currentVertex.mPosition.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                currentVertex.mPosition.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                currentVertex.mPosition.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0)
                {
                    currentVertex.mNormal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    currentVertex.mNormal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    currentVertex.mNormal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0)
                {
                    currentVertex.mUV.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    currentVertex.mUV.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                indices.push_back(vertices.size());
                vertices.push_back(currentVertex);
            }
            index_offset += fv;

            // per-face material
            shapes[s].mesh.material_ids[f];
        }
    }

    // push data to the device
    const uint32_t idx = mModels.size();
    mModels.push_back(std::make_unique<VertexBuffer>());
    mModels.at(idx)->update(
        sizeof(Vertex) * vertices.size(),
        sizeof(uint32_t) * indices.size(), 
        vertices.data(), 
        indices.data());
    mModelMats.push_back(glm::mat4(1.0f));

    // push model matrices to buffer
    mModelMatsBuffer = std::make_unique<ShaderBuffer>(mModelMats.size() * sizeof(glm::mat4));
    mModelMatsBuffer->upload(mModelMats.data());
    return true;
}


void Renderer::updateCamera(
    const int deltaX, 
    const int deltaY)
{
    // update previous matrix to this frame's mvp matrix
    mPreviousViewProjectionMat = mViewProjectionMat;
    updateUniform(PREV_MVP_MATRIX, mPreviousViewProjectionMat);

    mCamera.update(deltaX * 2.0f * M_PI / mResolution.x, deltaY * M_PI / mResolution.y);
    mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mCamParams);

    // update MVP
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mViewProjectionMat.mViewMatrix = viewMatrix;
    updateUniform(MVP_MATRIX, mViewProjectionMat);
}


void Renderer::updateCameraZoom(
    const int dir)
{
    // update previous matrix to this frame's mvp matrix
    mPreviousViewProjectionMat = mViewProjectionMat;
    updateUniform(PREV_MVP_MATRIX, mPreviousViewProjectionMat);

    mCamera.updateZoom(dir);
    mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mCamParams);

    // update MVP
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mViewProjectionMat.mViewMatrix = viewMatrix;
    updateUniform(MVP_MATRIX, mViewProjectionMat);
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


void Renderer::renderWater(
    const bool precompute)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (mRenderWater)
    {
        mShaders[WATER_SHADER]->use();
        switch (mSkyParams.mPrecomputeSettings.y)
        {
            case NISHITA_SKY: 
            {
                if (precompute)
                {
                    mSkyCubemap->bindTexture(WATER_ENV_TEX, 0);
                }
                else
                {
                    mFinalSkyCubemap->bindTexture(WATER_ENV_TEX, 0);
                }
                break;
            }
            case HOSEK_SKY:  
            {
                mHosekSkyModel->bind(WATER_ENV_TEX);             
                break;
            }
        }
        mOceanFFTHighRes->bind(WATER_DISPLACEMENT1_TEX);
        mOceanFFTMidRes->bind(WATER_DISPLACEMENT2_TEX);
        mOceanFFTLowRes->bind(WATER_DISPLACEMENT3_TEX);
        mOceanFoamTexture->bindTexture(WATER_FOAM_TEX);

        if (mOceanWireframe && !precompute)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        mClipmap.draw();
        if (mOceanWireframe && !precompute)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        mShaders[WATER_SHADER]->disable();
    }
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
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
        mRenderParams.mScreenSettings.x = 1600;
        mRenderParams.mScreenSettings.y = 900;
        updateUniform(RENDERER_PARAMS, mRenderParams);

        // reallocate render texture
        mScreenRenderTextures.resize(SCREEN_BUFFER_COUNT);
        for (int i = 0; i < SCREEN_BUFFER_COUNT; ++i)
        {
            mScreenRenderTextures[i] = std::make_unique<RenderTexture>(1, width * mLowResFactor, height * mLowResFactor);
        }
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
        mViewProjectionMat.mProjectionMatrix = perspectiveMatrix;
        updateUniform(MVP_MATRIX, mViewProjectionMat);
    }
}


void Renderer::preRender()
{
    if (!mPerlinNoiseRenderTexture)
    {
        return;
    }
    mRenderStartTime = std::chrono::high_resolution_clock::now();

    // precompute cloud's noise textures (perlin worley and worley fbm)
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_CLOUD_SHADER);
    {
        mCloudTexture.bindImageTexture(PRECOMPUTE_CLOUD_CLOUD_TEX, GL_READ_WRITE);
        const int workGroupSize = int(float(CLOUD_RESOLUTION) / float(PRECOMPUTE_CLOUD_LOCAL_SIZE));
        mShaders[PRECOMP_CLOUD_SHADER]->dispatch(true, workGroupSize, workGroupSize, workGroupSize);
        mCloudNoiseUpdated |= (1 << (mFrameCount % 4));
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_CLOUD_SHADER);

    // render into 100x100 noise textures
    glViewport(0, 0, 100, 100);
    {
        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(PERLIN_NOISE_SHADER);
        {
            mPerlinNoiseRenderTexture->bind();
            mShaders[PERLIN_NOISE_SHADER]->use();
            mQuad.draw();
            mShaders[PERLIN_NOISE_SHADER]->disable();
            mPerlinNoiseRenderTexture->unbind();
        }
        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PERLIN_NOISE_SHADER);

        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(WORLEY_NOISE_SHADER);
        {
            mWorleyNoiseRenderTexture->bind();
            mShaders[WORLEY_NOISE_SHADER]->use();
            mQuad.draw();
            mShaders[WORLEY_NOISE_SHADER]->disable();
            mWorleyNoiseRenderTexture->unbind();
        }
        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(WORLEY_NOISE_SHADER);

        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(CLOUD_NOISE_SHADER);
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
        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(CLOUD_NOISE_SHADER);
    }

    // ocean waves (iFFT) displacement precomputation
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_OCEAN_H0_SHADER);
    if (mRenderWater)
    {
        mOceanFFTHighRes->precompute(*this, mOceanParams);
        mOceanFFTMidRes->precompute(*this, mOceanParams);
        mOceanFFTLowRes->precompute(*this, mOceanParams);
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_OCEAN_H0_SHADER);

    // render cubemap (for reflection and preintegral evaluation)
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_SKY_SHADER);
    if (mUpdateSky)
    {
        glViewport(0, 0, int(mEnvironmentResolution.x), int(mEnvironmentResolution.y));

        if (mSkyParams.mPrecomputeSettings.y == NISHITA_SKY)
        {
            glm::vec3 rayleigh, mie, sky;
            nishitaSky(
                0.001f,
                mSkyParams.mNishitaSetting.x,
                mSkyParams.mNishitaSetting.y,
                glm::vec3(mSkyParams.mSunSetting.x, mSkyParams.mSunSetting.y, mSkyParams.mSunSetting.z),
                glm::vec3(mSkyParams.mSunSetting.x, mSkyParams.mSunSetting.y, mSkyParams.mSunSetting.z),
                rayleigh,
                mie,
                sky);
            mSkyParams.mSunLuminance = glm::vec4(sky.x, sky.y, sky.z, 1.0f);
            updateUniform(SKY_PARAMS, mSkyParams);
            mShaders[PRECOMP_SKY_SHADER]->use();
            for (int i = 0; i < 6; ++i)
            {
                mSkyParams.mPrecomputeSettings.x = i;
                updateUniform(SKY_PARAMS, offsetof(SkyParams, mPrecomputeSettings), sizeof(mSkyParams.mPrecomputeSettings), mSkyParams.mPrecomputeSettings);

                mSkyCubemap->bind(i);
                mQuad.draw();
            }
            mShaders[PRECOMP_SKY_SHADER]->disable();
            mSkyCubemap->unbind();
        }
        else if(mSkyParams.mPrecomputeSettings.y == HOSEK_SKY)
        {
            mSkyParams.mSunLuminance = glm::vec4(1.0f);
            updateUniform(SKY_PARAMS, mSkyParams);
            mHosekSkyModel->update(glm::vec3(mSkyParams.mSunSetting.x, mSkyParams.mSunSetting.y, mSkyParams.mSunSetting.z));
        }
        mUpdateSky = false;
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_SKY_SHADER);

    // bind fbo for the following
    mSkyParams.mPrecomputeSettings.x = mFrameCount % 6;
    updateUniform(SKY_PARAMS, offsetof(SkyParams, mPrecomputeSettings), sizeof(mSkyParams.mPrecomputeSettings), mSkyParams.mPrecomputeSettings);

    // 1. render environment sky box
    mTimeQueries.at(mFrameCount% QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_ENV_SHADER);
    if(mCloudNoiseUpdated == 0xF)
    {
        mFinalSkyCubemap->bind(mSkyParams.mPrecomputeSettings.x);
        glViewport(0, 0, int(mEnvironmentResolution.x), int(mEnvironmentResolution.y));

        // make sure we clear buffer as we render water geometry in this pass
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        mCloudTexture.bindTexture(PRECOMPUTE_ENVIRONMENT_CLOUD_TEX);
        mBlueNoiseTexture->bindTexture(PRECOMPUTE_ENVIRONMENT_NOISE_TEX);
        mSkyCubemap->bindTexture(PRECOMPUTE_ENVIRONMENT_SKY_TEX, 0);
        mShaders[PRECOMP_ENV_SHADER]->use();
        {
            // force 100 max steps for cubemap since it's low resolution
            const int oldMaxSteps = mRenderParams.mSteps.x;
            mRenderParams.mSteps.x = 100;
            updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mSteps), sizeof(mRenderParams.mSteps.x), mRenderParams.mSteps.x);

            mQuad.draw();

            // restore max steps to what it was before
            mRenderParams.mSteps.x = oldMaxSteps;
            updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mSteps), sizeof(mRenderParams.mSteps.x), mRenderParams.mSteps.x);
        }
        mShaders[PRECOMP_ENV_SHADER]->disable();

        // 2. render water
        mPrecomputeCamParams.mEye = glm::vec4(0, 50, 0, 1);
        mPrecomputeCamParams.mUp = glm::vec4(0, -1, 0, 0);
        switch (mSkyParams.mPrecomputeSettings.x)
        {
            case 0:
            {
                mPrecomputeCamParams.mTarget = mPrecomputeCamParams.mEye + glm::vec4(1, 0, 0, 0);
                break;
            }
            case 1:
            {
                mPrecomputeCamParams.mTarget = mPrecomputeCamParams.mEye + glm::vec4(-1, 0, 0, 0);
                break;
            }
            case 2:
            {
                // we are skipping this case
                mPrecomputeCamParams.mTarget = mPrecomputeCamParams.mEye + glm::vec4(0, -1, 0, 0);
                break;
            }
            case 3:
            {
                mPrecomputeCamParams.mTarget = mPrecomputeCamParams.mEye + glm::vec4(0, -1, 0, 0);
                mPrecomputeCamParams.mUp = glm::vec4(1, 0, 0, 0);
                break;
            }
            case 4:
            {
                mPrecomputeCamParams.mTarget = mPrecomputeCamParams.mEye + glm::vec4(0, 0, 1, 0);
                break;
            }
            case 5:
            {
                mPrecomputeCamParams.mTarget = mPrecomputeCamParams.mEye + glm::vec4(0, 0, -1, 0);
                break;
            }
        }
        mPrecomputeMatrix.mViewMatrix = glm::lookAt(
            glm::vec3(mPrecomputeCamParams.mEye),
            glm::vec3(mPrecomputeCamParams.mTarget),
            glm::vec3(mPrecomputeCamParams.mUp));

        // have to use the camera settings for precomputation viewport and camera and then revert it back after finishing
        if (mSkyParams.mPrecomputeSettings.x != 2)
        {
            updateUniform(CAMERA_PARAMS, mPrecomputeCamParams);
            updateUniform(MVP_MATRIX, mPrecomputeMatrix);
            renderWater(true);
            updateUniform(MVP_MATRIX, mViewProjectionMat);
            updateUniform(CAMERA_PARAMS, mCamParams);
        }
        mFinalSkyCubemap->unbind();

        mSkySideUpdated |= (1 << (mSkyParams.mPrecomputeSettings.x));
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_ENV_SHADER);
    
    // 3. preintegrate diffuse
    updateUniform(SKY_PARAMS, offsetof(SkyParams, mPrecomputeSettings), sizeof(mSkyParams.mPrecomputeSettings), mSkyParams.mPrecomputeSettings);
    if(mUpdateIrradiance && mSkySideUpdated == 0x3F)
    {
        mIrradianceCubemap->bind(mSkyParams.mPrecomputeSettings.x);

        mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_IRRADIANCE_SHADER);
        glViewport(0, 0, int(mIrradianceResolution.x), int(mIrradianceResolution.y));

        mFinalSkyCubemap->bindTexture(PRECOMPUTE_IRRADIANCE_SKY_TEX, 0);
        mShaders[PRECOMP_IRRADIANCE_SHADER]->use();
        mQuad.draw();
        mShaders[PRECOMP_IRRADIANCE_SHADER]->disable();
        mIrradianceCubemap->unbind();

        mIrradianceSideUpdated |= (1 << (mSkyParams.mPrecomputeSettings.x));
        if (mIrradianceSideUpdated == 0x3F)
        {
            mUpdateIrradiance = false;
            mIrradianceSideUpdated = 0;
        }
        mTimeQueries.at(mFrameCount% QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_IRRADIANCE_SHADER);
    }
}


void Renderer::render()
{
    if ((mScreenRenderTextures.size() == 0) ||
        mResolution.x <= 0.1f ||
        mResolution.y <= 0.1f)
    {
        return;
    }

    // update the time for simulation
    mRenderParams.mSettings.x = mTime * 0.001f;
    updateUniform(RENDERER_PARAMS, 0, sizeof(float), mRenderParams);

    // clear buffers
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render quarter sized render texture
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(TEMPORAL_QUAD_SHADER);
    {
        glViewport(0, 0, mResolution.x * mLowResFactor, mResolution.y * mLowResFactor);
        mScreenRenderTextures[mFrameCount % SCREEN_BUFFER_COUNT]->bind();
        mCloudTexture.bindTexture(QUAD_CLOUD_TEX);
        mBlueNoiseTexture->bindTexture(QUAD_NOISE_TEX);
        mScreenRenderTextures[(mFrameCount + 1) % SCREEN_BUFFER_COUNT]->bindTexture(QUAD_PREV_SCREEN_TEX, 0);
        switch (mSkyParams.mPrecomputeSettings.y)
        {
        case NISHITA_SKY: mSkyCubemap->bindTexture(QUAD_ENV_TEX, 0); break;
        case HOSEK_SKY:   mHosekSkyModel->bind(QUAD_ENV_TEX);        break;
        }
        mShaders[TEMPORAL_QUAD_SHADER]->use();
        mQuad.draw();
        mShaders[TEMPORAL_QUAD_SHADER]->disable();
        mScreenRenderTextures[mFrameCount % SCREEN_BUFFER_COUNT]->unbind();
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(TEMPORAL_QUAD_SHADER);

    // render final quad
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(TEXTURED_QUAD_SHADER);
    {
        glViewport(0, 0, int(mResolution.x), int(mResolution.y));
        mShaders[TEXTURED_QUAD_SHADER]->use();
        mScreenRenderTextures[mFrameCount % SCREEN_BUFFER_COUNT]->bindTexture(SCREEN_QUAD_TEX, 0);
        mQuad.draw();
        mShaders[TEXTURED_QUAD_SHADER]->disable();
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(TEXTURED_QUAD_SHADER);
    
    // enable depth mask for rendering objects in world space
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(WATER_SHADER);
    renderWater(false);
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(WATER_SHADER);

    // render scene objects
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(SCENE_OBJECT_SHADER);
    mShaders[SCENE_OBJECT_SHADER]->use();
    mModelMatsBuffer->bind(SCENE_MODEL_MATRIX);
    mIrradianceCubemap->bindTexture(SCENE_OBJECT_IRRADIANCE, 0);
    for (int i = 0; i < mModels.size(); ++i)
    {
        // set model matrix index
        mSceneObjectParams.mIndices.x = i;
        updateUniform(SCENE_OBJECT_PARAMS, mSceneObjectParams);

        mModels[i]->draw();
    }
    mShaders[SCENE_OBJECT_SHADER]->disable();
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(SCENE_OBJECT_SHADER);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
}


void Renderer::postRender()
{
    // update previous matrix to this frame's mvp matrix
    mPreviousViewProjectionMat = mViewProjectionMat;
    updateUniform(PREV_MVP_MATRIX, mPreviousViewProjectionMat);

    // get the results from the queries
    TimeQuery& timeQuery = *(mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT));
    mTotalShaderTimes = 0.0f;
    for (int i = 0; i < SHADER_COUNT; ++i)
    {
        const float shaderTime = timeQuery.elapsedTime(i);
        mShaderTimestamps[i] = shaderTime;
        mTotalShaderTimes += shaderTime;
    }

    // calculate the delta time in milliseconds for this frame
    mRenderEndTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> elapsed = (mRenderEndTime - mRenderStartTime);
    mDeltaTime = elapsed.count();

    mTime += (mDeltaTime);
    if (mTime > 3600000.0f)
    {
        mTime = fmodf(mTime, 3600000.0f);
    }

    // record the frame time and fps
    const float fps = (1.0f / (mDeltaTime * 0.001f));
    mFrameTimes[mFrameCount % FRAMETIMES_COUNT] = mDeltaTime;
    mFpsRecords[mFrameCount % FRAMETIMES_COUNT] = fps;
    if (fps > mMaxFps)
    {
        mMaxFps = fps;
    }
    if (fps < mMinFps)
    {
        mMinFps = fps;
    }

    // increment frame count since this is the last step of the iteration
    ++mFrameCount;
    if (mFrameCount >= 1000000)
    {
        mFrameCount = 0;
    }
    mRenderParams.mScreenSettings.z = mFrameCount;
    updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mScreenSettings), sizeof(glm::ivec4), mRenderParams.mScreenSettings);
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

        if (ImGui::BeginMenu("Tools"))
        {
            if (ImGui::MenuItem("Buffers"))
            {
                mShowBuffersWindow = !mShowBuffersWindow;
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
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
                            mUpdateSky = true;
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
                    mUpdateSky = true;
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                }

                if (ImGui::SliderFloat("y", &mSkyParams.mSunSetting.y, 0.0f, 1.0f))
                {
                    mUpdateSky = true;
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                }

                if (ImGui::SliderFloat("z", &mSkyParams.mSunSetting.z, -1.0f, 1.0f))
                {
                    mUpdateSky = true;
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                }

                if (ImGui::SliderFloat("intensity", &mSkyParams.mSunSetting.w, 0.0f, 100.0f))
                {
                    mUpdateSky = true;
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                }
                ImGui::Text("Sky");

                if (ImGui::SliderFloat("Rayleigh", &mSkyParams.mNishitaSetting.x, 0.0f, 40.0f))
                {
                    mUpdateSky = true;
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                }

                if (ImGui::SliderFloat("Mie", &mSkyParams.mNishitaSetting.y, 0.0f, 40.0f))
                {
                    mUpdateSky = true;
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
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
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderInt("Perlin octaves", &mPerlinNoiseParams.mNoiseOctaves, 1, 8))
                {
                    updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }

                if (ImGui::SliderFloat("Perlin freq", &mPerlinNoiseParams.mSettings.z, 0.0f, 100.0f, " %.3f", ImGuiSliderFlags_Logarithmic))
                {
                    updateUniform(PERLIN_PARAMS, mPerlinNoiseParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Absorption", &mRenderParams.mCloudAbsorption.x, 0.0f, 1.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Anisotropy", &mRenderParams.mCloudSettings.x, -1.0f, 1.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Cloud speed", &mRenderParams.mCloudSettings.y, 0.0f, 1.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Cloud coverage", &mRenderParams.mCloudMapping.z, 0.0f, 1.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Cloud density", &mRenderParams.mCloudSettings.z, 0.0001f, 100.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Cloud BBox height", &mRenderParams.mCloudSettings.w, 100.0f, 100000.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Cloud UV width", &mRenderParams.mCloudMapping.x, 1.0f, 1000.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderFloat("Cloud UV height", &mRenderParams.mCloudMapping.y, 1.0f, 1000.0f))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderInt("Max steps", &mRenderParams.mSteps.x, 4, 1024))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
                }
                if (ImGui::SliderInt("Max shadow steps", &mRenderParams.mSteps.y, 2, 32))
                {
                    updateUniform(RENDERER_PARAMS, mRenderParams);
                    mUpdateIrradiance = true;
                    mIrradianceSideUpdated = 0;
                    mSkySideUpdated = 0;
                    mCloudNoiseUpdated = 0;
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

    // properties window
    if (mShowPropertiesWindow)
    {
        ImGui::Begin("Properties", &mShowPropertiesWindow);
        if (ImGui::BeginTabBar("Settings", ImGuiTabBarFlags_None))
        {
            if (ImGui::BeginTabItem("Performance"))
            {
                ImGui::Text("Frame time: %f ms", mDeltaTime);
                {
                    ImGui::PlotLines("Time", &mFrameTimes[0], IM_ARRAYSIZE(mFrameTimes), 0, 0, 0.0f, 30.0f, ImVec2(0, 80));
                }

                ImGui::Text("Frames per sec: %.2f fps", (1.0f / (mDeltaTime * 0.001f)));

                {
                    ImGui::PlotLines("FPS", &mFpsRecords[0], IM_ARRAYSIZE(mFpsRecords), 0, 0, 0.0f, 300.0f, ImVec2(0, 80));
                }


                ImGui::Text("GPU time");

                char buf[32];
                for (int i = 0; i < SHADER_COUNT; ++i)
                {
                    if (mShaderTimestamps[i] > 0.01f)
                    {
                        sprintf(buf, "%.2f ms", mShaderTimestamps[i]);
                        ImGui::ProgressBar(mShaderTimestamps[i] / mTotalShaderTimes, ImVec2(0.f, 0.f), buf);
                        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);


                        sprintf(buf, "%s", mShaders[i]->name());
                        ImGui::Text(buf);
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Camera"))
            {
                ImGui::Text("Position");
                ImGui::Text("x: %.2f y: %.2f z: %.2f", mCamera.getEye().x, mCamera.getEye().y, mCamera.getEye().z);
                ImGui::Text("Target");
                ImGui::Text("x: %.2f y: %.2f z: %.2f", mCamera.getTarget().x, mCamera.getTarget().y, mCamera.getTarget().z);
                ImGui::Text("Distance");
                ImGui::Text("dist: %.2f", length(mCamera.getTarget() - mCamera.getEye()));
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        ImGui::End();
    }

    if (mShowBuffersWindow)
    {
        ImGui::Begin("Buffers", &mShowBuffersWindow);

        // the buffer we are rendering to
        const float textureWidth = 100 * mRenderParams.mSettings.y;
        const float textureHeight = 100;
        ImGui::Text("Main pass");
        ImTextureID screenBuffer = (ImTextureID)mScreenRenderTextures[mFrameCount % SCREEN_BUFFER_COUNT]->getTextureId(0);
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
        screenBuffer = (ImTextureID)mScreenRenderTextures[(mFrameCount + 1) % SCREEN_BUFFER_COUNT]->getTextureId(0);
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

    ini["renderparams"]["anisotropy"] = std::to_string(mRenderParams.mCloudSettings.x);
    ini["renderparams"]["speed"] = std::to_string(mRenderParams.mCloudSettings.y);
    ini["renderparams"]["density"] = std::to_string(mRenderParams.mCloudSettings.z);
    ini["renderparams"]["height"] = std::to_string(mRenderParams.mCloudSettings.w);
    ini["renderparams"]["cloudu"] = std::to_string(mRenderParams.mCloudMapping.x);
    ini["renderparams"]["cloudv"] = std::to_string(mRenderParams.mCloudMapping.y);
    ini["renderparams"]["coverage"] = std::to_string(mRenderParams.mCloudMapping.z);
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
            mRenderParams.mCloudSettings.x = std::stof(ini["renderparams"]["anisotropy"]);
            mRenderParams.mCloudSettings.y = std::stof(ini["renderparams"]["speed"]);
            mRenderParams.mCloudSettings.z = std::stof(ini["renderparams"]["density"]);
            mRenderParams.mCloudSettings.w = std::stof(ini["renderparams"]["height"]);
            mRenderParams.mCloudMapping.x = std::stof(ini["renderparams"]["cloudu"]);
            mRenderParams.mCloudMapping.y = std::stof(ini["renderparams"]["cloudv"]);
            mRenderParams.mCloudMapping.z = std::stof(ini["renderparams"]["coverage"]);

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