#pragma once

#include <iostream>
#include <iterator>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <windows.h>

#include "glew.h"

using std::cout;
using std::endl;
using std::string;
using std::stringstream;
using std::ifstream;

class Shader
{
private:
    GLuint mId;              // The unique ID / handle for the shader
    GLenum mTypeSource;      // String representation of the shader type (i.e. "Vertex" or such)
    std::string mSource;     // The shader source code (i.e. the GLSL code itself)

public:
    // Constructor
    Shader(const GLenum& type)
    {
        // Get the type of the shader
        // GL_VERTEX_SHADER
        // GL_FRAGMENT_SHADER
        // GL_COMPUTE_SHADER
        mTypeSource = type;

        // Create the vertex shader id / handle
        // Note: If you segfault here you probably don't have a valid rendering context.
        mId = glCreateShader(type);
    }


    ~Shader()
    {
        glDeleteShader(mId);
    }


    GLuint id() const
    {
        return mId;
    }


    GLenum type() const
    {
        return mTypeSource;
    }


    // Method to load the shader contents from a string
    void loadSPIRVFromFile(const string& filename)
    {
        // open the file 
        std::ifstream file(filename, std::ios::binary);

        // remove new lines in binary mode
        file.unsetf(std::ios::skipws);

        // get its size
        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // reserve capacity
        std::vector<uint8_t> vec;
        vec.reserve(fileSize);

        // read the data:
        vec.insert(vec.begin(),
            std::istream_iterator<uint8_t>(file),
            std::istream_iterator<uint8_t>());

        // apply shader binary to gl shader object
        glShaderBinary(1, &mId, GL_SHADER_BINARY_FORMAT_SPIR_V, vec.data(), (GLsizei)vec.size());
    }


    // Checks if the shaders are okay to use
    void compile()
    {
        // we don't do specialization constants at the moment
        glSpecializeShader(mId, "main", 0, nullptr, nullptr);

        // Check the compilation status and report any errors
        GLint shaderStatus;
        glGetShaderiv(mId, GL_COMPILE_STATUS, &shaderStatus);

        // If the shader failed to compile, display the info log and quit out
        if (shaderStatus == GL_FALSE)
        {
            GLint infoLogLength;
            glGetShaderiv(mId, GL_INFO_LOG_LENGTH, &infoLogLength);

            GLchar* strInfoLog = new GLchar[infoLogLength + 1];
            glGetShaderInfoLog(mId, infoLogLength, NULL, strInfoLog);

            std::cout << mTypeSource << " shader compilation failed: " << strInfoLog << std::endl;
            delete[] strInfoLog;
        }
        else
        {
            cout << mTypeSource << " shader compilation OK" << endl;
        }
    }
};