#ifndef FH_PICA_SHADOW_PROJECT_H
#define FH_PICA_SHADOW_PROJECT_H
#include "fh_pica_shadow_receivers.h"
void fh_pica_shadow_project_init(void);
void fh_pica_shadow_project_shutdown(void);
void fh_pica_shadow_project_invalidate(void);
void fh_pica_shadow_project_surface(unsigned alternate);
void fh_pica_shadow_project_transform(const FhSceneAsset*,const float[3][4]);
void fh_pica_shadow_project_query(const int32_t bounds[6]);
const FhPicaShadowReceiver *fh_pica_shadow_receivers_for_asset(const FhSceneAsset*);
const FhPicaWorldPlan *fh_pica_shadow_receiver_batch_plan(const FhSceneAsset*);
void fh_pica_shadow_project_world(const FhSceneAsset*,const FhPicaWorldPlan*,const float[3][4],const float[4][4],const unsigned[4]);
void fh_pica_shadow_project_forget(const FhSceneAsset*);
void fh_pica_shadow_project(const FhSceneAsset*,const float*,unsigned,const float[3][4],const float[3],float,float,unsigned);
void fh_pica_shadow_project_mask(unsigned,const float[3],float,unsigned);
/* Native caster renderer, called with the image attached; no game callbacks. */
unsigned fh_pica_scene_shadow_capture(const FhSceneAsset*,const float*,unsigned,const float[3][4],const float[3][4],const float[4][4],unsigned*);
#endif
