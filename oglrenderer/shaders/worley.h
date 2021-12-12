#ifndef WORLEY_H
#define WORLEY_H

#include "random.h"

float worley(
    const vec2 iUV,
    const vec2 fUV,
    const float time)
{
    // minimum distance
    float minDist = 1.0f;
    for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
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
    return minDist;
}


float worley3D(
    const vec3 iUV,
    const vec3 fUV,
    const float time)
{
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
    return minDist;
}

#endif