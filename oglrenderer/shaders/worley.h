#ifndef WORLEY_H
#define WORLEY_H

#include "random.h"

float worley(
    const vec2  uv,
    const float time,
    const bool  invert)
{
    const vec2 iUV = floor(uv);
    const vec2 fUV = fract(uv);
    // minimum distance
    float minDist = 1.0f;
    for (int y = -1; y <= 1; y++) 
    {
        for (int x = -1; x <= 1; x++) 
        {
            // Neighbor place in the grid
            const vec2 neighbor = vec2(float(x), float(y));

            // Random position from current + neighbor place in the grid
            vec2 point = random2(iUV + neighbor);

            // Animate the point
            point = 0.5 + 0.5 * sin(time + 6.2831 * point);

            // Vector between the pixel and the point
            const vec2 diff = neighbor + point - fUV;

            // Distance to the point
            float dist = length(diff);

            // Keep the closer distance
            minDist = min(minDist, dist);
        }
    }

    if (invert)
    {
        minDist = 1.0f - minDist;
        minDist = clamp(minDist, 0.0f, 1.0f);
    }

    return minDist;
}


float worley3D(
    const vec3  uv,
    const float time,
    const bool  invert)
{
    const vec3 iUV = floor(uv);
    const vec3 fUV = fract(uv);
    // minimum distance
    float minDist = 1.0f;
    for (int z = -1; z <= 1; z++)
    {
        for (int y = -1; y <= 1; y++) 
        {
            for (int x = -1; x <= 1; x++) 
            {
                // Neighbor place in the grid
                const vec3 neighbor = vec3(float(x), float(y), float(z));

                // Random position from current + neighbor place in the grid
                vec3 point = random3(iUV + neighbor);

                // Animate the point
                point = 0.5 + 0.5 * sin(time + 6.2831 * point);

                // Vector between the pixel and the point
                const vec3 diff = neighbor + point - fUV;

                // Distance to the point
                float dist = length(diff);

                // Keep the closer distance
                minDist = min(minDist, dist);
            }
        }
    }
    if (invert)
    {
        minDist = 1.0f - minDist;
        minDist = clamp(minDist, 0.0f, 1.0f);
    }
    return minDist;
}

#endif