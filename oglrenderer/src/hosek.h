#pragma once

#include "HosekSky/ArHosekSkyModel.h"

#include "glm/glm.hpp"
#include "texture.h"

class Hosek
{
public:
    Hosek(
        const glm::vec3 sunDir,
        const uint32_t  textureSize)
        : mSunDir(sunDir)
        , mTexSize(textureSize)
        , mCubemap(textureSize)
        , mDataR(nullptr)
        , mDataG(nullptr)
        , mDataB(nullptr)
    {
        mSkyBoxData.resize(6);

        precompute();
    }


    ~Hosek()
    {
        arhosekskymodelstate_free(mDataR);
        arhosekskymodelstate_free(mDataG);
        arhosekskymodelstate_free(mDataB);
    }


    void precompute()
    {
        if (mDataR != nullptr) { arhosekskymodelstate_free(mDataR); mDataR = nullptr;}
        if (mDataG != nullptr) { arhosekskymodelstate_free(mDataG); mDataG = nullptr;}
        if (mDataB != nullptr) { arhosekskymodelstate_free(mDataB); mDataB = nullptr;}

        mSunDir.y = glm::clamp(mSunDir.y, 0.0f, 1.0f);
        mSunDir = glm::normalize(mSunDir);
        float thetaS = angleBetween(mSunDir, glm::vec3(0.0f, 1.0f, 0.0f));
        float elevation = PI / 2.0f - thetaS;
        
        mDataR = arhosek_xyz_skymodelstate_alloc_init(2.0f, 0.5f, elevation);
        mDataG = arhosek_xyz_skymodelstate_alloc_init(2.0f, 0.5f, elevation);
        mDataB = arhosek_xyz_skymodelstate_alloc_init(2.0f, 0.5f, elevation);
        for (int s = 0; s < 6; s++)
        {
            if (mSkyBoxData[s].size() != mTexSize * mTexSize)
            {
                mSkyBoxData[s].resize(mTexSize * mTexSize);
            }

            for (int y = 0; y < mTexSize; y++)
            {
                for (int x = 0; x < mTexSize; x++)
                {
                    glm::vec3 dir = xyToRayDir(x, y, s, mTexSize, mTexSize);
                    glm::vec3 radiance = sampleSky(dir);
                    int idx = (y * mTexSize) + x;
                    mSkyBoxData[s][idx] = glm::vec4(glm::normalize(radiance), 1.0f);
                }
            }

            mCubemap.upload(s, mSkyBoxData[s].data());
        }
    }

    
    void update(
        const glm::vec3 sunDir)
    {
        mSunDir = sunDir;
        precompute();
    }


    uint32_t texId() const
    {
        return mCubemap.texId();
    }


    void bind(
        const uint32_t texUnit)
    {
        mCubemap.bindTexture(texUnit);
    }

private:

    static glm::vec3 xyToRayDir(
        const int x,
        const int y,
        const int s,
        const int width,
        const int height)
    {
        const float u = ((x + 0.5f) / float(width)) * 2.0f - 1.0f;
        float v = ((y + 0.5f) / float(height)) * 2.0f - 1.0f;
        v *= -1.0f;

        glm::vec3 dir = glm::vec3(0.0f);

        // +x, -x, +y, -y, +z, -z
        switch (s)
        {
        case 0:
            dir = glm::normalize(glm::vec3(1.0f, v, -u));
            break;
        case 1:
            dir = glm::normalize(glm::vec3(-1.0f, v, u));
            break;
        case 2:
            dir = glm::normalize(glm::vec3(u, 1.0f, -v));
            break;
        case 3:
            dir = glm::normalize(glm::vec3(u, -1.0f, v));
            break;
        case 4:
            dir = glm::normalize(glm::vec3(u, v, 1.0f));
            break;
        case 5:
            dir = glm::normalize(glm::vec3(-u, v, -1.0f));
            break;
        }
        return dir;
    }

    float angleBetween(
        const glm::vec3 &dir0, 
        const glm::vec3 &dir1)
    {
        return glm::acos(glm::max(glm::dot(dir0, dir1), 0.00001f));
    }


    glm::vec3 sampleSky(
        const glm::vec3 dir)
    {
        const float gamma = angleBetween(dir, mSunDir);
        const float theta = angleBetween(dir, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::vec3 radiance;

        radiance.x = float(arhosek_tristim_skymodel_radiance(mDataR, theta, gamma, 0));
        radiance.y = float(arhosek_tristim_skymodel_radiance(mDataG, theta, gamma, 1));
        radiance.z = float(arhosek_tristim_skymodel_radiance(mDataB, theta, gamma, 2));

        glm::vec3 rgb;
        rgb.x =  3.2404542f*radiance.x - 1.5371385f*radiance.y - 0.4985314f*radiance.z;
        rgb.y = -0.9692660f*radiance.x + 1.8760108f*radiance.y + 0.0415560f*radiance.z;
        rgb.z =  0.0556434f*radiance.x - 0.2040259f*radiance.y + 1.0572252f*radiance.z;

        return rgb * 2.0f * PI / 683.0f;
    }


    ArHosekSkyModelState* mDataR;
    ArHosekSkyModelState* mDataG;
    ArHosekSkyModelState* mDataB;
    glm::vec3 mSunDir;
    uint32_t mTexSize;
    std::vector<std::vector<glm::vec4>> mSkyBoxData;

    TextureCubemap mCubemap;
};