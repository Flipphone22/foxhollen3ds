#ifndef FH_FRAGMENT_H
#define FH_FRAGMENT_H
#include "fh_graphics.h"
int fh_fragment_supported(const FhDrawState *state);
uint32_t fh_texture_pixel(const FhTexture *texture,unsigned x,unsigned y);
void fh_texture_sample(float out[4],const FhTexture *texture,float s,float t);
int fh_texture_pack_fur(uint32_t *out,unsigned width,unsigned height,const FhTexture *base,const FhTexture *mask);
typedef struct {
    float scale[8][2],indirect_divisor[4][2],exponent[3],konst[16][4];
    unsigned uv_mask,sample_mask,indirect_mask;
    uint8_t arguments[16][4][4];
} FhFragmentProgram;
/* Prepared constants borrow the immutable state for one raster job. */
void fh_fragment_prepare(FhFragmentProgram *program,const FhDrawState *state);
int fh_fragment_evaluate_prepared(float out[4],const FhDrawState *state,const FhVertex *fragment,const FhFragmentProgram *program);
int fh_fragment_evaluate(float out[4],const FhDrawState *state,const FhVertex *fragment);
void fh_fur_noise(float out[4],const FhDrawState *state,const FhVertex *fragment,const float matrix[2][2][3]);
void fh_fur_coordinate(float out[3],const FhDrawState *state,const FhVertex *fragment,const float matrix[2][2][3]);
void fh_fur_samples(float samples[2][3],const FhDrawState *state,const FhVertex *fragment);
void fh_fur_offset(float offset[2],const float matrix[2][2][3],const float samples[2][3]);
void fh_fur_coordinate_sampled(float out[3],const FhDrawState *state,const FhVertex *fragment,const float matrix[2][2][3],const float samples[2][3]);
unsigned fh_fur_mesh(FhVertex *output,unsigned capacity,const FhVertex triangle[3],const FhDrawState *state,const float matrix[2][2][3]);
unsigned fh_fur_vertices(FhVertex *output,const FhVertex *triangles,unsigned count,const FhDrawState *state,const float matrix[2][2][3]);
/* Retained sampling requires invalidation at frame boundaries and source writes.
 * The ordinary entry point above remains self-contained for external callers. */
void fh_fur_cache_invalidate(void);
void fh_fur_cache_invalidate_range(const void *address,size_t bytes);
unsigned fh_fur_coordinates_cached(float output[][3],const FhVertex *triangles,unsigned count,const FhDrawState *state,const float matrix[2][2][3]);
/* IDs are borrowed only for this call and identify byte-identical vertices. */
unsigned fh_fur_coordinates_identified(float output[][3],const FhVertex *triangles,unsigned count,const FhDrawState *state,const float matrix[2][2][3],const uint32_t *identities);
unsigned fh_fur_vertices_cached(FhVertex *output,const FhVertex *triangles,unsigned count,const FhDrawState *state,const float matrix[2][2][3]);
#endif
