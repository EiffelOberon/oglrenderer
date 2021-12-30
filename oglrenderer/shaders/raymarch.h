#ifndef RAYMARCH_H
#define RAYMARCH_H

#include "cloud.h"

float phase(
    const float cosTheta, 
    const float g)
{
    return (1.0f - g * g) / (pow(1. + g * g - 2.0f * g * cosTheta, 1.5f) * 4.0f * PI);
}


void raymarchCloud(
    in Ray      r,
    in Box      b,
    const float offset,
    const float height,
    const float density,
    const vec2  uv,
    const int   maxSteps,
    const int   shadowMaxSteps,
    in float    tMin,
    in float    tMax,
    vec3        sunPos,
    vec4        sunLuminance,
    inout bool  hasClouds,
    inout vec4  cloudColor,
    inout float transmittance)
{
    const float stepLength = ((tMax - tMin) / float(maxSteps));
    r.mOrigin = r.mOrigin + r.mDir * (tMin + stepLength * offset);

    float densityScale = density * 0.001f;
    if (tMin > 0 && tMax > tMin)
    {
        for (int i = 0; i < maxSteps; ++i)
        {
            vec3 uvw = (r.mOrigin - b.mMin) / (b.mMax - b.mMin);
            uvw.xz *= uv;

            vec4 noise = texture(cloudTexture, uvw);
            const float coverage = hash12(uvw.xz) * 0.1 + (renderParams.mCloudMapping.z * .5 + .5);
            float lowFreqFBM = noise.y * 0.625f + noise.z * 0.25f + noise.w * 0.125f;
            float base = remap(noise.x, 1.0f - coverage, 1.0f, 0.0f, 1.0f) * coverage;
            base = remap(base, (lowFreqFBM - 1.0f) * 0.64f, 1.0f, 0.0f, 1.0f);
            base = max(0.0f, base);

            Ray shadowRay;
            shadowRay.mOrigin = r.mOrigin;
            shadowRay.mDir = normalize(sunPos - shadowRay.mOrigin);

            float lightRayDensity = 0.0f;
            vec4 lightTransmittance = vec4(1.0f);

            // only do light marching if transmittance is less than 1
            const float shadowStepLength = (height * 0.5f / float(shadowMaxSteps));
            if (transmittance < 1.0f)
            {
                for (int j = 0; j < shadowMaxSteps; ++j)
                {
                    float shadowtMin = 0.0f;
                    float shadowtMax = 0.0f;
                    const bool foundIntersection = intersect(b, shadowRay, shadowtMin, shadowtMax);
                    if (foundIntersection)
                    {
                        vec3 shadowUVW = (shadowRay.mOrigin - b.mMin) / (b.mMax - b.mMin);
                        shadowUVW.xz *= uv;

                        vec4 noise = texture(cloudTexture, shadowUVW);
                        const float coverage = hash12(uvw.xz) * 0.1 + (renderParams.mCloudMapping.z * .5 + .5);
                        float lowFreqFBM = noise.y * 0.625f + noise.z * 0.25f + noise.w * 0.125f;
                        float base = remap(noise.x, 1.0f - coverage, 1.0f, 0.0f, 1.0f) * coverage;
                        base = remap(base, (lowFreqFBM - 1.0f) * 0.64f, 1.0f, 0.0f, 1.0f);
                        base = max(0.0f, base);
                        lightTransmittance *= exp(-shadowStepLength * base * densityScale * renderParams.mCloudAbsorption.x);
                        lightRayDensity += base * densityScale;
                    }
                    else
                    {
                        break;
                    }
                    shadowRay.mOrigin = shadowRay.mOrigin + shadowStepLength * shadowRay.mDir;

                    if (length(lightTransmittance) < 0.1f)
                    {
                        break;
                    }
                }
            }

            vec4 luminance = vec4(0.0f);
            for (int j = 0; j < 4; ++j)
            {
                float a = pow(0.5f, j);
                float b = pow(0.5f, j);
                float c = pow(0.5f, j);

                const float stepTransmittance = exp(-a * shadowStepLength * lightRayDensity * renderParams.mCloudAbsorption.x);
                luminance += b* sunLuminance* phase(dot(shadowRay.mDir, r.mDir), c * renderParams.mCloudSettings.x)* stepTransmittance;
            }
            luminance *= base;

            float rayTransmittance = exp(-stepLength * base * densityScale * renderParams.mCloudAbsorption.x);
            vec4 integralScattering = (luminance - luminance * rayTransmittance) / max(0.01f, base * renderParams.mCloudAbsorption.x);
            cloudColor += (integralScattering * transmittance);
            transmittance *= rayTransmittance;

            if (length(transmittance) < 0.1f)
            {
                break;
            }

            r.mOrigin = r.mOrigin + r.mDir * stepLength;
        }
    }
}
#endif // 
