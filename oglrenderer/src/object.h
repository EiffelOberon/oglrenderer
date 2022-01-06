#pragma once

#include <memory>
#include "glm/glm.hpp"
#include "vertexbuffer.h"

struct Transform
{
    glm::mat4 mModelMatrix;
};


class Object
{
public:
    Object(
        const std::string &name)
        : mIsDrawable(false)
    {
        mName = name;
        mParent = nullptr;
        mTransform.mModelMatrix = glm::mat4(1.0f);
    }


    ~Object()
    {

    }


    Object* addChild(
        std::unique_ptr<Object> child)
    {
        child->mParent = this;
        const uint32_t idx = mChildren.size();
        mChildren.push_back(std::move(child));
        return mChildren.at(idx).get();
    }


    Object* parent()
    {
        return mParent;
    }


    Object* child(
        const uint32_t idx)
    {
        return mChildren[idx].get();
    }


    uint32_t childCount() const
    {
        return mChildren.size();
    }


    glm::mat4 transform() const
    {
        return mTransform.mModelMatrix;
    }


    bool isDrawable() const
    {
        return mIsDrawable;
    }

    
    const char* const name() const
    {
        return mName.c_str();
    }

protected:
    bool                                 mIsDrawable;
    std::string                          mName;
    Transform                            mTransform;
    Object                               *mParent;
    std::vector<std::unique_ptr<Object>> mChildren;
};


class Mesh : public Object
{
public:
    Mesh(
        const std::string &name)
        : Object(name)
    {
        mIsDrawable = true;
    }

    ~Mesh()
    {

    }


    void update(
        std::vector<Vertex>   &vertexList,
        std::vector<uint32_t> &indexList)
    {
        mVertexBuffer.update(
            sizeof(Vertex) * vertexList.size(),
            sizeof(uint32_t) * indexList.size(),
            vertexList.data(),
            indexList.data());
    }


    void draw()
    {
        mVertexBuffer.draw();
    }

private:
    VertexBuffer mVertexBuffer;
};