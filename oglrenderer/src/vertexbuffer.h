#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"

#include <vector>

struct Vertex
{
    glm::vec3 mPosition;
    glm::vec3 mNormal;
    glm::vec2 mUV;
    uint32_t  mMaterialId;
};


class VertexBuffer
{
public:
    VertexBuffer()
        : mTriangleCount(0)
        , mIBO(0)
        , mVAO(0)
        , mVBO(0)
    {
        glGenBuffers(1, &mVBO);
        glGenBuffers(1, &mIBO);
        glGenVertexArrays(1, &mVAO);
    }


    ~VertexBuffer()
    {
        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindVertexArray(0);

        glDeleteBuffers(1, &mVBO);
        glDeleteBuffers(1, &mIBO);
        glDeleteVertexArrays(1, &mVAO);
    }


    void update(
        uint32_t vertexDataSizeInBytes,
        uint32_t indexDataSizeInBytes,
        void     *data, 
        void     *indexData)
    {
        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, vertexDataSizeInBytes, data, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(nullptr));
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3) * 2));

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);
        glBindVertexArray(0);

        mTriangleCount = indexDataSizeInBytes / sizeof(uint32_t);
    }


    void draw()
    {
        if (mTriangleCount > 0)
        {
            glBindVertexArray(mVAO);
            glDrawElements(GL_TRIANGLES, mTriangleCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
        }
    }


private: 
    GLuint mIBO;
    GLuint mVBO;
    GLuint mVAO;

    uint32_t mTriangleCount;
};