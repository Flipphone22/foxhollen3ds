#ifndef FH_PICA_GLOW_FIELD_H
#define FH_PICA_GLOW_FIELD_H
#include <stddef.h>
#include <stdint.h>
#include "fh_pica_glow_adaptive.h"
/* Load-time source view, never a GPU allocation. Format 0: decoded RGBA8;
 * format 1: original big-endian 4x4-tiled RGB5A3, stride must be zero. */
typedef struct {
    const uint8_t *rgba;
    size_t bytes, stride;
    unsigned width, height, wrap_s, wrap_t;
    unsigned format;
} FhGlowField;
size_t fh_glow_field_bytes(const FhGlowField *field);
/* Callback for fh_glow_mesh_compile. Coordinates restricted to its unit square.
 * Returns source reflection-texel displacement; leaves output intact on error. */
int fh_glow_field_sample(void *context, float u, float v, float output[2]);
typedef struct {
    FhGlowField field;
    int16_t uv[4]; /* source opposite corners in 1/128 texture units */
} FhGlowFieldRegion;
int fh_glow_field_sample_uv(void *context,float u,float v,float output[2]);
int fh_glow_field_region_sample(void *context,float u,float v,float output[2]);
int fh_glow_field_region_compile(FhGlowMesh *output,FhGlowMeshVertex *scratch,
                                 size_t scratch_count,const FhGlowFieldRegion *region);
/* Explicit load-time quality budget; 2..FH_GLOW_MESH_TRIANGLES. Invalid budgets
 * are rejected before touching output or scratch. Ordinary callers stay128. */
int fh_glow_field_region_compile_budget(FhGlowMesh *output,FhGlowMeshVertex *scratch,
    size_t scratch_count,const FhGlowFieldRegion *region,unsigned triangle_budget);
/* End-to-end load compiler. Scratch has FH_GLOW_MESH_TRAINING entries and
 * can be reused immediately after return. Source, scratch and output must not
 * overlap. Does not allocate, upload, or publish the resulting mesh. */
int fh_glow_field_compile(FhGlowMesh *output, FhGlowMeshVertex *scratch,
                          size_t scratch_count, const FhGlowField *field);
/* Canonical full-texture field shared across four flips, with both diagonals.
 * Budget >=4. Consumer coordinate mapping is required; not a drop-in region. */
int fh_glow_field_shared_compile(FhGlowMesh *output,FhGlowMeshVertex *scratch,
    size_t scratch_count,const FhGlowField *field,unsigned triangle_budget);
#endif
