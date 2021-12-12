#pragma once

#include <iostream>
#include <map>
#include <memory>

#include "glew.h"
#include "shader.h"
#include "uniformbuffer.h"


class ShaderProgram
{
public:

    // Constructor
    ShaderProgram()
    {
        // Generate a unique Id / handle for the shader program
        // Note: We MUST have a valid rendering context before generating
        // the programId or it causes a segfault!
        mProgramId = glCreateProgram();

        // Initially, we have zero shaders attached to the program
        mShaderCount = 0;
    }

    
    // Vertex / Fragment constructor
    ShaderProgram(
        const std::string &vertPath,
        const std::string &fragPath)
    {
        // Generate a unique Id / handle for the shader program
        // Note: We MUST have a valid rendering context before generating
        // the programId or it causes a segfault!
        mProgramId = glCreateProgram();

        // Initially, we have zero shaders attached to the program
        mShaderCount = 0;

        // reserve for vertex and fragment
        mAttachedShaders.reserve(2);

        // vertex
        mAttachedShaders.push_back(std::make_unique<Shader>(GL_VERTEX_SHADER));
        mShaderCount = (GLuint) mAttachedShaders.size();
        mAttachedShaders[mShaderCount - 1]->loadSPIRVFromFile(vertPath);
        mAttachedShaders[mShaderCount - 1]->compile();
        glAttachShader(mProgramId, mAttachedShaders[mShaderCount - 1]->id());

        // fragment
        mAttachedShaders.push_back(std::make_unique<Shader>(GL_FRAGMENT_SHADER));
        mShaderCount = (GLuint) mAttachedShaders.size();
        mAttachedShaders[mShaderCount - 1]->loadSPIRVFromFile(fragPath);
        mAttachedShaders[mShaderCount - 1]->compile();
        glAttachShader(mProgramId, mAttachedShaders[mShaderCount - 1]->id());

        linkProgram();
    }


    // Destructor
    ~ShaderProgram()
    {
        for (int i = 0; i < mAttachedShaders.size(); ++i)
        {
            glDetachShader(mProgramId, mAttachedShaders[i]->id());
        }
        // Delete the shader program from the graphics card memory to
        // free all the resources it's been using
        glDeleteProgram(mProgramId);
    }


    template<class UniformType>
    void addUniform(
        uint32_t    bindingPt,
        UniformType &memoryBlock)
    {
        if (mUniforms.find(bindingPt) == mUniforms.end())
        {
            mUniforms[bindingPt] = std::make_unique<UniformBuffer>(sizeof(UniformType), bindingPt);
            mUniforms[bindingPt]->upload((void*)&memoryBlock);
        }
        else
        {
            // should not be adding multiple uniforms to the same binding point
            assert(false);
        }
    }


    template<class UniformType>
    void updateUniform(
        uint32_t     bindingPt,
        UniformType& memoryBlock)
    {
        mUniforms[bindingPt]->upload((void*)&memoryBlock);
    }


    template<class UniformType>
    void updateUniform(
        uint32_t     bindingPt,
        uint32_t     offsetInBytes,
        uint32_t     sizeInBytes,
        UniformType& memoryBlock)
    {
        mUniforms[bindingPt]->upload(offsetInBytes, sizeInBytes, (void*)&memoryBlock);
    }


    // Method to enable the shader program
    void use() const
    {
        glUseProgram(mProgramId);
    }


    // Method to disable the shader program
    void disable() const
    {
        glUseProgram(0);
    }


    GLuint id() const
    {
        return mProgramId;
    }

private:
    
    // Method to link the shader program and display the link status
    void linkProgram()
    {
        // Perform the linking process
        glLinkProgram(mProgramId);

        // Check the status
        GLint linkStatus;
        glGetProgramiv(mProgramId, GL_LINK_STATUS, &linkStatus);
        if (linkStatus == GL_FALSE)
        {
            GLenum e = glGetError();
            std::cout << "Shader program linking failed." << e << std::endl;
        }
        else
        {
            cout << "Shader program linking OK." << endl;
        }
    }

    // Variables keeping track of the program
    GLuint mProgramId;       // The unique ID / handle for the shader program
    GLuint mShaderCount;     // How many shaders are attached to the shader program

     // List of attached shaders and uniforms to the program
    std::vector<std::unique_ptr<Shader>>               mAttachedShaders;
    std::map<uint32_t, std::unique_ptr<UniformBuffer>> mUniforms;
};