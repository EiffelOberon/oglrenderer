#ifndef WORLEY_H
#define WORLEY_H

vec2 random2(const vec2 p) 
{
    return fract(sin(vec2(dot(p, vec2(127.1, 311.7)), 
                          dot(p, vec2(269.5, 183.3)))) * 43758.5453);
}

float worley(
    const vec2 iUV,
    const vec2 fUV,
    const float time)
{
    // minimum distance
    float minDist = 1.;
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


#endif