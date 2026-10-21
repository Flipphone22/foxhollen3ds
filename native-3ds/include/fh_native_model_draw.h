#ifndef FH_NATIVE_MODEL_DRAW_H
#define FH_NATIVE_MODEL_DRAW_H
#include "fh_native_model_asset.h"
#include "fh_graphics.h"
#include "fh_gpu_lighting.h"
#include "fh_native_scene.h"
typedef struct {
    FhDrawState state;
    FhGpuLighting lighting;
    FhModelSource source;
    FhModelGroup group;
    float palette[FH_MODEL_BONES][3][4],normal_scale,post[3][4];
    float color[4];unsigned color_from_vertex[4],coordinates[8],lit;
    /* Optional persistent, GPU-visible scene storage. The caller validates
     * source mutations and keeps both buffers alive through GPU completion.
     * It must publish/flush changes before submission. Neither pointer may
     * refer to the normal heap or to a temporary game allocation. */
    const FhModelVertex *persistent_vertex;
    const uint16_t *persistent_index;
    unsigned direct_uv;
    float uv_transform[8][4]; /* Per-coordinate scale XY and offset ZW. */
} FhNativeModelDraw;
/* Synchronous original-state admission. Success consumes the complete group;
 * failure consumes no geometry and the original ordered lists must execute. */
int fh_gx_native_model_draw(const FhModelGroup *group,const FhModelSource *source,
    const float *poses,unsigned pose_count,const float view[3][4],float normal_scale);
int fh_renderer_native_model_draw(const FhNativeModelDraw *draw);
int fh_gx_native_scene_draw(const FhSceneAsset *asset,const FhSceneGroup *group,const FhModelSource *source,
    const float *poses,unsigned pose_count,const float view[3][4],float normal_scale);
extern unsigned fh_native_model_draws,fh_native_model_lists,fh_native_model_vertices_drawn,fh_native_model_failures;
extern unsigned fh_native_model_admission[16];
#endif
