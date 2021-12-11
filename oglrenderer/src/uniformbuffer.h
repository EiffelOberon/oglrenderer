#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"


class UniformBuffer
{
public:
    UniformBuffer(
        const size_t sizeInBytes,
        const GLint  bindingPoint = -1)
        : mSizeInBytes(sizeInBytes)
    {
        glGenBuffers(1, &mUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
        glBufferData(GL_UNIFORM_BUFFER, mSizeInBytes, nullptr, GL_STATIC_DRAW);

        if (bindingPoint >= 0)
        {
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, mUBO);
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }


    ~UniformBuffer()
    {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glDeleteBuffers(1, &mUBO);
    }


    void upload(
        void* data)
    {
        glBindBuffer(GL_UNIFORM_BUFFER, mUBO);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, mSizeInBytes, data);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

private:
    GLuint      mUBO;
    size_t      mSizeInBytes;
};