#ifndef DEVICECONSTANTS_H
#define DEVICECONSTANTS_H

// uniform binding points
# define ORTHO_MATRIX    0
# define CAMERA_PARAMS   1
# define SKY_PARAMS      2
# define RENDERER_PARAMS 3
# define FBM_PARAMS      4
# define WORLEY_PARAMS   5
# define PERLIN_PARAMS   6

// texture binding points
# define SCREEN_QUAD   0
# define CLOUD_TEXTURE 1

// cloud noise texture resolution
# define CLOUD_RESOLUTION 256

// compute shader
# define PRECOMPUTE_CLOUD_LOCAL_SIZE 2

#define PI (3.1415926f)

#endif