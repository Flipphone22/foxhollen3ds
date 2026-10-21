#ifndef FH_INDIRECT_LIGHTING_H
#define FH_INDIRECT_LIGHTING_H
#include "fh_material.h"
extern unsigned fh_indirect_lighting_enabled,fh_indirect_lighting_triangles;
extern unsigned fh_indirect_lighting_detail;
typedef struct {
    float scale[8][2],divisor[4][2],exponent[3];
    unsigned uv_mask,indirect_mask;
    /* -1 retains the original coordinate; otherwise an unused output UV. */
    int destination[16];
} FhIndirectLighting;
int fh_indirect_lighting_prepare(FhIndirectLighting *prepared,const FhDrawState *state);
int fh_indirect_lighting_vertex_prepared(FhVertex *out,const FhVertex *in,const FhDrawState *state,const FhIndirectLighting *prepared);
int fh_indirect_lighting_mesh_prepared(const FhVertex triangle[3],const FhDrawState *state,const FhIndirectLighting *prepared,FhTriangleSink sink,void *context);
int fh_indirect_lighting_plan(FhMaterial *plan,FhDrawState *draw,const FhDrawState *state);
int fh_indirect_lighting_vertex(FhVertex *out,const FhVertex *in,const FhDrawState *state);
/* Clipped, resolved triangle only. Bounded to 256 output triangles. */
int fh_indirect_lighting_mesh(const FhVertex triangle[3],const FhDrawState *state,FhTriangleSink sink,void *context);
#endif
