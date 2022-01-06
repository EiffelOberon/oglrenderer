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


struct Material
{
    // x: diffuse y: specular z: roughness w: metallic
    ivec4 mTexture1;
    vec4 mDiffuse;
    vec4 mSpecular;
    // x: roughness, y: metallic, z: ior
    vec4 mShadingParams;
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

    // x, y, z: reflection w: choppiness
    vec4 mReflection;
    // x, y, z: transmission w: dampening distance
    vec4 mTransmission;
    // x, y, z: transmission w: lerp exponent
    vec4 mTransmission2;
    // x: foam scale y: foam intensity z: ocean ior w: ocean roughness
    vec4 mFoamSettings;
};


struct RendererParams
{
    // x = time, y = aspect ratio, z = bit flag 1 for pre-process, w = empty;
    vec4 mSettings;
    // x = anisotropy, y = speed, z = density, w = height;
    vec4 mCloudSettings;
    // x = u, y = v, z = coverage, w = empty;
    vec4 mCloudMapping;
    // x = absorption, y, z, w = empty;
    vec4 mCloudAbsorption;
    // x = horizontal width, y = vertical width, z = frame count, w = mDrawCall index
    ivec4 mScreenSettings;
    // x = max steps, y = shadow max steps, z = empty, w = empty;
    ivec4 mSteps;
};


struct SceneObjectParams
{
    // x: model matrix index
    ivec4 mIndices;
};


struct SkyParams
{
    // x, y, z: dir w: intensity
    vec4 mSunSetting;
    // x, y, z: luminance w: empty
    vec4 mSunLuminance;
    // x: rayleigh intensity, y: mie intensity
    vec4 mNishitaSetting;
    // x: min fog dist, y: max fog dist, z,w: empty
    vec4 mFogSettings;
    // x: i-th cubemap texture, y: active model z: empty w: empty
    ivec4 mPrecomputeSettings;
    // x: roughness y: roughness test z: ior test w: metallic test
    vec4 mPrecomputeGGXSettings;
};


struct ViewProjectionMatrix
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