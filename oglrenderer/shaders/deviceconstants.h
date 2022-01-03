#ifndef DEVICECONSTANTS_H
#define DEVICECONSTANTS_H

# define FRAMETIMES_COUNT          1000
# define SCREEN_BUFFER_COUNT       2
# define QUERY_DOUBLE_BUFFER_COUNT 2

// shader program Id
# define BUTTERFLY_SHADER         0
# define INVERSION_SHADER         1
# define PRECOMP_ENV_SHADER       2
# define PRECOMP_CLOUD_SHADER     3
# define PRECOMP_OCEAN_H0_SHADER  4
# define PRECOMP_OCEAN_H_SHADER   5
# define PRECOMP_BUTTERFLY_SHADER 6
# define PRE_RENDER_QUAD_SHADER   7
# define TEXTURED_QUAD_SHADER     8
# define WORLEY_NOISE_SHADER      9
# define PERLIN_NOISE_SHADER      10
# define CLOUD_NOISE_SHADER       11
# define WATER_SHADER             12
# define PRECOMP_SKY_SHADER       13
# define TEMPORAL_QUAD_SHADER     14
# define SHADER_COUNT             (TEMPORAL_QUAD_SHADER + 1)

// sky models
# define NISHITA_SKY 0
# define HOSEK_SKY   1

// uniform binding points
# define ORTHO_MATRIX    0
# define CAMERA_PARAMS   1
# define SKY_PARAMS      2
# define RENDERER_PARAMS 3
# define FBM_PARAMS      4
# define WORLEY_PARAMS   5
# define PERLIN_PARAMS   6
# define OCEAN_PARAMS    7
# define MVP_MATRIX      8
# define PREV_MVP_MATRIX 9

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
# define CLOUD_RESOLUTION       128
# define ENVIRONMENT_RESOLUTION 128
# define BLUENOISE_RESOLUTION   512

// ocean resolution
# define OCEAN_RESOLUTION_1 256
# define OCEAN_DIMENSIONS_1 1024
# define OCEAN_RESOLUTION_2 256
# define OCEAN_DIMENSIONS_2 512
# define OCEAN_RESOLUTION_3 64
# define OCEAN_DIMENSIONS_3 64

// compute shader
# define PRECOMPUTE_CLOUD_LOCAL_SIZE       4
# define PRECOMPUTE_OCEAN_WAVES_LOCAL_SIZE 16
# define PRECOMPUTE_FRESNEL_LOCAL_SIZE     4

# define PI (3.1415926f)

// BSDF
# define SAMPLE_COUNT 100

//////////////////////////////////////////////////////////////////////////

// butterfly operation
# define BUTTERFLY_INPUT_TEX     0
# define BUTTERFLY_PINGPONG_TEX0 1
# define BUTTERFLY_PINGPONG_TEX1 2

// cloud noise frag
# define CLOUD_NOISE_CLOUD_TEX 1

// inversion shader
# define INVERSION_PINGPONG_TEX0 0
# define INVERSION_PINGPONG_TEX1 1
# define INVERSION_OUTPUT_TEX    2

// ocean normal shader
# define OCEAN_NOMRAL_INPUT_TEX 0
# define OCEAN_NOMRAL_OUTPUT_TEX 1

// precompute butterfly shader
# define PRECOMPUTE_BUTTERFLY_OUTPUT 1

// precompute cloud shader
# define PRECOMPUTE_CLOUD_CLOUD_TEX 1

// precompute environment shader
# define PRECOMPUTE_ENVIRONMENT_CLOUD_TEX 1
# define PRECOMPUTE_ENVIRONMENT_NOISE_TEX 2
# define PRECOMPUTE_ENVIRONMENT_SKY_TEX   3

// ocean height field shader
# define OCEAN_HEIGHTFIELD_NOISE 1
# define OCEAN_HEIGHTFIELD_H0K   2

// ocean height final shader
# define OCEAN_HEIGHT_FINAL_H0K 3
# define OCEAN_HEIGHT_FINAL_H_X 4
# define OCEAN_HEIGHT_FINAL_H_Y 5
# define OCEAN_HEIGHT_FINAL_H_Z 6

// quad.frag
# define QUAD_CLOUD_TEX       1
# define QUAD_ENV_TEX         2
# define QUAD_NOISE_TEX       3
# define QUAD_PREV_SCREEN_TEX 4

// texturedQuad.frag
# define SCREEN_QUAD_TEX 1

// water shader
# define WATER_DISPLACEMENT1_TEX 1
# define WATER_DISPLACEMENT2_TEX 2
# define WATER_DISPLACEMENT3_TEX 3
# define WATER_ENV_TEX           4
# define WATER_FOAM_TEX          5

#endif