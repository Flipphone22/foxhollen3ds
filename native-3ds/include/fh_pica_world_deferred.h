#ifndef FH_PICA_WORLD_DEFERRED_H
#define FH_PICA_WORLD_DEFERRED_H
#include "fh_pica_scene.h"
static inline int fh_pica_deferred_surface_flags(unsigned flags){
    return (flags&0x40000000u) && !(flags&~0x400c011du);
}
/* Source queue family numbers, not material blend modes. Retain each authored
 * list: its bounds/selector and depth-sort key are resolved by the live owner. */
typedef struct {
    unsigned group,section,list,material,first,count,family;
} FhPicaWorldDeferred;
unsigned fh_pica_world_deferred_family(unsigned flags);
/* Validated immutable packets only. Load-time compilation; no GX interpreter,
 * resource allocation or GPU publication. Discard output on zero/failure. */
unsigned fh_pica_world_deferred_compile(const FhSceneAsset*,unsigned stream,
    FhPicaWorldDeferred*,unsigned capacity);
/* Original signed bounds/selector, captured once while loading. No GPU data. */
typedef struct {int16_t lo[3],hi[3];unsigned selector;} FhPicaWorldSortBounds;
typedef struct {FhPicaWorldDeferred draw;FhPicaWorldSortBounds bounds;
    FhPicaWorldDraw geometry;} FhPicaWorldDeferredEntry;
typedef struct {unsigned bytes,count[2];FhPicaWorldDeferredEntry entries[];} FhPicaWorldDeferredPlan;
const FhPicaWorldDeferredPlan *fh_native_scene_deferred_plan(const FhSceneAsset*);
void fh_pica_scene_world_slice(const FhSceneAsset*,const FhPicaWorldDraw*,
    const float[3][4],const float[4][4],const unsigned[4]);
int fh_pica_world_sort_bounds(FhPicaWorldSortBounds*,const void *source_lists,
    unsigned bytes,unsigned list);
/* Live translation and camera depth row only; source sort quantization retained.
 * Invalid inputs fail without modifying key. Visibility is a separate gate. */
int fh_pica_world_sort_key(const FhPicaWorldSortBounds*,float coordinate_scale,
    const float translation[3],const float camera_depth[4],uint32_t *key);
#endif
