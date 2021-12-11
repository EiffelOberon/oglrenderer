#pragma once

#include <iostream>
#include <map>

#include "glew.h"
#include "shader.h"

class ShaderProgram
{
protected:
    GLuint              mProgramId;       // The unique ID / handle for the shader program
    GLuint              mShaderCount;     // How many shaders are attached to the shader program
    std::vector<GLuint> mAttachedShaders; // List of attached shaders

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


    // Destructor
    ~ShaderProgram()
    {
        for (int i = 0; i < mAttachedShaders.size(); ++i)
        {
            glDetachShader(mProgramId, mAttachedShaders[i]);
        }
        // Delete the shader program from the graphics card memory to
        // free all the resources it's been using
        glDeleteProgram(mProgramId);
    }


    // Method to attach a shader to the shader program
    void attachShader(GLuint shaderId)
    {
        // Attach the shader to the program
        // Note: We identify the shader by its unique Id value
        glAttachShader(mProgramId, shaderId);

        // Increment the number of shaders we have associated with the program
        mShaderCount++;

        mAttachedShaders.push_back(shaderId);
    }


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


    // Method to enable the shader program
    void use()
    {
        glUseProgram(mProgramId);
    }


    // Method to disable the shader program
    void disable()
    {
        glUseProgram(0);
    }


    GLuint id() const
    {
        return mProgramId;
    }

};