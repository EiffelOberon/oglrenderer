#pragma once

#include <math.h>
#include <vector>

#include "deviceconstants.h"
#include "devicestructs.h"
#include "glm/glm.hpp"
#include "renderer.h"
#include "texture.h"

class OceanFFT
{

public:
    OceanFFT(
        const int N)
        : mN(N)
        , mPasses((int)(float(log(float(N))) / float(log(2.0f))))
        , mOceanDisplacementTexture(N, N, GL_LINEAR_MIPMAP_LINEAR, true, 32, false)
        , mOceanH0SpectrumTexture(N, N, GL_NEAREST, false, 32, false)
        , mOceanHDxSpectrumTexture(N, N, GL_NEAREST, false, 32, false)
        , mOceanHDySpectrumTexture(N, N, GL_NEAREST, false, 32, false)
        , mOceanHDzSpectrumTexture(N, N, GL_NEAREST, false, 32, false)
        , mPingPongTexture(N, N, GL_NEAREST, false, 32, false)
    {
        mBitReversedIndices.resize(N);
        for (int i = 0; i < N; i++)
        {
            int x = (int) reverse((uint32_t)i);
            x = rotateLeft(x, mPasses);
            mBitReversedIndices[i] = x;
        }
    }

    ~OceanFFT()
    {
    }


    void precompute(
        Renderer    &renderer,
        OceanParams &oceanParams,
        Texture     &noiseTexture,
        Texture     &butterflyTexture)
    {
        const int workGroupSize = int(float(OCEAN_RESOLUTION) / float(PRECOMPUTE_OCEAN_WAVES_LOCAL_SIZE));
        // pass 1
        noiseTexture.bindTexture(OCEAN_HEIGHTFIELD_NOISE);
        bindPass1(false);
        renderer.dispatch(PRECOMP_OCEAN_H0_SHADER, true, workGroupSize, workGroupSize, 1);

        // pass 2
        bindPass2();
        renderer.dispatch(PRECOMP_OCEAN_H_SHADER, true, workGroupSize, workGroupSize, 1);

        for (int texIdx = 0; texIdx < 3; ++texIdx)
        {
            butterflyTexture.bindImageTexture(BUTTERFLY_INPUT_TEX, GL_READ_ONLY);
            // 0 = dx, 1 = dy, 2 = dz
            bindPass3(texIdx);

            for (int i = 0; i < passes(); ++i)
            {
                oceanParams.mPingPong.y = i;
                oceanParams.mPingPong.z = 0;
                renderer.updateUniform(OCEAN_PARAMS, offsetof(OceanParams, mPingPong), sizeof(glm::ivec4), oceanParams.mPingPong);

                renderer.dispatch(BUTTERFLY_SHADER, true, workGroupSize, workGroupSize, 1);

                oceanParams.mPingPong.x++;
                oceanParams.mPingPong.x = oceanParams.mPingPong.x % 2;
            }

            for (int i = 0; i < passes(); ++i)
            {
                oceanParams.mPingPong.y = i;
                oceanParams.mPingPong.z = 1;
                renderer.updateUniform(OCEAN_PARAMS, offsetof(OceanParams, mPingPong), sizeof(glm::ivec4), oceanParams.mPingPong);

                renderer.dispatch(BUTTERFLY_SHADER, true, workGroupSize, workGroupSize, 1);

                oceanParams.mPingPong.x++;
                oceanParams.mPingPong.x = oceanParams.mPingPong.x % 2;
            }

            oceanParams.mPingPong.w = texIdx;
            renderer.updateUniform(OCEAN_PARAMS, offsetof(OceanParams, mPingPong), sizeof(glm::ivec4), oceanParams.mPingPong);
            bindPass4(texIdx);
            renderer.dispatch(INVERSION_SHADER, true, workGroupSize, workGroupSize, 1);
        }
        finalize();
    }

    void bind(
        const int idx)
    {
        mOceanDisplacementTexture.bindTexture(idx);
    }
    

    void bindPass1(
        const bool readonly)
    {
        mOceanH0SpectrumTexture.bindImageTexture(OCEAN_HEIGHTFIELD_H0K, readonly ? GL_READ_ONLY : GL_WRITE_ONLY);
    }


    void bindPass2()
    {
        mOceanH0SpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H0K, GL_READ_ONLY);
        mOceanHDxSpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H_X, GL_WRITE_ONLY);
        mOceanHDySpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H_Y, GL_WRITE_ONLY);
        mOceanHDzSpectrumTexture.bindImageTexture(OCEAN_HEIGHT_FINAL_H_Z, GL_WRITE_ONLY);
    }


    void bindPass3(
        const int differential)
    {
        Texture* spectrum = nullptr;
        switch (differential)
        {
        case 0: spectrum = &mOceanHDxSpectrumTexture; break;
        case 1: spectrum = &mOceanHDySpectrumTexture; break;
        case 2: spectrum = &mOceanHDzSpectrumTexture; break;
        }

        spectrum->bindImageTexture(BUTTERFLY_PINGPONG_TEX0, GL_READ_WRITE);
        mPingPongTexture.bindImageTexture(BUTTERFLY_PINGPONG_TEX1, GL_READ_WRITE);
    }


    void bindPass4(
        const int differential)
    {
        Texture* spectrum = nullptr;
        switch (differential)
        {
        case 0: spectrum = &mOceanHDxSpectrumTexture; break;
        case 1: spectrum = &mOceanHDySpectrumTexture; break;
        case 2: spectrum = &mOceanHDzSpectrumTexture; break;
        }
        spectrum->bindImageTexture(INVERSION_PINGPONG_TEX0, GL_READ_ONLY);
        mPingPongTexture.bindImageTexture(INVERSION_PINGPONG_TEX1, GL_READ_ONLY);
        mOceanDisplacementTexture.bindImageTexture(INVERSION_OUTPUT_TEX, GL_READ_WRITE);
    }


    void finalize()
    {
        mOceanDisplacementTexture.generateMipmap();
    }


    uint32_t h0TexId() const
    {
        return mOceanH0SpectrumTexture.texId();
    }


    uint32_t dxTexId() const
    {
        return mOceanHDxSpectrumTexture.texId();
    }


    uint32_t dyTexId() const
    {
        return mOceanHDySpectrumTexture.texId();
    }


    uint32_t dzTexId() const
    {
        return mOceanHDzSpectrumTexture.texId();
    }


    uint32_t displacementTexId() const
    {
        return mOceanDisplacementTexture.texId();
    }


    int passes() const
    {
        return mPasses;
    }


    const int* bitReversedIndices() const
    {
        return mBitReversedIndices.data();
    }


    int bitReversedIndicesSizeInBytes() const
    {
        return mBitReversedIndices.size() * sizeof(int);
    }


private:
    uint32_t reverse(uint32_t x)
    {
        x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
        x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
        x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
        x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
        x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
        return x;
    }


    int rotateLeft(int value, int distance) 
    {
        int mask = (1 << distance) - 1;
        int leftPart = (value << distance) & (~mask);
        int rightPart = (value >> (32 - distance)) & (mask);
        return leftPart | rightPart;
    }


    Texture mOceanDisplacementTexture;
    Texture mOceanH0SpectrumTexture;
    Texture mOceanHDxSpectrumTexture;
    Texture mOceanHDySpectrumTexture;
    Texture mOceanHDzSpectrumTexture;
    Texture mPingPongTexture;

    int mN;
    int mPasses;
    std::vector<int> mBitReversedIndices;
};