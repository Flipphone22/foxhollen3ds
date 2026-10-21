#ifndef FH_PICA_ACTOR_LIVE_H
#define FH_PICA_ACTOR_LIVE_H
#include "fh_native_scene.h"
/* A synchronous borrow of one prepared material. Never retain this handle
 * across an actor update, resource retirement, or a queued worker boundary. */
typedef struct FhPicaActorLiveTarget FhPicaActorLiveTarget;
typedef struct {
    unsigned object_alpha,threshold,post_state,environment_slot,environment_strength;
    unsigned cloud_mode,model_flags,shadow;
} FhPicaActorLiveCommon;
typedef struct {
    float scroll[2][2];
    unsigned frames[2],alpha,konst,flags,selector,environment_enabled;
} FhPicaActorLiveValues;
FhPicaActorLiveTarget *fh_pica_actor_live_target(const FhSceneAsset*,unsigned,unsigned *dual);
void fh_pica_actor_live_publish(FhPicaActorLiveTarget*,const FhPicaActorLiveCommon*,const FhPicaActorLiveValues*);
#endif
