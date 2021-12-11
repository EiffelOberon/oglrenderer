#pragma once

#include "GL/glew.h"
#include "glm/glm.hpp"

class Texture
{
public:
    Texture(
        int          width,
        int          height,
        const int    bitsPerChannel = 8,
        const bool   greyScale      = false,
        const GLuint texUnit        = 0)
        : mWidth(width)
        , mHeight(height)
        , mInternalFormat(GL_RGBA8)
        , mTexUnit(texUnit)
    {
        glGenTextures(1, &mTex);
        glBindTexture(GL_TEXTURE_2D, mTex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        assert(bitsPerChannel == 8 || bitsPerChannel == 32);

        switch (bitsPerChannel)
        {
        case 8:  mInternalFormat = greyScale ? GL_R8 : GL_RGBA8; break;
        case 32: mInternalFormat = greyScale ? GL_R32F : GL_RGBA32F; break;
        default: assert(false);
        }

        glTexImage2D(GL_TEXTURE_2D, 0, mInternalFormat, width, height, 0, greyScale ? GL_R : GL_RGBA, GL_FLOAT, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    ~Texture()
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDeleteTextures(1, &mTex);
    }


    void bind(
        bool readOnly = true)
    {
        // TODO: read from unit 0 for now
        if (readOnly)
        {
            glBindTextureUnit(mTexUnit, mTex);
        }
        else
        {
            glBindImageTexture(mTexUnit, mTex, 0, GL_FALSE, 0, GL_WRITE_ONLY, mInternalFormat);
        }
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

private:

    GLuint mTexUnit;
    GLuint mTex;
    GLuint mInternalFormat;

    int mWidth;
    int mHeight;
};