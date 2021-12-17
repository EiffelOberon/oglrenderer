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
# define OCEAN_PARAMS    7

// texture binding points
# define SCREEN_QUAD         0
# define CLOUD_TEXTURE       1
# define ENVIRONMENT_TEXTURE 2
# define OCEAN_NOISE         3
# define H0K_TEXTURE         4
# define H_X_TEXTURE         5
# define H_Y_TEXTURE         6
# define H_Z_TEXTURE         7
# define BUTTERFLY_TEXTURE   8
# define TEXTURES_COUNT      (BUTTERFLY_TEXTURE + 1)

// ssbo binding points
# define BUTTERFLY_INDICES   0

// texture resolution
# define CLOUD_RESOLUTION 128
# define OCEAN_RESOLUTION 256

// compute shader
# define PRECOMPUTE_CLOUD_LOCAL_SIZE       4
# define PRECOMPUTE_OCEAN_WAVES_LOCAL_SIZE 16

#define PI (3.1415926f)

#endif