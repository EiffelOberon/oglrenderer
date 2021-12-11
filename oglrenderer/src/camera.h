#pragma once

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

class Camera
{
public:
    Camera()
        : mEye(0.0f, 0.0f, 10.0f)
        , mTarget(0.0f, 0.0f, 0.0f)
        , mUp(0.0f, 1.0f, 0.0f)
    {
        update();
    }


    ~Camera()
    {

    }


    void update()
    {
        mMVP = glm::lookAt(mEye, mTarget, mUp);
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

private:
    glm::vec3 mEye;
    glm::vec3 mTarget;
    glm::vec3 mUp;
    

    glm::mat4 mMVP;
};