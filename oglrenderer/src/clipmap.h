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
        const uint32_t maxIteration,
        const uint32_t lowestLevel,
        uint32_t       n,
        float          dimension)
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;

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
        int m = n - 1;
        for (int row = 0; row < m; ++row)
        {
            for (int column = 0; column < m; ++column)
            {
                if (iterationCount > 0 &&
                    (column > exclusionColumn && column < (n - exclusionColumn) &&
                     row > exclusionColumn && row < (n - exclusionColumn)))
                {
                    continue;
                }
                else if (row == 0 && column < m)
                {
                    if ((column + 1) <n)
                    {
                        if (column > 0)
                        {
                            indices.push_back(row * n + column);
                            indices.push_back((row + 1) * n + (column + 1));
                            indices.push_back((row + 1) * n + (column));
                        }

                        if ((column + 2) < n)
                        {
                            indices.push_back(row * n + column);
                            indices.push_back((row + 1) * n + (column + 2));
                            indices.push_back((row + 1) * n + (column + 1));
                            if ((column + 4) < n)
                            {
                                indices.push_back(row * n + column);
                                indices.push_back(row * n + (column + 4));
                                indices.push_back((row + 1) * n + (column + 2));

                                indices.push_back((row + 1) * n + (column + 2));
                                indices.push_back(row * n + (column + 4));
                                indices.push_back((row + 1) * n + (column + 3));

                                indices.push_back((row + 1) * n + (column + 3));
                                indices.push_back(row * n + (column + 4));
                                indices.push_back((row + 1) * n + (column + 4));
                            }
                            else
                            {
                                // patch up the triangle hole at the end because we didn't actually have this vertex at this resolution
                                const uint32_t newId = vertices.size();
                                Vertex v;
                                v.mPosition = glm::vec3((column + 4)* offset - dimension * 0.5f, 0.0f, (row) * offset - dimension * 0.5f);
                                v.mNormal = glm::vec3(0, 1, 0);
                                v.mUV = glm::vec2(column / float(n - 1), row / float(n - 1));
                                vertices.push_back(v);

                                indices.push_back(row * n + column);
                                indices.push_back((row + 1) * n + (column + 2));
                                indices.push_back(newId);
                            }
                        }
                    }
                    if (row == 0 && column == 0)
                    {
                        if ((row + 1) < n)
                        {
                            indices.push_back((row)*n + (column));
                            indices.push_back((row + 1) * n + (column + 1));
                            indices.push_back((row)*n + (column + 1));
                            if ((row + 2) < n)
                            {
                                indices.push_back((row)*n + (column));
                                indices.push_back((row + 2) * n + (column + 1));
                                indices.push_back((row + 1) * n + (column + 1));

                                if ((row + 4) < n)
                                {
                                    // big triangle
                                    indices.push_back((row)*n + (column));
                                    indices.push_back((row + 4) * n + (column));
                                    indices.push_back((row + 2) * n + (column + 1));

                                    indices.push_back((row + 4) * n + (column));
                                    indices.push_back((row + 3) * n + (column + 1));
                                    indices.push_back((row + 2) * n + (column + 1));

                                    indices.push_back((row + 4) * n + (column));
                                    indices.push_back((row + 4) * n + (column + 1));
                                    indices.push_back((row + 3) * n + (column + 1));
                                }
                            }
                        }
                    }

                    column += 3;
                    continue;
                }
                else if (column == 0 && row < m)
                {
                    if ((row % 4) == 0)
                    {
                        if ((row + 1) < n)
                        {
                            indices.push_back((row)*n + (column));
                            indices.push_back((row + 1) * n + (column + 1));
                            indices.push_back((row)*n + (column + 1));
                            if ((row + 2) < n)
                            {
                                indices.push_back((row)*n + (column));
                                indices.push_back((row + 2) * n + (column + 1));
                                indices.push_back((row + 1) * n + (column + 1));

                                if ((row + 4) < n)
                                {
                                    // big triangle
                                    indices.push_back((row)*n + (column));
                                    indices.push_back((row + 4) * n + (column));
                                    indices.push_back((row + 2) * n + (column + 1));

                                    indices.push_back((row + 4)* n + (column));
                                    indices.push_back((row + 3)* n + (column + 1));
                                    indices.push_back((row + 2)* n + (column + 1));

                                    indices.push_back((row + 4) * n + (column));
                                    indices.push_back((row + 4) * n + (column + 1));
                                    indices.push_back((row + 3) * n + (column + 1));
                                }
                                else
                                {
                                    // patch up the triangle hole at the end because we didn't actually have this vertex at this resolution
                                    const uint32_t newId = vertices.size();
                                    Vertex v;
                                    v.mPosition = glm::vec3((column) * offset - dimension * 0.5f, 0.0f, (row + 4)*offset - dimension * 0.5f);
                                    v.mNormal = glm::vec3(0, 1, 0);
                                    v.mUV = glm::vec2(column / float(n - 1), row / float(n - 1));
                                    vertices.push_back(v);

                                    indices.push_back((row)*n + (column));
                                    indices.push_back(newId);
                                    indices.push_back((row + 2) * n + (column + 1));
                                }
                            }
                        }
                    }
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

        generateHorizontalStitchingTriangles(n, dimension, vertices, indices);
        generateVerticalStitchingTriangles(n, dimension, vertices, indices);

        mGrid.update(vertices.size() * sizeof(Vertex), indices.size() * sizeof(uint32_t), vertices.data(), indices.data());
    }


    void draw()
    {
        mGrid.draw();
    }

private:

    void generateHorizontalStitchingTriangles(
        const uint32_t        n,
        const float           dimension,
        std::vector<Vertex>   &vertices,
        std::vector<uint32_t> &indices)
    {
        float offset = (dimension + 1) / (n + 1);
        const uint32_t endIdx = vertices.size();

        // stitching triangles for neighboring patches
        for (int column = 0; column < n; column += 4)
        {
            Vertex v;
            v.mPosition = glm::vec3(column * offset - dimension * 0.5f, 0.0f, (n + 1) * offset - dimension * 0.5f);
            v.mNormal = glm::vec3(0, 1, 0);
            v.mUV = glm::vec2(column / float(n - 1), n / float(n - 1));
            vertices.push_back(v);
        }

        Vertex v;
        v.mPosition = glm::vec3((n + 1) * offset - dimension * 0.5f, 0.0f, (n + 1) * offset - dimension * 0.5f);
        v.mNormal = glm::vec3(0, 1, 0);
        v.mUV = glm::vec2(1.0f, 1.0f);
        vertices.push_back(v);

        for (int column = 0; column < n; column += 4)
        {
            // last row
            int row = n - 1;

            if (column > 0)
            {
                indices.push_back(row * n + column);
                indices.push_back(row * n + (column + 1));
                indices.push_back(endIdx + (column / 4));
            }
            indices.push_back(row * n + (column + 1));
            indices.push_back(row * n + (column + 2));
            indices.push_back(endIdx + (column / 4));

            if (((endIdx + (column / 4) + 1) < vertices.size()))
            {
                if ((column + 2) < n)
                {
                    indices.push_back(row * n + (column + 2));
                    indices.push_back(endIdx + (column / 4));
                    indices.push_back(endIdx + (column / 4) + 1);

                    if ((column + 3) < n)
                    {
                        indices.push_back(row * n + (column + 2));
                        indices.push_back(row * n + (column + 3));
                        indices.push_back(endIdx + (column / 4) + 1);

                        if ((column + 4) < n)
                        {
                            indices.push_back(row * n + (column + 3));
                            indices.push_back(row * n + (column + 4));
                            indices.push_back(endIdx + (column / 4) + 1);
                        }
                    }
                }

            }
        }
    }


    void generateVerticalStitchingTriangles(
        const uint32_t        n,
        const float           dimension,
        std::vector<Vertex>   &vertices,
        std::vector<uint32_t> &indices)
    {
        float offset = (dimension + 1) / (n + 1);
        const uint32_t endIdx = vertices.size();

        // stitching triangles for neighboring patches
        for (int row = 0; row < n; row += 4)
        {
            Vertex v;
            v.mPosition = glm::vec3((n+1) * offset - dimension * 0.5f, 0.0f, row * offset - dimension * 0.5f);
            v.mNormal = glm::vec3(0, 1, 0);
            v.mUV = glm::vec2(n / float(n - 1), row / float(n - 1));
            vertices.push_back(v);
        }

        Vertex v;
        v.mPosition = glm::vec3((n + 1) * offset - dimension * 0.5f, 0.0f, (n + 1) * offset - dimension * 0.5f);
        v.mNormal = glm::vec3(0, 1, 0);
        v.mUV = glm::vec2(1.0f, 1.0f);
        vertices.push_back(v);

        for (int row = 0; row < n; row += 4)
        {
            int column = n - 1;
            if (row > 0)
            {
                indices.push_back(row * n + column);
                indices.push_back(endIdx + (row / 4));
                indices.push_back((row + 1) * n + (column));
            }
            indices.push_back((row+1) * n + (column));
            indices.push_back(endIdx + (row / 4));
            indices.push_back((row+2) * n + (column));

            if (((endIdx + (row / 4) + 1) < vertices.size()))
            {
                if ((row + 2) < n)
                {
                    indices.push_back((row + 2) * n + (column));
                    indices.push_back(endIdx + (row / 4));
                    indices.push_back(endIdx + (row / 4) + 1);

                    if ((row + 3) < n)
                    {
                        indices.push_back((row + 2) * n + (column));
                        indices.push_back((row + 3) * n + (column));
                        indices.push_back(endIdx + (row / 4) + 1);

                        if ((row + 4) < n)
                        {
                            indices.push_back((row + 3) * n + (column));
                            indices.push_back((row + 4) * n + (column));
                            indices.push_back(endIdx + (row / 4) + 1);
                        }
                    }
                }
            }
        }
    }

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
        , mLowestLevel(10)
    {

    }

    void generateGeometry()
    {
        for (int i = 0; i < mLevels; ++i)
        {
            // patch count along each axis (square)
            int n = (pow(2, mLowestLevel - i)) - 1;
            float dimension = (pow(2, mLowestLevel + i)) - 1;
#ifdef _DEBUG
            printf("%d/%d | %d %f\n", i, mLevels, n, dimension);
#endif
            const uint32_t idx = mClipmapPatches.size();
            mClipmapPatches.push_back(std::make_unique<ClipmapPatch>(dimension, dimension));
            mClipmapPatches[idx]->generatePatch(i, mLevels, mLowestLevel, n, dimension);
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
    uint32_t mLowestLevel;
    uint32_t mLevels;
    uint32_t mWidth;
    uint32_t mHeight;

    std::vector<std::unique_ptr<ClipmapPatch>> mClipmapPatches;
};