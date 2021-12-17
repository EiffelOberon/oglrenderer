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
        , mPasses((int)(log(float(N)) / log(2.0f)))
        , mButterflyTable(nullptr)
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
        if (!mButterflyTable)
        {
            delete[] mButterflyTable;
        }
    }
    

    void computeButterflyLookupTable()
    {
        mButterflyTable = new float[mN * mPasses * 4];

        for (int i = 0; i < mPasses; i++)
        {
            int nBlocks = (int)pow(2, mPasses - 1 - i);
            int nHInputs = (int)pow(2, i);

            for (int j = 0; j < nBlocks; j++)
            {
                for (int k = 0; k < nHInputs; k++)
                {
                    int i1, i2, j1, j2;
                    if (i == 0)
                    {
                        i1 = j * nHInputs * 2 + k;
                        i2 = j * nHInputs * 2 + nHInputs + k;
                        j1 = BitReverse(i1);
                        j2 = BitReverse(i2);
                    }
                    else
                    {
                        i1 = j * nHInputs * 2 + k;
                        i2 = j * nHInputs * 2 + nHInputs + k;
                        j1 = i1;
                        j2 = i2;
                    }

                    float wr = cos(2.0f * M_PI * (float)(k * nBlocks) / float(mN));
                    float wi = sin(2.0f * M_PI * (float)(k * nBlocks) / float(mN));

                    int offset1 = 4 * (i1 + i * mN);
                    mButterflyTable[offset1 + 0] = j1;
                    mButterflyTable[offset1 + 1] = j2;
                    mButterflyTable[offset1 + 2] = wr;
                    mButterflyTable[offset1 + 3] = wi;

                    int offset2 = 4 * (i2 + i * mN);
                    mButterflyTable[offset2 + 0] = j1;
                    mButterflyTable[offset2 + 1] = j2;
                    mButterflyTable[offset2 + 2] = -wr;
                    mButterflyTable[offset2 + 3] = -wi;
                }
            }
        }
    }


    const int* bitReversedIndices() const
    {
        return mBitReversedIndices.data();
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

    int BitReverse(int i)
    {
        int j = i;
        int Sum = 0;
        int W = 1;
        int M = mN / 2;
        while (M != 0)
        {
            j = ((i & M) > M - 1) ? 1 : 0;
            Sum += j * W;
            W *= 2;
            M /= 2;
        }
        return Sum;
    }


    //Performs two FFTs on two complex numbers packed in a vector4
    glm::vec4 fft(
        const glm::vec2 w, 
        glm::vec4&      input1, 
        const glm::vec4 input2)
    {
        input1.x += w.x * input2.x - w.y * input2.y;
        input1.y += w.y * input2.x + w.x * input2.y;
        input1.z += w.x * input2.z - w.y * input2.w;
        input1.w += w.y * input2.z + w.x * input2.w;

        return input1;
    }

    //Performs one FFT on a complex number
    glm::vec2 fft(
        const glm::vec2 w, 
        glm::vec2&      input1, 
        const glm::vec2 input2)
    {
        input1.x += w.x * input2.x - w.y * input2.y;
        input1.y += w.y * input2.x + w.x * input2.y;

        return input1;
    }


    int peformFFT(
        int startIdx, 
        glm::vec2* data0, 
        glm::vec4* data1, 
        glm::vec4* data2)
    {

        int x;
        int y; 
        int i;
        int idx = 0; 
        int idx1; 
        int bftIdx;
        int X; 
        int Y;
        glm::vec2 w;

        int j = startIdx;

        for (i = 0; i < mPasses; i++, j++)
        {
            idx = j % 2;
            idx1 = (j + 1) % 2;

            for (x = 0; x < mN; x++)
            {
                for (y = 0; y < mN; y++)
                {
                    bftIdx = 4 * (x + i * mN);

                    X = (int)mButterflyTable[bftIdx + 0];
                    Y = (int)mButterflyTable[bftIdx + 1];
                    w.x = mButterflyTable[bftIdx + 2];
                    w.y = mButterflyTable[bftIdx + 3];

                    data0[idx, x + y * mN] = fft(w, data0[idx1, X + y * mN], data0[idx1, Y + y * mN]);
                    data1[idx, x + y * mN] = fft(w, data1[idx1, X + y * mN], data1[idx1, Y + y * mN]);
                    data2[idx, x + y * mN] = fft(w, data2[idx1, X + y * mN], data2[idx1, Y + y * mN]);
                }
            }
        }

        for (i = 0; i < mPasses; i++, j++)
        {
            idx = j % 2;
            idx1 = (j + 1) % 2;

            for (x = 0; x < mN; x++)
            {
                for (y = 0; y < mN; y++)
                {
                    bftIdx = 4 * (y + i * mN);

                    X = (int)mButterflyTable[bftIdx + 0];
                    Y = (int)mButterflyTable[bftIdx + 1];
                    w.x = mButterflyTable[bftIdx + 2];
                    w.y = mButterflyTable[bftIdx + 3];

                    data0[idx, x + y * mN] = fft(w, data0[idx1, x + X * mN], data0[idx1, x + Y * mN]);
                    data1[idx, x + y * mN] = fft(w, data1[idx1, x + X * mN], data1[idx1, x + Y * mN]);
                    data2[idx, x + y * mN] = fft(w, data2[idx1, x + X * mN], data2[idx1, x + Y * mN]);
                }
            }
        }

        return idx;
    }
	

    int mN;
    int mPasses;
    float* mButterflyTable;
    std::vector<int> mBitReversedIndices;
};