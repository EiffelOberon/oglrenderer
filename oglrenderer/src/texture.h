#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"


class Texture
{
public:

    Texture(
        int            width,
        int            height,
        const uint32_t sampleMode,
        const int      bitsPerChannel = 8,
        const bool     greyScale      = false,
        const void*    data           = nullptr)
        : mWidth(width)
        , mHeight(height)
        , mInternalFormat(GL_RGBA8)
        , mIsGreyScale(greyScale)
    {
        glGenTextures(1, &mTex);
        glBindTexture(GL_TEXTURE_2D, mTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, sampleMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, sampleMode);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        assert(bitsPerChannel == 8 || bitsPerChannel == 32);

        switch (bitsPerChannel)
        {
        case 8:  mInternalFormat = greyScale ? GL_R8 : GL_RGBA8; break;
        case 32: mInternalFormat = greyScale ? GL_R32F : GL_RGBA32F; break;
        default: assert(false);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, mInternalFormat, width, height, 0, greyScale ? GL_R : GL_RGBA, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~Texture()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &mTex);
    }


    virtual void bindTexture(
        const uint32_t texUnit)
    {
        glBindTextureUnit(texUnit, mTex);
    }


    virtual void bindImageTexture(
        const uint32_t texUnit,
        const uint32_t readWriteState)
    {
        glBindImageTexture(texUnit, mTex, 0, GL_FALSE, 0, readWriteState, mInternalFormat);
    }


    void uploadData(
        void * data)
    {
        glBindTexture(GL_TEXTURE_2D, mTex);
        glTexImage2D(GL_TEXTURE_2D, 0, mInternalFormat, mWidth, mHeight, 0, mIsGreyScale ? GL_R : GL_RGBA, GL_FLOAT, data);
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    GLuint texId()
    {
        return mTex;
    }


    int channelCount()
    {
        switch (mInternalFormat)
        {
        case GL_RGBA8:
        case GL_RGBA32F:
            return 4;
        case GL_R8:
        case GL_R32F:
            return 1;
        default:
            assert(false);
            return 4;
        }
    }


    GLuint channelType()
    {
        switch (mInternalFormat)
        {
        case GL_RGBA8:
        case GL_R8:
            return GL_UNSIGNED_BYTE;
        case GL_RGBA32F:
        case GL_R32F:
            return GL_FLOAT;
        default:
            assert(false);
            return GL_UNSIGNED_BYTE;
        }
    }


    GLuint format()
    {
        switch (mInternalFormat)
        {
        case GL_RGBA8:
        case GL_RGBA32F:
            return GL_RGBA;
        case GL_R8:
        case GL_R32F:
            return GL_R;
        default:
            assert(false);
            return GL_RGBA;
        }
    }

protected:
    Texture(){}

    GLuint mTex;
    GLuint mInternalFormat;

    int mWidth;
    int mHeight;

    bool mIsGreyScale;
};


class Texture3D : public Texture
{
public:
    Texture3D(
        int          width,
        int          height,
        int          depth,
        const int    bitsPerChannel = 8,
        const bool   greyScale = false)
    {
        mWidth          = width;
        mHeight         = height;
        mDepth          = depth;
        mInternalFormat = GL_RGBA8;

        glGenTextures(1, &mTex);
        glBindTexture(GL_TEXTURE_3D, mTex);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT);

        assert(bitsPerChannel == 8 || bitsPerChannel == 32);

        switch (bitsPerChannel)
        {
        case 8:  mInternalFormat = greyScale ? GL_R8 : GL_RGBA8; break;
        case 32: mInternalFormat = greyScale ? GL_R32F : GL_RGBA32F; break;
        default: assert(false);
        }

        glTexImage3D(GL_TEXTURE_3D, 0, mInternalFormat, width, height, depth, 0, greyScale ? GL_R : GL_RGBA, GL_FLOAT, 0);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    ~Texture3D()
    {
        glBindTexture(GL_TEXTURE_3D, 0);
        glDeleteTextures(1, &mTex);
    }


    void bindTexture(
        const uint32_t texUnit) override
    {
        glBindTextureUnit(texUnit, mTex);
    }


    void bindImageTexture(
        const uint32_t texUnit,
        const uint32_t readWriteState) override
    {
        glBindImageTexture(texUnit, mTex, 0, GL_TRUE, 0, readWriteState, mInternalFormat);
    }

private:
    int mDepth;
};