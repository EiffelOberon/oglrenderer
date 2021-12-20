#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

class Camera
{
public:
    Camera()
        : mEye(0.0f, 0.0f, 2.0f)
        , mTarget(0.0f, 0.0f, 0.0f)
        , mUp(0.0f, 1.0f, 0.0f)
    {
    }


    ~Camera()
    {

    }


    void update(
        const int deltaX,
        const int deltaY)
    {
        float dist = glm::length(mTarget - mEye);
        glm::vec3 forward = glm::normalize(mTarget - mEye);
        forward = glm::rotate(forward, -deltaX / 1600.0f * 2.0f * glm::pi<float>(), mUp);

        glm::vec3 right = glm::cross(forward, mUp);
        mEye = (mTarget - forward) * dist;
        mUp = glm::normalize(glm::cross(right, forward));
        right = normalize(cross(normalize(forward), mUp));


        mUp = glm::normalize(glm::cross(right, forward));
        forward = glm::rotate(forward, -deltaY / 800.0f * 2.0f * glm::pi<float>(), right);

        mEye = (mTarget - forward) * dist;
        mUp  = glm::normalize(glm::cross(right, forward));
    }


    glm::vec3 getEye() const
    {
        return mEye;
    }


    glm::vec3 getTarget() const
    {
        return mTarget;
    }


    glm::vec3 getUp() const
    {
        return mUp;
    }


    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(mEye, mTarget, mUp);
    }

private:
    glm::vec3 mEye;
    glm::vec3 mTarget;
    glm::vec3 mUp;
    

    glm::mat4 mMVP;
    glm::mat4 mViewMatrix;
};