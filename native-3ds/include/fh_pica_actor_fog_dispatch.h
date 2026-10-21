#ifndef FH_PICA_ACTOR_FOG_DISPATCH_H
#define FH_PICA_ACTOR_FOG_DISPATCH_H
#include "fh_pica_actor_fog_live.h"
/* Synchronous game-thread handoff; generation is the source-material generation
 * owned by the loaded scene bank. A zero/absent generation returns before source
 * fog/light queries. near_shadow must be the CURRENT object's culling result.
 * Returns 1 only after the graphics bridge consumes this actor (including its
 * ordinary native fallback); 0 means the caller must issue its ordinary draw.
 * No allocation, source/geometry compilation, file IO, or retained pointers. */
int fh_pica_actor_fog_game_draw(const FhSceneAsset*,unsigned generation,
    const struct ObjModel*,struct GameObject*,unsigned near_shadow,
    const float *poses,unsigned joints,const float world[3][4],
    const float view[3][4],const float projection[4][4],const unsigned clip[4]);
#endif
