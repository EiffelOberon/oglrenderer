#pragma once

#include <math.h>
#include <vector>

#include "glm/glm.hpp"

class OceanFFT
{

public:
    OceanFFT(
        const int N)
        : mN(N)
        , mPasses((int)(float(log(float(N))) / float(log(2.0f))))
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
    

            int nBlocks = (int)pow(2, mPasses - 1 - i);
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


    int mN;
    int mPasses;
    std::vector<int> mBitReversedIndices;
};