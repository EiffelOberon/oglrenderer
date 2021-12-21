#pragma once

#include <memory>

#include "glm/glm.hpp"

#include "deviceconstants.h"
#include "devicestructs.h"
#include "vertexbuffer.h"


class ClipmapPatch
{
public:
    ClipmapPatch(
        const uint32_t width,
        const uint32_t height)
        : mWidth(width)
        , mHeight(height)
    {
    }

    
    ~ClipmapPatch()
    {

    }


    void generatePatch(
        const uint32_t iterationCount,
        const uint32_t n,
        const float    dimension)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

        int count = 0;
        float offset = (dimension + 1) / (n + 1);
        for (int row = 0; row < n; ++row)
        {
            for (int column = 0; column < n; ++column)
            {
                Vertex v;
                v.mPosition = glm::vec3(column * offset - dimension * 0.5f, 0.0f, row * offset - dimension * 0.5f);
                v.mNormal = glm::vec3(0, 1, 0);
                v.mUV = glm::vec2(column / float(n - 1), row / float(n - 1));
                vertices.push_back(v);
            }
        }

        int exclusionColumn = ((dimension / 2.0f) - (dimension / 4.0f)) / offset;

        for (int row = 0; row < (n - 1); ++row)
        {
            for (int column = 0; column < (n - 1); ++column)
            {
                if (iterationCount > 0 &&
                    (column > exclusionColumn && column < (n - 1 - exclusionColumn) &&
                     row > exclusionColumn && row < (n - 1 - exclusionColumn)))
                {
                    continue;
                }
                else
                {
                    indices.push_back(row * n + column);
                    indices.push_back(row * n + (column + 1));
                    indices.push_back((row + 1) * n + column);

                    indices.push_back((row + 1) * n + column);
                    indices.push_back(row * n + (column + 1));
                    indices.push_back((row + 1) * n + (column + 1));
                }
            }
        }

        mGrid.update(vertices.size() * sizeof(Vertex), indices.size() * sizeof(uint32_t), vertices.data(), indices.data());
    }


    void draw()
    {
        mGrid.draw();
    }

private:

    uint32_t mWidth;
    uint32_t mHeight;

    VertexBuffer mGrid;
};


class Clipmap
{
public:
    Clipmap(
        const uint32_t levels)
        : mWidth(0)
        , mHeight(0)
        , mLevels(levels)
    {

    }

    void generateGeometry()
    {
        int lowestLevel = 8;
        for (int i = 0; i < mLevels; ++i)
        {
            // patch count along each axis (square)
            int n = (pow(2, lowestLevel - i)) - 1;
            float dimension = (pow(2, lowestLevel + i)) - 1;
#ifdef _DEBUG
            printf("%d/%d | %d %f\n", i, mLevels, n, dimension);
#endif
            const uint32_t idx = mClipmapPatches.size();
            mClipmapPatches.push_back(std::make_unique<ClipmapPatch>(dimension, dimension));
            mClipmapPatches[idx]->generatePatch(i, n, dimension);
        }
    }


    void draw()
    {
        for (int i = 0; i < mClipmapPatches.size(); ++i)
        {
            mClipmapPatches[i]->draw();
        }
    }


    ~Clipmap()
    {

    }


    uint32_t levels()
    {
        return mLevels;
    }

private:
    uint32_t mLevels;
    uint32_t mWidth;
    uint32_t mHeight;

    std::vector<std::unique_ptr<ClipmapPatch>> mClipmapPatches;
};