#ifndef DEVICESTRUCTS_H
#define DEVICESTRUCTS_H

#ifndef GLSL_SHADER
# include "glm/glm.hpp"
# include "GL/glew.h"
# define mat4  glm::mat4
# define vec4  glm::vec4
# define vec3  glm::vec3
# define vec2  glm::vec2
# define uint  uint32_t
# define ivec4 glm::ivec4
#endif

struct CameraParams
{
    vec4 mEye;
    vec4 mTarget;
    vec4 mUp;
};


struct SkyParams
{
    vec4 mSunDir;
    // x: i-th cubemap texture, y,z,w: empty
    ivec4 mPrecomputeSettings;
};


struct RendererParams
{
    // x = time, y = aspect ratio, z = empty, w = empty;
    vec4 mSettings;
    // x = cut off, y = speed, z = density, w = height;
    vec4 mCloudSettings;
    // x = u, y = v, z = empty, w = empty;
    vec4 mCloudMapping;
    // x = max steps, y = shadow max steps, z = empty, w = empty;
    ivec4 mSteps;
};


struct NoiseParams
{
    // x: width, y: height, z: frequency, w: empty
    vec4 mSettings;
    int  mNoiseOctaves;
    int  mTextureIdx;
    int  __padding__;
    bool mInvert;
};


struct OceanParams
{
    // x: N, y: L, z: empty, w: empty
    ivec4 mHeightSettings;

    // x: ping pong, y: stage, z: direction, w: dX, dY, or dZ (0, 1, 2)
    ivec4 mPingPong;

    // x: amplitude, y: wind speed z & w: wind direction
    vec4 mWaveSettings;

    // x, y, z: transmission w: empty
    vec4 mTransmission;
    // x, y, z: transmission w: lerp exponent
    vec4 mTransmission2;
};


struct MVPMatrix
{
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
};


#ifndef GLSL_SHADER
# undef mat4
# undef vec4
# undef vec3
# undef vec2
# undef uint
# undef ivec4
#endif

#endif