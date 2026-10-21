#ifndef FH_PICA_HOLOGRAM_H
#define FH_PICA_HOLOGRAM_H
#include "fh_pica_model.h"
typedef struct {
    float projected[3][4],normal_uv[2][4];
    unsigned mask_page,alpha,mask_alpha,cull_front;
} FhPicaHologram;
int fh_pica_game_hologram_matches(const void *callback);
void fh_pica_game_hologram_state(const void *object,FhPicaHologram*);
void fh_pica_hologram_init(void *entry);
void fh_pica_hologram_shutdown(void);
void fh_pica_hologram_prepare(const FhSceneAsset*);
void fh_pica_scene_hologram(const FhSceneAsset*,const float*,unsigned,const float[3][4],
    const float[3][4],const float[4][4],const unsigned[4],const FhPicaHologram*);
unsigned fh_pica_hologram_draw(const FhSceneAsset*,const FhPicaModelPlan*,const float*,unsigned,
    const float[3][4],const float[3][4],const float[4][4],const unsigned[4],const FhPicaHologram*,unsigned*);
#endif
