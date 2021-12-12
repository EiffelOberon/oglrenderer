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


    void bindTexture2D(
        const uint32_t slot) const
    {
        glBindTexture(GL_TEXTURE_2D, mTex[slot]);
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

private:
    GLuint mFbo;
    std::vector<GLuint> mTex;
    GLuint mRbo;

    int mWidth;
    int mHeight;
    int mCount;
};