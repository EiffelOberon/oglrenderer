#ifndef PERLIN_H
#define PERLIN_H

#include "deviceconstants.h"


vec4 permute(vec4 x) 
{ 
    return mod(((x * 34.0f) + 1.0f) * x, 289.0f); 
}


vec2 fade(vec2 t) 
{ 
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); 
}

vec3 fade(vec3 t) 
{ 
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); 
}


vec4 taylorInvSqrt(vec4 r) 
{ 
    return 1.79284291400159f - 0.85373472095314f * r; 
}


vec3 hash(vec3 p)
{
    p = vec3(dot(p, vec3(127.1f, 311.7f, 74.7f)),
        dot(p, vec3(269.5f, 183.3f, 246.1f)),
        dot(p, vec3(113.5f, 271.9f, 124.6f)));

    return -1.0f + 2.0f * fract(sin(p) * 43758.5453123f);
}


vec3 hash33(vec3 p)
{
#define UI0 1597334673U
#define UI1 3812015801U
#define UI2 uvec2(UI0, UI1)
#define UI3 uvec3(UI0, UI1, 2798796415U)
#define UIF (1.0 / float(0xffffffffU))
    uvec3 q = uvec3(ivec3(p)) * UI3;
    q = (q.x ^ q.y ^ q.z) * UI3;
    return -1. + 2. * vec3(q) * UIF;
}

float perlin2D(vec2 P) 
{
    vec4 Pi = floor(P.xyxy) + vec4(0.0f, 0.0f, 1.0f, 1.0f);
    vec4 Pf = fract(P.xyxy) - vec4(0.0f, 0.0f, 1.0f, 1.0f);
    Pi = mod(Pi, 289.0f); // To avoid truncation effects in permutation
    vec4 ix = Pi.xzxz;
    vec4 iy = Pi.yyww;
    vec4 fx = Pf.xzxz;
    vec4 fy = Pf.yyww;
    vec4 i = permute(permute(ix) + iy);
    vec4 gx = 2.0f * fract(i * 0.0243902439f) - 1.0f; // 1/41 = 0.0f24...
    vec4 gy = abs(gx) - 0.5f;
    vec4 tx = floor(gx + 0.5f);
    gx = gx - tx;
    vec2 g00 = vec2(gx.x, gy.x);
    vec2 g10 = vec2(gx.y, gy.y);
    vec2 g01 = vec2(gx.z, gy.z);
    vec2 g11 = vec2(gx.w, gy.w);
    vec4 norm = 
        1.79284291400159f - 0.85373472095314f *
        vec4(dot(g00, g00), dot(g01, g01), dot(g10, g10), dot(g11, g11));
    g00 *= norm.x;
    g01 *= norm.y;
    g10 *= norm.z;
    g11 *= norm.w;
    float n00 = dot(g00, vec2(fx.x, fy.x));
    float n10 = dot(g10, vec2(fx.y, fy.y));
    float n01 = dot(g01, vec2(fx.z, fy.z));
    float n11 = dot(g11, vec2(fx.w, fy.w));
    vec2 fade_xy = fade(Pf.xy);
    vec2 n_x = mix(vec2(n00, n01), vec2(n10, n11), fade_xy.x);
    float n_xy = mix(n_x.x, n_x.y, fade_xy.y);
    return 2.3 * n_xy;
}


float perlin3D(vec3 P)
{
    vec3 Pi0 = floor(P); // Integer part for indexing
    vec3 Pi1 = Pi0 + vec3(1.0f); // Integer part + 1
    Pi0 = mod(Pi0, 289.0f);
    Pi1 = mod(Pi1, 289.0f);
    vec3 Pf0 = fract(P); // Fractional part for interpolation
    vec3 Pf1 = Pf0 - vec3(1.0f); // Fractional part - 1.0f
    vec4 ix = vec4(Pi0.x, Pi1.x, Pi0.x, Pi1.x);
    vec4 iy = vec4(Pi0.yy, Pi1.yy);
    vec4 iz0 = Pi0.zzzz;
    vec4 iz1 = Pi1.zzzz;
    
    vec4 ixy = permute(permute(ix) + iy);
    vec4 ixy0 = permute(ixy + iz0);
    vec4 ixy1 = permute(ixy + iz1);
    
    vec4 gx0 = ixy0 / 7.0f;
    vec4 gy0 = fract(floor(gx0) / 7.0f) - 0.5f;
    gx0 = fract(gx0);
    vec4 gz0 = vec4(0.5f) - abs(gx0) - abs(gy0);
    vec4 sz0 = step(gz0, vec4(0.0f));
    gx0 -= sz0 * (step(0.0f, gx0) - 0.5f);
    gy0 -= sz0 * (step(0.0f, gy0) - 0.5f);
    
    vec4 gx1 = ixy1 / 7.0f;
    vec4 gy1 = fract(floor(gx1) / 7.0f) - 0.5f;
    gx1 = fract(gx1);
    vec4 gz1 = vec4(0.5f) - abs(gx1) - abs(gy1);
    vec4 sz1 = step(gz1, vec4(0.0f));
    gx1 -= sz1 * (step(0.0f, gx1) - 0.5f);
    gy1 -= sz1 * (step(0.0f, gy1) - 0.5f);
    
    vec3 g000 = vec3(gx0.x,gy0.x,gz0.x);
    vec3 g100 = vec3(gx0.y,gy0.y,gz0.y);
    vec3 g010 = vec3(gx0.z,gy0.z,gz0.z);
    vec3 g110 = vec3(gx0.w,gy0.w,gz0.w);
    vec3 g001 = vec3(gx1.x,gy1.x,gz1.x);
    vec3 g101 = vec3(gx1.y,gy1.y,gz1.y);
    vec3 g011 = vec3(gx1.z,gy1.z,gz1.z);
    vec3 g111 = vec3(gx1.w,gy1.w,gz1.w);
    
    vec4 norm0 = taylorInvSqrt(vec4(dot(g000, g000), dot(g010, g010), dot(g100, g100), dot(g110, g110)));
    g000 *= norm0.x;
    g010 *= norm0.y;
    g100 *= norm0.z;
    g110 *= norm0.w;
    vec4 norm1 = taylorInvSqrt(vec4(dot(g001, g001), dot(g011, g011), dot(g101, g101), dot(g111, g111)));
    g001 *= norm1.x;
    g011 *= norm1.y;
    g101 *= norm1.z;
    g111 *= norm1.w;
    
    float n000 = dot(g000, Pf0);
    float n100 = dot(g100, vec3(Pf1.x, Pf0.yz));
    float n010 = dot(g010, vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = dot(g110, vec3(Pf1.xy, Pf0.z));
    float n001 = dot(g001, vec3(Pf0.xy, Pf1.z));
    float n101 = dot(g101, vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = dot(g011, vec3(Pf0.x, Pf1.yz));
    float n111 = dot(g111, Pf1);
    
    vec3 fade_xyz = fade(Pf0);
    vec4 n_z = mix(vec4(n000, n100, n010, n110), vec4(n001, n101, n011, n111), fade_xyz.z);
    vec2 n_yz = mix(n_z.xy, n_z.zw, fade_xyz.y);
    float n_xyz = mix(n_yz.x, n_yz.y, fade_xyz.x); 
    return 2.2f * n_xyz;
}


// 3D Gradient noise by iq.
float perlin3D2(vec3 p)
{
    const vec3 i = floor(p);
    const vec3 f = fract(p);

    const vec3 u = f * f * (3.0f - 2.0f * f);

    return mix(mix(mix(dot(hash(i + vec3(0.0f, 0.0f, 0.0f)), f - vec3(0.0f, 0.0f, 0.0f)),
        dot(hash(i + vec3(1.0f, 0.0f, 0.0f)), f - vec3(1.0f, 0.0f, 0.0f)), u.x),
        mix(dot(hash(i + vec3(0.0f, 1.0f, 0.0f)), f - vec3(0.0f, 1.0f, 0.0f)),
            dot(hash(i + vec3(1.0f, 1.0f, 0.0f)), f - vec3(1.0f, 1.0f, 0.0f)), u.x), u.y),
        mix(mix(dot(hash(i + vec3(0.0f, 0.0f, 1.0f)), f - vec3(0.0f, 0.0f, 1.0f)),
            dot(hash(i + vec3(1.0f, 0.0f, 1.0f)), f - vec3(1.0f, 0.0f, 1.0f)), u.x),
            mix(dot(hash(i + vec3(0.0f, 1.0f, 1.0f)), f - vec3(0.0f, 1.0f, 1.0f)),
                dot(hash(i + vec3(1.0f, 1.0f, 1.0f)), f - vec3(1.0f, 1.0f, 1.0f)), u.x), u.y), u.z);
}



// Gradient noise by iq (modified to be tileable)
float gradientNoise(vec3 x, float freq)
{
    // grid
    vec3 p = floor(x);
    vec3 w = fract(x);
    
    // quintic interpolant
    vec3 u = w * w * w * (w * (w * 6. - 15.) + 10.);

    
    // gradients
    vec3 ga = hash33(mod(p + vec3(0., 0., 0.), freq));
    vec3 gb = hash33(mod(p + vec3(1., 0., 0.), freq));
    vec3 gc = hash33(mod(p + vec3(0., 1., 0.), freq));
    vec3 gd = hash33(mod(p + vec3(1., 1., 0.), freq));
    vec3 ge = hash33(mod(p + vec3(0., 0., 1.), freq));
    vec3 gf = hash33(mod(p + vec3(1., 0., 1.), freq));
    vec3 gg = hash33(mod(p + vec3(0., 1., 1.), freq));
    vec3 gh = hash33(mod(p + vec3(1., 1., 1.), freq));
    
    // projections
    float va = dot(ga, w - vec3(0., 0., 0.));
    float vb = dot(gb, w - vec3(1., 0., 0.));
    float vc = dot(gc, w - vec3(0., 1., 0.));
    float vd = dot(gd, w - vec3(1., 1., 0.));
    float ve = dot(ge, w - vec3(0., 0., 1.));
    float vf = dot(gf, w - vec3(1., 0., 1.));
    float vg = dot(gg, w - vec3(0., 1., 1.));
    float vh = dot(gh, w - vec3(1., 1., 1.));
	
    // interpolation
    return va + 
           u.x * (vb - va) + 
           u.y * (vc - va) + 
           u.z * (ve - va) + 
           u.x * u.y * (va - vb - vc + vd) + 
           u.y * u.z * (va - vc - ve + vg) + 
           u.z * u.x * (va - vb - ve + vf) + 
           u.x * u.y * u.z * (-va + vb + vc - vd + ve - vf - vg + vh);
}

#endif