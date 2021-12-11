#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"

#include <vector>

class Quad
{
public:
    Quad(
        const GLuint type,
        const int    vertexCount)
        : mType(type)
    {
        mPositions.resize(vertexCount);
        mUVs.resize(vertexCount);

        glGenBuffers(2, &mVbo[0]);
        glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * mPositions.size(), mPositions.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, mVbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * mUVs.size(), mUVs.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glGenVertexArrays(1, &mVao);
        glBindVertexArray(mVao);
        {
            glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) nullptr);
            
            glBindBuffer(GL_ARRAY_BUFFER, mVbo[1]);
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) nullptr);

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
        }
        glBindVertexArray(0);
    }


    ~Quad()
    {
        // unbind
        glBindVertexArray(mVao);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glBindVertexArray(0);

        // delete
        glDeleteBuffers(2, &mVbo[0]);
        glDeleteVertexArrays(1, &mVao);
    }


    glm::vec3& get(
        const uint32_t index)
    {
        return mPositions[index];
    }


    void upload()
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVbo[0]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * mPositions.size(), mPositions.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindBuffer(GL_ARRAY_BUFFER, mVbo[1]);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * mUVs.size(), mUVs.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }


    void update(
        const uint32_t  index,
        const glm::vec3 position,
        const glm::vec2 uv)
    {
        mPositions[index] = position;
        mUVs[index]       = uv;
    }


    void draw()
    {
        glBindVertexArray(mVao);
        glDrawArrays(mType, 0, mPositions.size());
    }


private:
    GLuint mVao;
    GLuint mVbo[2];
    GLuint mType;

    std::vector<glm::vec3> mPositions;
    std::vector<glm::vec2> mUVs;
};