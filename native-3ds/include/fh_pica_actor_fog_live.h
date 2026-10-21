#ifndef FH_PICA_ACTOR_FOG_LIVE_H
#define FH_PICA_ACTOR_FOG_LIVE_H
#include "fh_pica_actor_fog_draw.h"
struct ObjModel;
struct GameObject;
/* Stack-owned, synchronous publication. Do not copy this struct after capture:
 * live.adjustment/planes refer to its own storage. No skeleton or vertices are
 * copied. The source image is an identity only; the GPU owner selects its
 * immutable height atlas after it has acquired command ownership. */
typedef struct {
    FhPicaActorFogLive live;
    float adjustment[3][4],planes[2][4];
} FhPicaActorFogSnapshot;
/* Ordinary main actor pass only, after animation. Source-mutating actors need
 * a separate input implementation; rejection leaves ordinary drawing intact.
 * Disabled/invalid admission does not query source fog or prepare GPU work.
 * Returns zero without modifying output. */
int fh_pica_actor_fog_game_live(FhPicaActorFogSnapshot*,const struct ObjModel*,
    const struct GameObject*,const float *poses,const float world[3][4],
    const float view[3][4]);
/* Main graphics owner: uses its usual begin/projection/scissor/accounting and
 * falls back to the ordinary native actor if fog admission declines. Never
 * retains the stack snapshot or game matrices after returning. */
void fh_pica_scene_actor_with_fog(const FhSceneAsset*,const FhActorFogAsset*,
    const FhPicaActorFogSnapshot*,const float *poses,unsigned joints,
    const float world[3][4],const float view[3][4],
    const float camera_projection[4][4],const unsigned scissor[4]);
#endif
