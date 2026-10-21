#ifndef FH_PICA_CLOUD_H
#define FH_PICA_CLOUD_H
#include "fh_pica_scene.h"
/* Scoped only around the original cloud objects' state/animation update and
 * native actor dispatch. No GX command path or generated cloud vertices. */
extern unsigned fh_pica_cloud_mode,fh_pica_cloud_glare_draws;
extern unsigned fh_pica_celestial_updates;
void fh_pica_game_sky_update(unsigned visible);
void fh_pica_cloud_material(const FhSceneAsset*,unsigned,unsigned model_flags,unsigned threshold,unsigned object_alpha,unsigned shadow);
static inline unsigned fh_pica_cloud_glare_parameters(float fraction,float size,float rect[4]){
    /* The shared HUD vertex entry consumes quarter-unit rectangle coordinates. */
    rect[0]=rect[1]=-size*4;rect[2]=rect[3]=size*4;
    unsigned alpha=fraction>=.5f?255u:(unsigned)(2.f*(255.f*fraction));
    return 0x8080ff00u|alpha;
}
static inline unsigned fh_pica_cloud_raster(unsigned mode,unsigned model,unsigned shader,unsigned threshold,unsigned alpha,unsigned shadow){
    unsigned blend=alpha<255 || (shader&0x40000000u) || shadow;
    unsigned test=!(model&0x400),write=test && (!blend || (model&0x2000));
    unsigned reject=blend?(test && (model&0x2000)):(shader&0x400);
    unsigned ref=blend?threshold:64;
    return 1u | (test?2u:0u) | (write?4u:0u) | (mode==1?8u:0u) |
        (blend?16u:0u) | (reject?32u:0u) | ((ref&255)<<8);
}
#endif
