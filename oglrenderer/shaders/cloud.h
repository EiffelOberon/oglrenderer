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


float perlinWorley3D(
    const vec3  worley,
    const vec3  st,
    const float time,
    float       freq,
    const int   octaves,
    const bool  invert)
{
    const float perlinNoise = perlinFBM(st, time, freq, octaves);
    const float worleyNoise =
        worley.x * 0.625f +
        worley.y * 0.25f +
        worley.z * 0.125f;
    float noise = remap(abs(perlinNoise * 2 - 1), (1 - worleyNoise), 1.0f, 0.0f, 1.0f);
    return 1 - noise;
}


#define CLOUD_COVERAGE .8

// Hash functions by Dave_Hoskins
float hash12(vec2 p)
{
    uvec2 q = uvec2(ivec2(p)) * uvec2(1597334673U, 3812015801U);
    uint n = (q.x ^ q.y) * 1597334673U;
    return float(n) * (1.0 / float(0xffffffffU));
}


vec2 hash22(vec2 p)
{
    uvec2 q = uvec2(ivec2(p)) * uvec2(1597334673U, 3812015801U);
    q = (q.x ^ q.y) * uvec2(1597334673U, 3812015801U);
    return vec2(q) * (1.0 / float(0xffffffffU));
}


struct Box
{
    vec3 mMin;
    vec3 mMax;
};


struct Ray
{
    vec3 mOrigin;
    vec3 mDir;
};

bool intersect(
    const in Box b,
    const in Ray r,
    out float minDist,
    out float maxDist)
{
    minDist = -1.0f;
    maxDist = -1.0f;
    float tmin = (b.mMin.x - r.mOrigin.x) / r.mDir.x;
    float tmax = (b.mMax.x - r.mOrigin.x) / r.mDir.x;
    if (tmin > tmax)
    {
        float temp = tmin;
        tmin = tmax;
        tmax = temp;
    }
    float tymin = (b.mMin.y - r.mOrigin.y) / r.mDir.y;
    float tymax = (b.mMax.y - r.mOrigin.y) / r.mDir.y;
    if (tymin > tymax)
    {
        float temp = tymin;
        tymin = tymax;
        tymax = temp;
    }
    if ((tmin > tymax) || (tymin > tmax))
    {
        return false;
    }
    if (tymin > tmin)
    {
        tmin = tymin;
    }
    if (tymax < tmax)
    {
        tmax = tymax;
    }

    float tzmin = (b.mMin.z - r.mOrigin.z) / r.mDir.z;
    float tzmax = (b.mMax.z - r.mOrigin.z) / r.mDir.z;

    if (tzmin > tzmax)
    {
        float temp = tzmin;
        tzmin = tzmax;
        tzmax = temp;
    }
    if ((tmin > tzmax) || (tzmin > tmax))
    {
        return false;
    }
    if (tzmin > tmin)
    {
        tmin = tzmin;
    }
    if (tzmax < tmax)
    {
        tmax = tzmax;
    }

    minDist = tmin;
    maxDist = tmax;
    return true;
}


#endif