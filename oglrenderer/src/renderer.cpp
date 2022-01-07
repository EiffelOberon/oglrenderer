#include "renderer.h"

#include <random>

#define TINYOBJLOADER_IMPLEMENTATION
#include "freeglut.h"
#include "FreeImage/FreeImage.h"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"
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
    , mPrefilterCubemapResolution(PREFILTER_CUBEMAP_RESOLUTION, PREFILTER_CUBEMAP_RESOLUTION)
    , mQuad(GL_TRIANGLE_STRIP, 4)
    , mGUIRenderer(this)
    , mClipmap(6)
    , mClipmapLevel(0)
    , mDrawCallTriangleCount(0)
    , mWaterTriangleCount(0)
    , mSkyCubemap(nullptr)
    , mFinalSkyCubemap(nullptr)
    , mIrradianceCubemap(nullptr)
    , mPrecomputedFresnelTexture(nullptr)
    , mPrefilterCubemap(nullptr)
    , mWorleyNoiseRenderTexture(nullptr)
    , mCamera()
    , mUpdateSky(true)
    , mUpdateIrradiance(true)
    , mCloudNoiseUpdated(0)
    , mIrradianceSideUpdated(0)
    , mSkySideUpdated(0)
    , mDeltaTime(0.0f)
    , mLowResFactor(0.5f)
    , mTime(0.0f)
    , mTotalShaderTimes(0.0f)
    , mFrameCount(0)
    , mWaterGrid()
    , mMinFps(FLT_MAX)
    , mMaxFps(FLT_MIN)
{
    mRoot = std::make_unique<Object>("root");

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
    mShaders[PRECOMP_FRESNEL_SHADER] = std::make_unique<ShaderProgram>("fresnel", "./spv/precomputefresnel.spv");
    mShaders[PREFILTER_ENVIRONMENT_SHADER] = std::make_unique<ShaderProgram>("prefilterenvironment", "./spv/vert.spv", "./spv/prefilterenvironmentfrag.spv");

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
    mParams.mCamParams.mEye = glm::vec4(eye, 0.0f);
    mParams.mCamParams.mTarget = glm::vec4(target, 0.0f);
    mParams.mCamParams.mUp = glm::vec4(up, 0.0f);
    addUniform(CAMERA_PARAMS, mParams.mCamParams);

    // initialize sun
    addUniform(SKY_PARAMS, mParams.mSkyParams);

    // initialize noise
    addUniform(PERLIN_PARAMS, mParams.mPerlinNoiseParams);
    addUniform(WORLEY_PARAMS, mParams.mWorleyNoiseParams);

    // initialize render params
    addUniform(RENDERER_PARAMS, mParams.mRenderParams);

    // initialize ocean params
    addUniform(OCEAN_PARAMS, mParams.mOceanParams);

    addUniform(SCENE_OBJECT_PARAMS, mParams.mSceneObjectParams);

    loadStates();

    // cubemap environment
    mSkyCubemap = std::make_unique<RenderCubemapTexture>(mEnvironmentResolution.x, false);
    mFinalSkyCubemap = std::make_unique<RenderCubemapTexture>(mEnvironmentResolution.x, false);
    mIrradianceCubemap = std::make_unique<RenderCubemapTexture>(mIrradianceResolution.x, false);
    mPrefilterCubemap = std::make_unique<RenderCubemapTexture>(mPrefilterCubemapResolution.x, true);

    // ocean related noise texture and other shader buffers
    mOceanFFTHighRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_1, OCEAN_DIMENSIONS_1);
    mOceanFFTMidRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_2, OCEAN_DIMENSIONS_2);
    mOceanFFTLowRes = std::make_unique<OceanFFT>(*this, OCEAN_RESOLUTION_3, OCEAN_DIMENSIONS_3);

    // compute water geometry
    //updateWaterGrid();
    mClipmap.generateGeometry();
    mWaterTriangleCount = mClipmap.triangleCount();

    // compute camera and projection matrix for normal camera
    glm::mat4 projMatrix = glm::perspective(glm::radians(60.0f), 1600.0f / 900.0f, 0.1f, 10000.0f);
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mViewProjectionMat.mProjectionMatrix = projMatrix;
    mViewProjectionMat.mViewMatrix = viewMatrix;
    addUniform(MVP_MATRIX, mViewProjectionMat);

    mPreviousViewProjectionMat = mViewProjectionMat;
    addUniform(PREV_MVP_MATRIX, mPreviousViewProjectionMat);

    mPrecomputeMatrix.mProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10000.0f);
    mPrecomputeMatrix.mViewMatrix = glm::lookAt(glm::vec3(0, 1, 0), glm::vec3(0, 1, 0) + glm::vec3(0, 0, -1), glm::vec3(0, 1, 0));

    // hosek
    mHosekSkyModel = std::make_unique<Hosek>(glm::vec3(
        mParams.mSkyParams.mSunSetting.x,
        mParams.mSkyParams.mSunSetting.y, 
        mParams.mSkyParams.mSunSetting.z), 
        512);

    // precompute fresnel
    mPrecomputedFresnelTexture = std::make_unique<Texture>(FRESNEL_RESOLUTION, FRESNEL_RESOLUTION, GL_LINEAR, false, 32, false, true, false, nullptr);
    mShaders[PRECOMP_FRESNEL_SHADER]->use();
    mPrecomputedFresnelTexture->bindImageTexture(PRECOMPUTE_FRESNEL_TEX, GL_WRITE_ONLY);
    mShaders[PRECOMP_FRESNEL_SHADER]->dispatch(true, FRESNEL_RESOLUTION / PRECOMPUTE_FRESNEL_LOCAL_SIZE, FRESNEL_RESOLUTION / PRECOMPUTE_FRESNEL_LOCAL_SIZE, 1);
    mShaders[PRECOMP_FRESNEL_SHADER]->disable();

    // load textures
    FreeImage_Initialise();
    bool result = loadTexture(mOceanFoamTexture, true, false, "./resources/foamDiffuse.jpg");
    assert(result);
    result = result && loadTexture(mBlueNoiseTexture, false, true, "./resources/blueNoise512.png");
    assert(result);

    // load models
    //std::string inputfile = "./models/box.obj";
    //assert(loadModel(inputfile));


    // lua initialization
    mLuaState = luaL_newstate();
    luaL_openlibs(mLuaState);
    luabridge::getGlobalNamespace(mLuaState)
        .beginClass<Renderer>("renderer")
        .addFunction("loadModel", (bool (Renderer::*)(std::string))& Renderer::loadModel)
        .endClass();
    luabridge::push(mLuaState, this);
    lua_setglobal(mLuaState, "renderer");

    //luaL_dofile(mLuaState, "./script/test.lua");

    FreeImage_DeInitialise();
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
    
    tex = std::make_unique<Texture>((int)width, (int)height, mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST, mipmap, 8, false, alpha, true, (void*)bits);
    tex->generateMipmap();
    FreeImage_Unload(dib);
    return true;
}


bool Renderer::loadModel(
    const std::string &fileName)
{
    const std::string folderPath = fileName.substr(0, fileName.find_last_of('/') + 1);

    tinyobj::ObjReaderConfig reader_config;
    reader_config.mtl_search_path = folderPath.c_str();

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

    // count the number of vertex per material, each material is rendered as a draw call
    std::map<uint32_t, uint32_t> mMaterialVertexCount;
    std::map<uint32_t, uint32_t> mMaterialVertexOffset;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            uint32_t count = shapes[s].mesh.num_face_vertices[f];
            uint32_t materialId = shapes[s].mesh.material_ids[f];

            if (mMaterialVertexCount.find(materialId) == mMaterialVertexCount.end())
            {
                mMaterialVertexCount[materialId] = count;
                mMaterialVertexOffset[materialId] = 0;
            }
            else
            {
                mMaterialVertexCount[materialId] += count;
            }
        }
    }

    std::map<uint32_t, std::vector<Vertex>> vertexList;
    std::map<uint32_t, std::vector<uint32_t>> indexList;
    for (std::map<uint32_t, uint32_t>::iterator it = mMaterialVertexCount.begin(); 
        it != mMaterialVertexCount.end(); 
        ++it)
    {
        const uint32_t matId = it->first;
        const uint32_t vertexCount = it->second;

        vertexList[matId].resize(vertexCount);
        indexList[matId].resize(vertexCount);
    }
        
    // modify the material list instance
    const uint32_t materialIdx = mMaterials.size();
    mMaterials.resize(mMaterials.size() + materials.size());
    mMaterialNames.resize(mMaterialNames.size() + materials.size());
    for (uint32_t i = 0; i < materials.size(); ++i)
    {
        mMaterials[i + materialIdx].mTexture1 = glm::ivec4(INVALID_TEX_ID, INVALID_TEX_ID, INVALID_TEX_ID, INVALID_TEX_ID);
        mMaterialNames[i + materialIdx] = materials[i].name;
        std::unique_ptr<Texture> diffuseTex;
        if (materials[i].diffuse_texname != "" && 
            loadTexture(diffuseTex, true, false, folderPath + materials[i].diffuse_texname))
        {
            const uint32_t texIdx = mTextures.size();
            mTextures.push_back(std::move(diffuseTex));
            mMaterials[i + materialIdx].mTexture1.x = texIdx;
        }

        // TODO
        assert(materials[i].specular_texname == "");
        assert(materials[i].roughness_texname == "");
        assert(materials[i].metallic_texname == "");
        assert(materials[i].alpha_texname == "");

        // colors
        mMaterials[i + materialIdx].mDiffuse = glm::vec4(materials[i].diffuse[0], materials[i].diffuse[1], materials[i].diffuse[2], 1.0f);
        mMaterials[i + materialIdx].mSpecular = glm::vec4(materials[i].specular[0], materials[i].specular[1], materials[i].specular[2], 1.0f);

        // fixed floating point values if textures are not used
        mMaterials[i + materialIdx].mShadingParams.x = materials[i].roughness;
        mMaterials[i + materialIdx].mShadingParams.y = materials[i].metallic;
        mMaterials[i + materialIdx].mShadingParams.z = materials[i].ior;
    }

    // Loop over shapes
    //int bufferIdx = 0;
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

            // per-face material
            const int materialId = shapes[s].mesh.material_ids[f];
            
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                Vertex& vertex = vertexList[materialId][mMaterialVertexOffset[materialId]];
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                vertex.mPosition.x = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
                vertex.mPosition.y = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
                vertex.mPosition.z = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

                // Check if `normal_index` is zero or positive. negative = no normal data
                if (idx.normal_index >= 0)
                {
                    vertex.mNormal.x = attrib.normals[3 * size_t(idx.normal_index) + 0];
                    vertex.mNormal.y = attrib.normals[3 * size_t(idx.normal_index) + 1];
                    vertex.mNormal.z = attrib.normals[3 * size_t(idx.normal_index) + 2];
                }

                // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                if (idx.texcoord_index >= 0)
                {
                    vertex.mUV.x = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
                    vertex.mUV.y = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];
                }

                indexList[materialId][mMaterialVertexOffset[materialId]] = mMaterialVertexOffset[materialId];
                mMaterialVertexOffset[materialId] = mMaterialVertexOffset[materialId] + 1;
            }
            index_offset += fv;
        }
    }

    // create meshes that holds vertex buffers
    const std::string file = fileName.substr(fileName.find_last_of('/') + 1);
    Object* parent = mRoot->addChild(std::make_unique<Object>(file));
    for (std::map<uint32_t, uint32_t>::iterator it = mMaterialVertexCount.begin();
        it != mMaterialVertexCount.end();
        ++it)
    {
        const uint32_t matId = it->first;
        const uint32_t vertexCount = it->second;

        // insert mesh into the node tree
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>(mMaterialNames[matId]);
        mesh->update(vertexList[matId], indexList[matId]);
        parent->addChild(std::move(mesh));
    }

    // update draw calls list
    updateDrawCalls(mRoot.get());

    // upload material buffer to the GPU
    mMaterialBuffer = std::make_unique<ShaderBuffer>(mMaterials.size() * sizeof(Material), GL_DYNAMIC_DRAW);
    mMaterialBuffer->upload(mMaterials.data());
    return true;
}


void Renderer::updateDrawCalls(
    Object    *obj)
{
    if (!obj)
    {
        // should not happen
        assert(false);
        return;
    }

    // reset if root
    if (obj == mRoot.get())
    {
        mDrawCallTriangleCount = 0;
        mDrawCalls.clear();
        mDrawCallMatrices.clear();
    }

    if (obj->isDrawable())
    {
        Mesh *mesh = static_cast<Mesh*>(obj);
        mDrawCalls.push_back(mesh);

        // calculate total triangle count
        mDrawCallTriangleCount += mesh->triangleCount();

        Object* parent = obj->parent();
        glm::mat4 transformMatrix = obj->transform();
        while (parent)
        {
            transformMatrix *= parent->transform();
            parent = parent->parent();
        }

        mDrawCallMatrices.push_back(transformMatrix);
    }

    for (int i = 0; i < obj->childCount(); ++i)
    {
        updateDrawCalls(obj->child(i));
    }
    // push model matrices to buffer
    if (!mModelMatsBuffer)
    {
        mModelMatsBuffer = std::make_unique<ShaderBuffer>(mDrawCallMatrices.size() * sizeof(glm::mat4), GL_STREAM_DRAW);
    }
    mModelMatsBuffer->upload(mDrawCallMatrices.data());
}


void Renderer::updateCamera(
    const int deltaX, 
    const int deltaY)
{
    // update previous matrix to this frame's mvp matrix
    mPreviousViewProjectionMat = mViewProjectionMat;
    updateUniform(PREV_MVP_MATRIX, mPreviousViewProjectionMat);

    mCamera.update(deltaX * 2.0f * M_PI / mResolution.x, deltaY * M_PI / mResolution.y);
    mParams.mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mParams.mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mParams.mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mParams.mCamParams);

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
    mParams.mCamParams.mEye = glm::vec4(mCamera.getEye(), 0.0f);
    mParams.mCamParams.mTarget = glm::vec4(mCamera.getTarget(), 0.0f);
    mParams.mCamParams.mUp = glm::vec4(mCamera.getUp(), 0.0f);
    updateUniform(CAMERA_PARAMS, mParams.mCamParams);

    // update MVP
    glm::mat4 viewMatrix = mCamera.getViewMatrix();
    mViewProjectionMat.mViewMatrix = viewMatrix;
    updateUniform(MVP_MATRIX, mViewProjectionMat);
}


void Renderer::renderWater(
    const bool precompute)
{
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (mParams.mRenderWater)
    {
        mShaders[WATER_SHADER]->use();
        switch (mParams.mSkyParams.mPrecomputeSettings.y)
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
        if (!precompute)
        {
            mPrefilterCubemap->bindTexture(WATER_PREFILTER_ENV, 0);
            mPrecomputedFresnelTexture->bindTexture(WATER_PRECOMPUTED_GGX);
            mIrradianceCubemap->bindTexture(WATER_IRRADIANCE, 0);
        }
        if (mParams.mOceanWireframe && !precompute)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        mClipmap.draw();
        if (mParams.mOceanWireframe && !precompute)
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
        mParams.mRenderParams.mSettings.y = (mResolution.x / mResolution.y);
        mParams.mRenderParams.mScreenSettings.x = 1600;
        mParams.mRenderParams.mScreenSettings.y = 900;
        updateUniform(RENDERER_PARAMS, mParams.mRenderParams);

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
    
    mParams.mRenderParams.mSettings.z = 1;
    updateUniform(RENDERER_PARAMS, 0, sizeof(glm::vec4), mParams.mRenderParams);

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
            mParams.mWorleyNoiseParams.mTextureIdx = i;
            updateUniform(WORLEY_PARAMS, mParams.mWorleyNoiseParams);

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
    if (mParams.mRenderWater)
    {
        mOceanFFTHighRes->precompute(*this, mParams.mOceanParams);
        mOceanFFTMidRes->precompute(*this, mParams.mOceanParams);
        mOceanFFTLowRes->precompute(*this, mParams.mOceanParams);
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_OCEAN_H0_SHADER);

    // render cubemap (for reflection and preintegral evaluation)
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_SKY_SHADER);
    if (mUpdateSky)
    {
        glViewport(0, 0, int(mEnvironmentResolution.x), int(mEnvironmentResolution.y));

        if (mParams.mSkyParams.mPrecomputeSettings.y == NISHITA_SKY)
        {
            glm::vec3 rayleigh, mie, sky;
            nishitaSky(
                0.001f,
                mParams.mSkyParams.mNishitaSetting.x,
                mParams.mSkyParams.mNishitaSetting.y,
                glm::vec3(mParams.mSkyParams.mSunSetting.x, mParams.mSkyParams.mSunSetting.y, mParams.mSkyParams.mSunSetting.z),
                glm::vec3(mParams.mSkyParams.mSunSetting.x, mParams.mSkyParams.mSunSetting.y, mParams.mSkyParams.mSunSetting.z),
                rayleigh,
                mie,
                sky);
            mParams.mSkyParams.mSunLuminance = glm::vec4(sky.x, sky.y, sky.z, 1.0f);
            updateUniform(SKY_PARAMS, mParams.mSkyParams);
            mShaders[PRECOMP_SKY_SHADER]->use();
            for (int i = 0; i < 6; ++i)
            {
                mParams.mSkyParams.mPrecomputeSettings.x = i;
                updateUniform(
                    SKY_PARAMS, 
                    offsetof(SkyParams, mPrecomputeSettings), 
                    sizeof(mParams.mSkyParams.mPrecomputeSettings), 
                    mParams.mSkyParams.mPrecomputeSettings);

                mSkyCubemap->bind(i);
                mQuad.draw();
            }
            mShaders[PRECOMP_SKY_SHADER]->disable();
            mSkyCubemap->unbind();
        }
        else if(mParams.mSkyParams.mPrecomputeSettings.y == HOSEK_SKY)
        {
            mParams.mSkyParams.mSunLuminance = glm::vec4(1.0f);
            updateUniform(SKY_PARAMS, mParams.mSkyParams);
            mHosekSkyModel->update(glm::vec3(
                mParams.mSkyParams.mSunSetting.x, 
                mParams.mSkyParams.mSunSetting.y, 
                mParams.mSkyParams.mSunSetting.z));
        }
        mUpdateSky = false;
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_SKY_SHADER);

    // bind fbo for the following
    mParams.mSkyParams.mPrecomputeSettings.x = mFrameCount % 6;
    updateUniform(SKY_PARAMS, offsetof(SkyParams, mPrecomputeSettings), sizeof(mParams.mSkyParams.mPrecomputeSettings), mParams.mSkyParams.mPrecomputeSettings);

    // 1. render environment sky box
    mTimeQueries.at(mFrameCount% QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_ENV_SHADER);
    if(mCloudNoiseUpdated == 0xF)
    {
        mFinalSkyCubemap->bind(mParams.mSkyParams.mPrecomputeSettings.x);
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
            const int oldMaxSteps = mParams.mRenderParams.mSteps.x;
            mParams.mRenderParams.mSteps.x = 100;
            updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mSteps), sizeof(mParams.mRenderParams.mSteps.x), mParams.mRenderParams.mSteps.x);

            mQuad.draw();

            // restore max steps to what it was before
            mParams.mRenderParams.mSteps.x = oldMaxSteps;
            updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mSteps), sizeof(mParams.mRenderParams.mSteps.x), mParams.mRenderParams.mSteps.x);
        }
        mShaders[PRECOMP_ENV_SHADER]->disable();

        // 2. render water
        mParams.mPrecomputeCamParams.mEye = glm::vec4(PRECOMPUTE_CAM_POS_X, PRECOMPUTE_CAM_POS_Y, PRECOMPUTE_CAM_POS_Z, 1);
        mParams.mPrecomputeCamParams.mUp = glm::vec4(0, -1, 0, 0);
        switch (mParams.mSkyParams.mPrecomputeSettings.x)
        {
            case 0:
            {
                mParams.mPrecomputeCamParams.mTarget = mParams.mPrecomputeCamParams.mEye + glm::vec4(1, 0, 0, 0);
                break;
            }
            case 1:
            {
                mParams.mPrecomputeCamParams.mTarget = mParams.mPrecomputeCamParams.mEye + glm::vec4(-1, 0, 0, 0);
                break;
            }
            case 2:
            {
                // we are skipping this case
                mParams.mPrecomputeCamParams.mTarget = mParams.mPrecomputeCamParams.mEye + glm::vec4(0, -1, 0, 0);
                break;
            }
            case 3:
            {
                mParams.mPrecomputeCamParams.mTarget = mParams.mPrecomputeCamParams.mEye + glm::vec4(0, -1, 0, 0);
                mParams.mPrecomputeCamParams.mUp = glm::vec4(0, 0, -1, 0);
                break;
            }
            case 4:
            {
                mParams.mPrecomputeCamParams.mTarget = mParams.mPrecomputeCamParams.mEye + glm::vec4(0, 0, 1, 0);
                break;
            }
            case 5:
            {
                mParams.mPrecomputeCamParams.mTarget = mParams.mPrecomputeCamParams.mEye + glm::vec4(0, 0, -1, 0);
                break;
            }
        }
        mPrecomputeMatrix.mViewMatrix = glm::lookAt(
            glm::vec3(mParams.mPrecomputeCamParams.mEye),
            glm::vec3(mParams.mPrecomputeCamParams.mTarget),
            glm::vec3(mParams.mPrecomputeCamParams.mUp));

        // have to use the camera settings for precomputation viewport and camera and then revert it back after finishing
        if (mParams.mSkyParams.mPrecomputeSettings.x != 2)
        {
            updateUniform(CAMERA_PARAMS, mParams.mPrecomputeCamParams);
            updateUniform(MVP_MATRIX, mPrecomputeMatrix);
            renderWater(true);
            updateUniform(MVP_MATRIX, mViewProjectionMat);
            updateUniform(CAMERA_PARAMS, mParams.mCamParams);
        }
        mFinalSkyCubemap->unbind();

        mSkySideUpdated |= (1 << (mParams.mSkyParams.mPrecomputeSettings.x));
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_ENV_SHADER);
    
    // 3. preintegrate diffuse and ggx
    updateUniform(SKY_PARAMS, offsetof(SkyParams, mPrecomputeSettings), sizeof(mParams.mSkyParams.mPrecomputeSettings), mParams.mSkyParams.mPrecomputeSettings);
    if(mUpdateIrradiance && mSkySideUpdated == 0x3F)
    {
        // diffuse irradiance
        mTimeQueries.at(mFrameCount% QUERY_DOUBLE_BUFFER_COUNT)->start(PRECOMP_IRRADIANCE_SHADER);
        mIrradianceCubemap->bind(mParams.mSkyParams.mPrecomputeSettings.x);
        {
            glViewport(0, 0, int(mIrradianceResolution.x), int(mIrradianceResolution.y));
            mFinalSkyCubemap->bindTexture(PRECOMPUTE_IRRADIANCE_SKY_TEX, 0);
            mShaders[PRECOMP_IRRADIANCE_SHADER]->use();
            mQuad.draw();
            mShaders[PRECOMP_IRRADIANCE_SHADER]->disable();
        }
        mIrradianceCubemap->unbind();
        mTimeQueries.at(mFrameCount% QUERY_DOUBLE_BUFFER_COUNT)->end(PRECOMP_IRRADIANCE_SHADER);

        // update flags
        mIrradianceSideUpdated |= (1 << (mParams.mSkyParams.mPrecomputeSettings.x));
        if (mIrradianceSideUpdated == 0x3F)
        {
            mUpdateIrradiance = false;
            mIrradianceSideUpdated = 0;
        }
    }

    // specular ggx
    mTimeQueries.at(mFrameCount% QUERY_DOUBLE_BUFFER_COUNT)->start(PREFILTER_ENVIRONMENT_SHADER);
    for (int i = 0; i < PREFILTER_MIP_COUNT; ++i)
    {
        const uint32_t mipWidth = 128 * std::pow(0.5, i);
        const uint32_t mipHeight = 128 * std::pow(0.5, i);
        const float roughness = float(i) / float(PREFILTER_MIP_COUNT - 1);
        glViewport(0, 0, mipWidth, mipHeight);

        mParams.mSkyParams.mPrecomputeGGXSettings.x = roughness;
        updateUniform(SKY_PARAMS, offsetof(SkyParams, mPrecomputeGGXSettings), sizeof(glm::vec4), mParams.mSkyParams.mPrecomputeGGXSettings);

        mPrefilterCubemap->bind(mParams.mSkyParams.mPrecomputeSettings.x, mipWidth, mipHeight, i);
        mFinalSkyCubemap->bindTexture(PREFILTER_ENVIRONMENT_SKY_TEX, 0);
        mShaders[PREFILTER_ENVIRONMENT_SHADER]->use();
        mQuad.draw();
        mShaders[PREFILTER_ENVIRONMENT_SHADER]->disable();
        mPrefilterCubemap->unbind();
    }
    mTimeQueries.at(mFrameCount % QUERY_DOUBLE_BUFFER_COUNT)->end(PREFILTER_ENVIRONMENT_SHADER);
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
    mParams.mRenderParams.mSettings.x = mTime * 0.001f;
    mParams.mRenderParams.mSettings.z = 0;
    updateUniform(RENDERER_PARAMS, 0, sizeof(glm::vec4), mParams.mRenderParams);

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
        switch (mParams.mSkyParams.mPrecomputeSettings.y)
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
    if (mDrawCalls.size() > 0)
    {
        mShaders[SCENE_OBJECT_SHADER]->use();
        mModelMatsBuffer->bind(SCENE_MODEL_MATRIX);
        mMaterialBuffer->bind(SCENE_MATERIAL);
        mIrradianceCubemap->bindTexture(SCENE_OBJECT_IRRADIANCE, 0);
        mPrefilterCubemap->bindTexture(SCENE_OBJECT_PREFILTER_ENV, 0);
        mPrecomputedFresnelTexture->bindTexture(SCENE_OBJECT_PRECOMPUTED_GGX);
        mFinalSkyCubemap->bindTexture(SCENE_OBJECT_SKY, 0);
        for (int i = 0; i < mDrawCalls.size(); ++i)
        {
            // set model matrix index
            mParams.mSceneObjectParams.mIndices.x = i;
            updateUniform(SCENE_OBJECT_PARAMS, mParams.mSceneObjectParams);
            mParams.mRenderParams.mScreenSettings.w = i;
            updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mScreenSettings), sizeof(glm::ivec4), mParams.mRenderParams.mScreenSettings);

            if (mMaterials[i].mTexture1.x != INVALID_TEX_ID)
            {
                mTextures[mMaterials[i].mTexture1.x]->bindTexture(SCENE_OBJECT_DIFFUSE);
            }

            mDrawCalls[i]->draw();
        }
        mShaders[SCENE_OBJECT_SHADER]->disable();
    }
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
    mParams.mRenderParams.mScreenSettings.z = mFrameCount;
    updateUniform(RENDERER_PARAMS, offsetof(RendererParams, mScreenSettings), sizeof(glm::ivec4), mParams.mRenderParams.mScreenSettings);
}


void Renderer::renderGUI()
{
    mGUIRenderer.renderMenu(mParams);

    GLuint cloudIds[4] = 
    { 
        mCloudNoiseRenderTexture[0]->getTextureId(0), 
        mCloudNoiseRenderTexture[1]->getTextureId(0),
        mCloudNoiseRenderTexture[2]->getTextureId(0), 
        mCloudNoiseRenderTexture[3]->getTextureId(0)
    };
    mGUIRenderer.renderSkyWindow(
        mRoot.get(), 
        cloudIds,
        mOceanFFTHighRes.get(),
        mOceanFFTMidRes.get(),
        mOceanFFTLowRes.get(),
        mMaterialBuffer.get(),
        mMaterials, 
        mMaterialNames, 
        mParams);

    // properties window
    mGUIRenderer.renderPropertiesWindow(
        mCamera,
        mDeltaTime,
        mFrameTimes,
        mFpsRecords,
        mShaders,
        mShaderTimestamps,
        mTotalShaderTimes,
        mDrawCallTriangleCount,
        mWaterTriangleCount,
        mParams);

    GLuint renderTextures[2] =
    {
        mScreenRenderTextures[mFrameCount % SCREEN_BUFFER_COUNT]->getTextureId(0),
        mScreenRenderTextures[(mFrameCount + 1)% SCREEN_BUFFER_COUNT]->getTextureId(0)
    };
    mGUIRenderer.renderBuffersWindow(
        renderTextures,
        mPrecomputedFresnelTexture->texId(),
        mFrameCount,
        mParams);

    bool test = true;
    ImGui::ShowDemoWindow(&test);
}


void Renderer::resetEnvironment()
{
    mUpdateSky = true;
    mUpdateIrradiance = true;
    mIrradianceSideUpdated = 0;
    mSkySideUpdated = 0;
    mCloudNoiseUpdated = 0;
}


void Renderer::saveStates()
{
    mParams.save();
}


void Renderer::loadStates()
{
    mParams.load();
    updateUniform(SKY_PARAMS, mParams.mSkyParams);
    updateUniform(RENDERER_PARAMS, mParams.mRenderParams);
    updateUniform(WORLEY_PARAMS, mParams.mWorleyNoiseParams);
    updateUniform(PERLIN_PARAMS, mParams.mPerlinNoiseParams);
    updateUniform(OCEAN_PARAMS, mParams.mOceanParams);
    resetEnvironment();
}

