#ifndef FH_PICA_MODEL_H
#define FH_PICA_MODEL_H
#include "fh_native_scene.h"
#include "fh_pica_scene.h"
/* Load-time commands. Offsets deliberately survive instance storage selection;
 * no descriptor ever retains a pointer into another actor's mutable vertices. */
typedef struct {
    unsigned vertex_offset,index_offset,indices,vertices,format,cull;
    unsigned material,flags,first_ordinal,ordinals,two_uv,bones;
    unsigned palette_changed; /* Load-compiled slot changes from previous draw. */
    unsigned fur_palette_changed; /* Fur-only subsequence; skipped groups never publish. */
    uint16_t bone[FH_MODEL_BONES];
} FhPicaModelDraw;
typedef struct {unsigned count;FhPicaMaterialSet materials;FhPicaModelDraw draw[];} FhPicaModelPlan;
extern const unsigned fh_pica_game_model_draw_stride;
extern unsigned fh_pica_scene_actors;
extern unsigned fh_pica_cpu_normal_calls_avoided,fh_pica_cpu_normal_groups_avoided;
const FhPicaModelPlan *fh_native_scene_pica_model(const FhSceneAsset*);
unsigned fh_pica_scene_actor_draw(const FhSceneAsset*,const FhPicaModelPlan*,const float*,unsigned,
    const float[3][4],const float[3][4],const float[4][4],const unsigned[4],float,float,unsigned*);
void fh_pica_scene_actor(const FhSceneAsset*,const float*,unsigned,const float[3][4],const float[3][4],const float[4][4],const unsigned[4]);
/* Experimental planar silhouette. Existing actor ownership/fences and GPU
 * skinning; no extra texture, vertex copies, worker or render target. */
void fh_pica_scene_shadow(const FhSceneAsset*,const float*,unsigned,const float[3][4],const float[3][4],const float[4][4],const unsigned[4],unsigned);
/* The asset must already have passed fh_native_scene_open. Every source draw
 * must be represented; a compiled omission is an error, not an empty model.
 * NULL output measures. Insufficient capacity never publishes a partial plan. */
unsigned fh_pica_model_compile(const FhSceneAsset*,FhPicaModelDraw*,unsigned);
/* Native palette is in world space. The shader applies camera/projection once
 * per vertex; do not rebuild view*joint for each material draw. Live game joint
 * matrices have 16-float stride. Fixed/rigid world matrix uses one explicit slot.
 * No geometry validation, allocation, source copies or GX state reconstruction. */
int fh_pica_model_pose(float out[3][4],const float joint[12],const float tail[3]);
#endif
