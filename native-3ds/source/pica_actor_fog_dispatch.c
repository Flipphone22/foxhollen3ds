#include "fh_pica_actor_fog_dispatch.h"
#include "fh_pica_actor_fog_game.h"
#include "fh_native_actor_fog_owner.h"
#include "fh_graphics_options.h"
int fh_pica_actor_fog_game_draw(const FhSceneAsset*asset,unsigned generation,
    const struct ObjModel*model,struct GameObject*obj,unsigned near_shadow,
    const float*poses,unsigned joints,const float world[3][4],
    const float view[3][4],const float projection[4][4],const unsigned clip[4]){
    if(!generation||!asset||!projection||!clip||near_shadow>1||
       !(fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG)||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS)))return 0;
    FhPicaActorFogSnapshot snapshot;
    if(!fh_pica_actor_fog_game_live(&snapshot,model,obj,poses,world,view))return 0;
    FhPicaActorFogSelection selection;
    if(!fh_pica_actor_fog_game_selection(&selection,model,obj,near_shadow))return 0;
    const FhActorFogAsset*fog=fh_native_scene_actor_fog_select(asset,generation,&selection);
    if(!fog)return 0;
    fh_pica_scene_actor_with_fog(asset,fog,&snapshot,poses,joints,world,view,projection,clip);
    return 1;
}
