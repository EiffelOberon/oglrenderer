#ifndef BSDF_H
#define BSDF_H

#include "deviceconstants.h"

float fresnel(
    const float ior,
    const float cosTheta)
{
    float r0 = (ior - 1.0f) / (ior + 1.0f);
    r0 *= r0;

    return r0 + (1 - r0) * pow((1 - cosTheta), 5);
}


float luminance(
    const vec3 rgb) 
{
    return dot(vec3(0.2126f, 0.7152f, 0.0722f), rgb);
}


float GGX_D(
    const vec3 wh, 
    const vec2 alpha)
{
    const vec2 He = wh.xy / alpha;
    const float denom = dot(He, He) + (wh.z * wh.z);
    return 1.0 / (PI * alpha.x * alpha.y * (denom * denom));
}


float GGX_PDF(
    const vec3  wh, 
    const float lDotH, 
    const vec2  alpha)
{
    const float nDotH = wh.z;
    return GGX_D(wh, alpha) * nDotH / (4.0 * lDotH);
}


vec3 GGX_sample(
    const vec2 u, 
    vec3       wo, 
    const vec2 alpha)
{
    // Transform the view direction to the hemisphere configuration.
    wo = normalize(vec3(wo.xy * alpha, wo.z));

    // Construct an orthonormal basis from the view direction.
    float len = length(wo.xy);
    vec3 T1 = (len > 0.0) ? vec3(-wo.y, wo.x, 0.0) / len : vec3(1.0, 0.0, 0.0);
    vec3 T2 = cross(wo, T1);

    // Parameterization of the projected area.
    float r = sqrt(u.y);
    float phi = 2.0 * PI * u.x;
    float t1 = r * cos(phi);
    float t2 = r * sin(phi);
    float s = 0.5 * (1.0 + wo.z);
    t2 = (1.0 - s) * sqrt(1.0 - (t1 * t1)) + s * t2;

    // Reprojection onto hemisphere.
    vec3 wh = t1 * T1 + t2 * T2 + sqrt(max(0.0, 1.0 - (t1 * t1) - (t2 * t2))) * wo;

    // Transform the microfacet normal back to the ellipsoid configuration.
    wh = normalize(vec3(wh.xy * alpha, max(wh.z, 0.0)));

    return wh;
}


vec3 GGX_sample_with_n(vec2 u, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0f * PI * u.x;
    float cosTheta = sqrt((1.0f - u.y) / (1.0f + (a * a - 1.0f) * u.y));
    float sinTheta = sqrt(1.0f - cosTheta * cosTheta);

    // from spherical coordinates to cartesian coordinates
    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    // from tangent-space vector to world-space sample vector
    vec3 up = abs(N.z) < 0.999 ? vec3(0.0f, 0.0f, 1.0f) : vec3(1.0f, 0.0f, 0.0f);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}


float G1(
    const float cosTheta, 
    const float alpha)
{
    if (cosTheta == 0.0f)
    {
        return 0.0f;
    }
    float cosTheta2 = (cosTheta * cosTheta);
    float tanTheta2 = (1.0 - cosTheta2) / cosTheta2;
    
    return 2.0 / (1.0 + sqrt(1.0 + (alpha * alpha) * tanTheta2));
}


float G2(
    const float nDotL, 
    const float nDotV, 
    const float alpha)
{
    if (nDotL == 0.0f || nDotV == 0.0f)
    {
        return 0.0f;
    }

    float alpha2 = (alpha * alpha);
    float lambdaL = sqrt(alpha2 + (1.0 - alpha2) * (nDotL * nDotL));
    float lambdaV = sqrt(alpha2 + (1.0 - alpha2) * (nDotV * nDotV));
    return 2.0 / (lambdaL / nDotL + lambdaV / nDotV);
}

#endif