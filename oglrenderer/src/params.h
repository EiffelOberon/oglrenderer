#pragma once

#include "deviceconstants.h"
#include "devicestructs.h"
#include "ini.h"

struct Params
{
    // render features
    bool mRenderWater;
    bool mOceanWireframe;

    // gui
    bool mShowPropertiesWindow;
    bool mShowSkyWindow;
    bool mShowBuffersWindow;

    // uniform buffers
    CameraParams      mCamParams;
    CameraParams      mPrecomputeCamParams;
    NoiseParams       mWorleyNoiseParams;
    NoiseParams       mPerlinNoiseParams;
    OceanParams       mOceanParams;
    RendererParams    mRenderParams;
    SkyParams         mSkyParams;
    SceneObjectParams mSceneObjectParams;

    Params()
    {
        mRenderWater = true;
        mOceanWireframe = false;

        mShowPropertiesWindow = true;
        mShowSkyWindow = true;
        mShowBuffersWindow = false;

        mSkyParams.mSunSetting = glm::vec4(0.0f, 1.0f, 0.0f, 20.0f);
        mSkyParams.mNishitaSetting = glm::vec4(20.0f, 20.0f, 0.0f, 0.0f);
        mSkyParams.mFogSettings = glm::vec4(3000.0f, 5000.0f, 0.0f, 0.0f);
        mSkyParams.mPrecomputeSettings.x = 0;
        mSkyParams.mPrecomputeSettings.y = 0;
        mSkyParams.mPrecomputeGGXSettings = glm::vec4(0.0f, 0.0f, 1.5f, 0.0f);

        mPerlinNoiseParams.mSettings = glm::vec4(1.0f, 1.0f, 0.4f, 1.0f);
        mPerlinNoiseParams.mNoiseOctaves = 7;

        mWorleyNoiseParams.mSettings = glm::vec4(1.0f, 1.0f, 0.0f, 1.0f);
        mWorleyNoiseParams.mInvert = true;

        mOceanParams.mHeightSettings = glm::ivec4(OCEAN_RESOLUTION_1, OCEAN_DIMENSIONS_1, 0, 0);
        mOceanParams.mPingPong = glm::ivec4(0, 0, 0, 0);
        mOceanParams.mWaveSettings = glm::vec4(4.0f, 40.0f, 1.0f, 1.0f);
        mOceanParams.mReflection = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        mOceanParams.mTransmission = glm::vec4(0.0f, 0.0f, 1.0f, 2000.0f);
        mOceanParams.mTransmission2 = glm::vec4(0.0f, 0.0f, 1.0f, 4.0f);
        mOceanParams.mFoamSettings = glm::vec4(1.0f, 0.6f, 1.33f, 0.01f);

        mRenderParams.mSettings.x = 0.0f;
        mRenderParams.mSettings.y = (1600.0f / 900.0f);
        mRenderParams.mSettings.z = 0;
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

        mSceneObjectParams.mIndices = glm::ivec4(0);
    }


    void save()
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
        ini["oceanparams"]["ior"] = std::to_string(mOceanParams.mFoamSettings.z);
        ini["oceanparams"]["roughness"] = std::to_string(mOceanParams.mFoamSettings.w);

        // generate an INI file (overwrites any previous file)
        file.generate(ini);
    }


    void load()
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

            if (ini.has("worleyparams"))
            {
                mWorleyNoiseParams.mInvert = bool(std::stoi(ini["worleyparams"]["invert"]));
            }

            if (ini.has("perlinparams"))
            {
                mPerlinNoiseParams.mNoiseOctaves = std::stoi(ini["perlinparams"]["octaves"]);
                mPerlinNoiseParams.mSettings.z = std::stof(ini["perlinparams"]["frequency"]);
            }

            if (ini.has("oceanparams"))
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

                mOceanParams.mFoamSettings.z = std::stof(ini["oceanparams"]["ior"]);
                mOceanParams.mFoamSettings.w = std::stof(ini["oceanparams"]["roughness"]);
            }

        }
    }
};