#ifndef RANDOM_H
#define RANDOM_H

float random(const vec2 st)
{
    return fract(sin(dot(st.xy,
        vec2(12.9898, 78.233))) *
        43758.5453123);
}


vec2 random2(const vec2 p)
{
    return fract(sin(vec2(dot(p, vec2(127.1f, 311.7f)),
        dot(p, vec2(269.5f, 183.3f)))) * 43758.5453f);
}


vec3 random3(const vec3 p)
{
    return fract(sin(vec3(dot(p, vec3(12.989f, 78.233f, 37.719f)),
        dot(p, vec3(39.346f, 11.135f, 83.155f)),
        dot(p, vec3(73.156f, 52.235f, 09.151f)))) * 43758.5453f);
}

#endif