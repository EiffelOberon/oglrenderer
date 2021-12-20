#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/rotate_vector.hpp"

class Camera
{
public:
    Camera()
        : mEye(0.0f, 2.0f, 5.0f)
        , mTarget(0.0f, 1.0f, 0.0f)
        , mUp(0.0f, 1.0f, 0.0f)
    {
    }


    ~Camera()
    {

    }


    void update(
        const float deltaX,
        float       deltaY)
    {
        // get the homogenous position of the camera and pivot point
        glm::vec4 position(mEye.x, mEye.y, mEye.z, 1);
        glm::vec4 pivot(mTarget.x, mTarget.y, mTarget.z, 1);

        float xAngle = -deltaX;
        float yAngle = -deltaY;

        // extra step to handle the problem when the camera direction is the same as the up vector
        float cosAngle = dot(normalize(mTarget - mEye), normalize(mUp));
        if (cosAngle * glm::sign(yAngle) > 0.99f)
        {
            yAngle = 0;
        }

        // rotate the camera around the pivot point on the first axis.
        glm::mat4x4 rotationMatrixX(1.0f);
        rotationMatrixX = glm::rotate(rotationMatrixX, xAngle, mUp);
        position = (rotationMatrixX * (position - pivot)) + pivot;

        // rotate the camera around the pivot point on the second axis.
        glm::mat4x4 rotationMatrixY(1.0f);
        glm::vec3 rightVector = normalize(cross(normalize(mTarget - mEye), normalize(mUp)));
        rotationMatrixY = glm::rotate(rotationMatrixY, yAngle, rightVector);
        glm::vec3 finalPosition = (rotationMatrixY * (position - pivot)) + pivot;

        mEye = finalPosition;
    }


    void updateZoom(
        const int dir)
    {
        glm::vec3 forward = (mTarget - mEye);
        float dist = length(forward);
        dist -= dir;
        dist = glm::clamp(dist, 0.01f, 1000.0f);

        mEye = mTarget - normalize(forward) * dist;
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