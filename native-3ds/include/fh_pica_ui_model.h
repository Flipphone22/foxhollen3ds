#ifndef FH_PICA_UI_MODEL_H
#define FH_PICA_UI_MODEL_H
#include "fh_pica_scene.h"
enum { FH_PICA_UI_RING=1,FH_PICA_UI_ICON=2,FH_PICA_UI_ACTION=3,FH_PICA_UI_LOCK=4 };
typedef struct {unsigned kind,rgba,page,blend,depth_test,depth_write;} FhPicaUiMaterial;
unsigned fh_pica_game_ui_kind(const void*);
unsigned fh_pica_game_reticle_kind(const void*);
void fh_pica_game_ui_materials(const FhSceneAsset*,const void*,const void*,unsigned);
void fh_pica_game_reticle_materials(const FhSceneAsset*,const void*,const void*,unsigned);
void fh_pica_material_ui(const FhSceneAsset*,unsigned,const FhPicaUiMaterial*);
void fh_pica_scene_ui_actor(const FhSceneAsset*,const float*,unsigned,const float[3][4],const float[3][4],const float[4][4],const unsigned[4]);
#endif
