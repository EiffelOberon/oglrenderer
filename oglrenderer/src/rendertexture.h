#pragma once

#include <vector>

#include "GL/glew.h"
#include "glm/glm.hpp"

class RenderTexture
{
public:

    RenderTexture(
        int count,
        int width,
        int height)
        : mWidth(width)
        , mHeight(height)
        , mCount(count)
    {
        mTex.resize(count);

        GLenum t = glGetError();
        glGenTextures(count, &mTex[0]);
        for (int i = 0; i < count; ++i)
        {
            glBindTexture(GL_TEXTURE_2D, mTex[i]);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            
            // TODO: make something else other than 32bit floating point textures
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenRenderbuffers(1, &mRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, mRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glGenFramebuffers(1, &mFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        for (int i = 0; i < count; ++i)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTex[i], 0);
        }

        std::vector<GLenum> drawBuffers;
        for (int i = 0; i < count; ++i)
        {
            drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
        }
        glDrawBuffers(count, &drawBuffers[0]);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRbo);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            assert(false);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~RenderTexture()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        for (int i = 0; i < mCount; ++i)
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, 0, 0);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteTextures(mCount, &mTex[0]);
        glDeleteRenderbuffers(1, &mRbo);
        glDeleteFramebuffers(1, &mFbo);
    }


    virtual void bindTexture(
        const uint32_t texUnit,
        const uint32_t slot) const
    {
        glBindTextureUnit(texUnit, mTex[slot]);
    }


    GLuint getTextureId(
        const uint32_t slot) const
    {
        return mTex[slot];
    }


    void bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
    }


    void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    int width() const
    {
        return mWidth;
    }


    int height() const
    {
        return mHeight;
    }

protected:
    RenderTexture()
    {

    }

    GLuint mFbo;
    std::vector<GLuint> mTex;
    GLuint mRbo;

    int mWidth;
    int mHeight;

private:
    int mCount;
};



class RenderCubemapTexture : public RenderTexture
{
public:

    RenderCubemapTexture(
        const uint32_t dimension)
    {
        mWidth = dimension;
        mHeight = dimension;
        mTex.resize(1);

        GLenum t = glGetError();
        glGenTextures(1, &mTex[0]);
        glBindTexture(GL_TEXTURE_CUBE_MAP, mTex[0]);

        for (int i = 0; i < 6; ++i)
        {
            // TODO: make something else other than 32bit floating point textures
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA32F, mWidth, mHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

        glGenRenderbuffers(1, &mRbo);
        glBindRenderbuffer(GL_RENDERBUFFER, mRbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, mWidth, mHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glGenFramebuffers(1, &mFbo);
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        for (int i = 0; i < 6; ++i)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mTex[0], 0);
        }

        std::vector<GLenum> drawBuffers;
        drawBuffers.push_back(GL_COLOR_ATTACHMENT0);
        glDrawBuffers(1, &drawBuffers[0]);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mRbo);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            assert(false);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~RenderCubemapTexture()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        for (int i = 0; i < 6; ++i)
        {
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, 0, 0);
        }
        glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glDeleteTextures(1, &mTex[0]);
        glDeleteRenderbuffers(1, &mRbo);
        glDeleteFramebuffers(1, &mFbo);
    }


    GLuint getTextureId(
        const uint32_t slot) const
    {
        return mTex[slot];
    }


    void bind(
        const uint32_t i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, mFbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, mTex[0], 0);
    }


    void unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }


    int width() const
    {
        return mWidth;
    }


    int height() const
    {
        return mHeight;
    }
};