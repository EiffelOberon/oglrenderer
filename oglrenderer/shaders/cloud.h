#ifndef CLOUD_H
#define CLOUD_H

#include "fbm.h"
#include "perlin.h"
#include "worley.h"


float remap(
    const float x,
    const float a,
    const float b,
    const float c,
    const float d)
{
    return (((x - a) / (b - a)) * (d - c)) + c;
}


float worleyFBM(
    const vec3  st,
    const float time,
    const float freq,
    const bool  invert)
{
    const float noise =
        worley3D(st * freq, time * freq, invert) * 0.625f +
        worley3D(st * 2 * freq, time * 2 * freq, invert) * 0.25f +
        worley3D(st * 4 * freq, time * 4 * freq, invert) * 0.125f;
    return noise;
}


float perlinFBM(
    const vec3  st,
    const float time,
    float       freq,
    const int   octaves)
{
    const float G = exp2(-0.85f);
    float amp = 1.0f;
    float noise = 0;
    for (int i = 0; i < octaves; ++i)
    {
        noise += amp * gradientNoise(st * freq + time, freq);
        freq *= 2;
        amp *= G;
    }
    return noise;
}


float perlinWorley3D(
    const vec3  st,
    const float time,
    float       freq,
    const int   octaves,
    const bool  invert)
{
    const float perlinNoise = perlinFBM(st, time, freq, octaves);
    const float worleyNoise = worleyFBM(st, time, freq, invert);
    float noise = remap(abs(perlinNoise * 2 - 1), (1 - worleyNoise), 1.0f, 0.0f, 1.0f);
    return 1-noise;
}

#endif