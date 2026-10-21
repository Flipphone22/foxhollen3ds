#ifndef FH_PICA_ACTOR_FOG_PLAN_H
#define FH_PICA_ACTOR_FOG_PLAN_H
#include "fh_pica_actor_fog.h"
/* Load-resolved source pose recipe. Extra-joint indices are ORIGINAL indices,
 * not the visible packet's numbering (which may insert a world slot). A blend
 * remains a blend at weights zero/four: its selected bind tail still applies.
 * Owners may deduplicate these immutable recipes across groups. No live source
 * pointers or GPU storage are retained by the recipe. */
typedef struct {
    uint8_t kind,a,b,weight,blend,reserved[3];
    float tail_a[3],tail_b[3];
} FhPicaActorFogPosePlan;
_Static_assert(sizeof(FhPicaActorFogPosePlan)==32,"Source fog pose recipe size");
/* One selected/changed entry only. Original poses have a 16-float stride;
 * padding is not read. cached_normal is the source cache mode, supplied by the
 * adapter. The owner publishes successful rows and invalidates on instance,
 * camera, adjustment and shader changes. No implicit cross-frame pose cache. */
int fh_pica_actor_fog_plan_pose(float out[3][4],const FhPicaActorFogPosePlan*,
    const float *poses,unsigned pose_count,const float view[3][4],
    const float adjustment[3][4],unsigned cached_normal);
/* Ordinary pre-stream world entries use the object's actual world matrix.
 * Source normals clear translation BEFORE adjustment, independent of whether
 * skeletal source matrices use a cache. Fur-transformed preloads are separate
 * and are not admitted by this ordinary actor helper. */
int fh_pica_actor_fog_plan_world_pose(float out[3][4],const FhPicaActorFogPosePlan*,
    const float world[3][4],const float view[3][4],const float adjustment[3][4]);
#endif
