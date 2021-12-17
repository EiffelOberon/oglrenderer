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
//# define SCREEN_QUAD         0
//# define CLOUD_TEXTURE       1
//# define ENVIRONMENT_TEXTURE 2
//# define OCEAN_NOISE         3
//# define H0K_TEXTURE         4
//# define H_X_TEXTURE         5
//# define H_Y_TEXTURE         6
//# define H_Z_TEXTURE         7
//# define BUTTERFLY_TEXTURE   8
//# define TEXTURES_COUNT      (BUTTERFLY_TEXTURE + 1)

// ssbo binding points
# define BUTTERFLY_INDICES   0

// texture resolution
# define CLOUD_RESOLUTION 128
# define OCEAN_RESOLUTION 256

// compute shader
# define PRECOMPUTE_CLOUD_LOCAL_SIZE       4
# define PRECOMPUTE_OCEAN_WAVES_LOCAL_SIZE 16

# define PI (3.1415926f)

//////////////////////////////////////////////////////////////////////////

// cloud noise frag
# define CLOUD_NOISE_CLOUD_TEX 1

// precompute butterfly shader
# define PRECOMPUTE_BUTTERFLY_H0     1
# define PRECOMPUTE_BUTTERFLY_OUTPUT 2

// precompute cloud shader
# define PRECOMPUTE_CLOUD_CLOUD_TEX 1

// ocean height field shader
# define OCEAN_HEIGHTFIELD_NOISE 1
# define OCEAN_HEIGHTFIELD_H0K   2

// ocean height final shader
# define OCEAN_HEIGHT_FINAL_H0K 3
# define OCEAN_HEIGHT_FINAL_H_X 4
# define OCEAN_HEIGHT_FINAL_H_Y 5
# define OCEAN_HEIGHT_FINAL_H_Z 6

// quad.frag
# define QUAD_CLOUD_TEX 1
# define QUAD_ENV_TEX   2

// texturedQuad.frag
# define SCREEN_QUAD_TEX 1

#endif