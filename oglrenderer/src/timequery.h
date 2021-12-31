#pragma once

#include <vector>

#include "glew.h"

// GPU time query class
class TimeQuery
{
public:
    TimeQuery(
        const uint32_t queryCount)
    {
        mQueryStarted.resize(queryCount);
        mQueryStartIds.resize(queryCount);
        glGenQueries(queryCount, &mQueryStartIds[0]);
        mQueryEndIds.resize(queryCount);
        glGenQueries(queryCount, &mQueryEndIds[0]);
    }

    ~TimeQuery()
    {
        glDeleteQueries(mQueryStartIds.size(), &mQueryStartIds[0]);
        glDeleteQueries(mQueryEndIds.size(), &mQueryEndIds[0]);
    }

    
    void start(
        const GLuint programId)
    {
        mQueryStarted[programId] = true;
        glQueryCounter(mQueryStartIds[programId], GL_TIMESTAMP);
    }


    void end(
        const GLuint programId)
    {
        glQueryCounter(mQueryEndIds[programId], GL_TIMESTAMP);
    }


    float elapsedTime(
        const GLuint programId)
    {
        if (!mQueryStarted[programId])
        {
            return 0.0f;
        }

        GLint done = false;
        while (!done)
        {
            glGetQueryObjectiv(mQueryStartIds[programId], GL_QUERY_RESULT_AVAILABLE, &done);
            if (done)
            {
                glGetQueryObjectiv(mQueryEndIds[programId], GL_QUERY_RESULT_AVAILABLE, &done);
            }
        }

        GLuint64 timerStart, timerEnd;
        glGetQueryObjectui64v(mQueryStartIds[programId], GL_QUERY_RESULT, &timerStart);
        glGetQueryObjectui64v(mQueryEndIds[programId], GL_QUERY_RESULT, &timerEnd);

        mQueryStarted[programId] = false;
        // return in milliseconds
        return (timerEnd - timerStart) / 1000000.0f;
    }

private:
    std::vector<GLuint> mQueryStartIds;
    std::vector<GLuint> mQueryEndIds;
    std::vector<bool>   mQueryStarted;
};
