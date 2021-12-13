#ifndef DEVICESTRUCTS_H
#define DEVICESTRUCTS_H

#ifndef GLSL_SHADER
# include "glm/glm.hpp"
# define vec4 glm::vec4
# define vec3 glm::vec3
# define vec2 glm::vec2
# define uint uint32_t
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
};


struct RendererParams
{
    // x = time, y = aspect ratio, z = empty, w = empty;
    vec4 mSettings;
    // x = cut off, y = speed, z = empty, w = empty;
    vec4 mCloudSettings;
};


struct NoiseParams
{
    // x: width, y: height, z: frequency, w: empty
    vec4 mSettings;
    int  mNoiseOctaves;
    bool mInvert;
    vec2 __padding__;
};


#ifndef GLSL_SHADER
# undef vec4
# undef vec3
# undef vec2
# undef uint
#endif

#endif