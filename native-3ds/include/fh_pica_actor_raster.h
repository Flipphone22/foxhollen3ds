#ifndef FH_PICA_ACTOR_RASTER_H
#define FH_PICA_ACTOR_RASTER_H
#include <stdint.h>
enum { FH_AR_DEPTH=1, FH_AR_WRITE=2, FH_AR_BLEND=4, FH_AR_ALPHA=8, FH_AR_LIVE_ALPHA=16 };
/* Original shaderSetup material policy, compiled once per loaded material.
 * Lower half: opaque object. Upper half: fading object. Flags are source
 * model/material properties; the active draw never decodes them again. */
static inline uint32_t fh_pica_actor_raster_plan(unsigned model,unsigned shader){
    unsigned opaque=(model&0x400)?0:FH_AR_DEPTH|FH_AR_WRITE;
    unsigned faded=FH_AR_BLEND;
    if(!(model&0x400)){
        faded|=FH_AR_DEPTH;
        if(model&0x2000)faded|=FH_AR_WRITE|FH_AR_ALPHA|FH_AR_LIVE_ALPHA;
    }
    if(shader&0x400)opaque|=FH_AR_ALPHA|(0x40u<<8);
    if(shader&0x40000000u)opaque=faded;
    return opaque|(faded<<16);
}
static inline unsigned fh_pica_actor_raster_live(uint32_t plan,unsigned object_alpha,unsigned threshold){
    unsigned state=(plan>>(object_alpha<255?16:0))&0xffff;
    if(state&FH_AR_LIVE_ALPHA)state=(state&255)|((threshold&255)<<8);
    return state;
}
#endif
