#ifndef FH_PICA_ACTOR_FOG_DRAW_H
#define FH_PICA_ACTOR_FOG_DRAW_H
#include "fh_native_actor_fog.h"
#include "fh_pica_model.h"
/* Synchronous actor submission borrows these original source matrices until
 * the draw call returns; commands contain copied uniforms, not these pointers.
 * A future queued actor path MUST snapshot them or retain them through worker
 * completion. Native visible-pose numbering
 * is separate (it may insert a world slot). No guessing from skinned vertices.
 * Source positions must still match the immutable prepared packet; mutable
 * source/morph publication must be admitted separately before using this API. */
typedef struct {
    const float *poses;
    unsigned pose_count,cached_normal,color;
    const float (*world)[4],(*view)[4],(*adjustment)[4],(*planes)[4];
    const uint8_t *image;
    unsigned intensity;
} FhPicaActorFogLive;
/* Admission-time shader setup. Never called by draw. */
void fh_pica_scene_actor_fog_init(void);
/* All material/geometry admission precedes the first draw. Zero means the
 * caller must render this actor through its ordinary native path. A nonzero
 * result renders the full packet; no partial actor or omitted material. */
unsigned fh_pica_scene_actor_fog_draw(const FhSceneAsset*,const FhPicaModelPlan*,
    const FhActorFogAsset*,const FhPicaActorFogLive*,const float *visible_poses,
    unsigned visible_joints,const float world[3][4],const float view[3][4],
    const float projection[4][4],const unsigned clip[4],float near_z,float far_z,
    unsigned *indices);
#endif
