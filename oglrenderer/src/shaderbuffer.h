#pragma once

#include "glew.h"
#include "glm/glm.hpp"

#include <string>


class ShaderBuffer
{
public:
    ShaderBuffer(
        const size_t sizeInBytes)
        : mSizeInBytes(sizeInBytes)
    {
        glGenBuffers(1, &mSSBO);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, mSizeInBytes, nullptr, GL_STATIC_DRAW);
        glClearNamedBufferData(mSSBO, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    ~ShaderBuffer()
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        glDeleteBuffers(1, &mSSBO);
    }


    void upload(
        const void* data)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSSBO);
        if (data)
        {
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, mSizeInBytes, data);
        }
        else
        {
            glClearNamedBufferData(mSSBO, GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE, 0);
        }
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }


    void download(
        void*  data,
        size_t sizeInBytes = 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, mSSBO);
        void * gpuData = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
        memcpy(data, gpuData, sizeInBytes == 0 ? mSizeInBytes : sizeInBytes);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    
    void bind(
        const uint32_t bindingPoint)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, mSSBO);
    }


    GLuint id()
    {
        return mSSBO;
    }


    size_t sizeInBytes()
    {
        return mSizeInBytes;
    }

private:
    GLuint      mSSBO;
    size_t      mSizeInBytes;
};