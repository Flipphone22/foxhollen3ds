#ifndef FH_MATERIAL_H
#define FH_MATERIAL_H
#include "fh_graphics.h"
enum { FH_PRIMARY,FH_TEXTURE0,FH_TEXTURE1,FH_TEXTURE2,FH_PREVIOUS,FH_CONSTANT,FH_BUFFER };
enum { FH_REPLACE,FH_MODULATE,FH_ADD,FH_ADD_SIGNED,FH_INTERPOLATE,FH_SUBTRACT,FH_MULTIPLY_ADD };
typedef struct { unsigned source[3],operand[3],operation,scale; } FhCombinerChannel;
typedef struct { FhCombinerChannel channel[2];float constant[4]; } FhCombiner;
typedef struct {
    unsigned count,textures;
    unsigned buffer_updates[2];
    int map[3],coord[3],primary[4];
    unsigned texture_mode[3];
    int secondary_map[3];
    FhCombiner stages[6];
} FhPipeline;
typedef struct {
    float color[4][9],bias[4];
    unsigned textured[4],scale[2];
    int map,coord;
    const char *failure;
    unsigned sphere_lighting;
    FhPipeline pipeline;
    unsigned movie_layer;
    unsigned cpu_fragment;
    float movie_mix;
    /* Ordered nonzero affine terms, one 1-based raster index per nibble. */
    uint32_t color_terms[4];
    unsigned sparse_colors;
} FhMaterial;
int fh_pipeline_compile(FhMaterial *plan,const FhDrawState *state);
int fh_layered_material_compile(FhMaterial *plan,const FhDrawState *state);
int fh_fur_material(const FhDrawState *state);
void fh_fur_pipeline(FhMaterial *plan,const FhDrawState *state);
void fh_fur_mesh_pipeline(FhMaterial *plan,const FhDrawState *state);
int fh_fur_surface_pipeline(FhMaterial *plan,const FhDrawState *state);
extern unsigned fh_chained_disabled,fh_chained_reference_bounds;
int fh_chained_material(const FhDrawState *state);
void fh_chained_pipeline(FhMaterial *plan,const FhDrawState *state);
unsigned fh_chained_triangle(FhVertex output[384],const FhVertex input[3],const FhDrawState *state);
/* Same validation; reports storage/depth exhaustion as 385, invalid inputs
 * as zero. Neither rejection permits submitting the partial output array. */
unsigned fh_chained_triangle_capacity(FhVertex output[384],const FhVertex input[3],const FhDrawState *state);
/* Append after an existing triangle prefix. On overflow (>384), only the
 * new suffix may be partial; the caller must discard that suffix. */
unsigned fh_chained_triangle_append(FhVertex output[384],unsigned prefix,const FhVertex input[3],const FhDrawState *state);
int fh_blur_material(const FhDrawState *state);
void fh_blur_pipeline(FhMaterial *plan,const FhDrawState *state,unsigned tap);
int fh_blur_quad(const FhVertex *vertices,unsigned count,const FhDrawState *state);
int fh_reflective_material(const FhDrawState *state);
int fh_reflective_triangle(const FhVertex *vertices,const FhDrawState *state);
void fh_reflective_pipeline(FhMaterial *plan,const FhDrawState *state,unsigned pass);
int fh_reflective_lod_pipeline(FhMaterial *plan,const FhDrawState *state);
extern unsigned fh_reflective_lod_enabled,fh_reflective_lod_plans;
extern unsigned fh_reflective_disabled;
extern unsigned fh_reflective_triangles,fh_reflective_fallbacks;
int fh_material_compile(FhMaterial *plan,const FhDrawState *state);
int fh_sphere_prepare(FhMaterial *plan,FhDrawState *draw,FhVertex *output,const FhDrawState *state,const FhVertex *input,unsigned count);
void fh_material_color(float output[4],const FhMaterial *plan,const float raster[2][4]);
typedef void (*FhTriangleSink)(const FhVertex triangle[3],void *context);
unsigned fh_material_subdivide(const FhVertex triangle[3],FhTriangleSink sink,void *context);
#endif
