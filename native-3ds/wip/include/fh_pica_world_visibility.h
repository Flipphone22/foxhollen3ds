#ifndef FH_PICA_WORLD_VISIBILITY_H
#define FH_PICA_WORLD_VISIBILITY_H
#include "fh_world_bounds.h"
/* Resident draw bounds, rebuilt only when the owning world positions change.
 * Side planes only: retain near/far intersections and nonfinite input. No
 * index gathering, triangle reordering, temporal visibility or GPU readback. */
static inline void fh_pica_world_draw_bounds(FhWorldBounds *b,const void *vertices,
    const uint16_t *indices,unsigned count,unsigned packed,unsigned format){
    fh_world_bounds_build(b,vertices,indices,count,packed,format);
    if(!count){for(unsigned k=0;k<3;k++)b->lo[k]=b->hi[k]=NAN;return;}
    if(!packed)for(unsigned i=0;i<count;i++)for(unsigned k=0;k<3;k++){
        float v;memcpy(&v,(const unsigned char*)vertices+indices[i]*64+k*4,4);
        if(!isfinite(v)){for(unsigned j=0;j<3;j++)b->lo[j]=b->hi[j]=NAN;return;}
    }
}
#endif
