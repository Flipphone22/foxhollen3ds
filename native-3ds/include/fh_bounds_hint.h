#ifndef FH_BOUNDS_HINT_H
#define FH_BOUNDS_HINT_H
#include "fh_graphics.h"
/* Original common-plane intersection, retained as the fallback and oracle. */
static inline unsigned fh_bounds_set_original(const float low[10][3],const float high[10][3],unsigned used,
        const FhDrawState *state,const float pica[4][4],unsigned cpu,unsigned outside){
    if(used && !(used&(used-1))){
        unsigned slot=__builtin_ctz(used);
        unsigned gx=fh_vertex_bounds_any_plane(low[slot],high[slot],slot*3,state,state->position_matrix,cpu?outside:(outside&15));
        if(!gx)return 0;
        if(cpu)return gx;
        unsigned gpu=fh_vertex_bounds_any_plane(low[slot],high[slot],slot*3,state,pica,outside>>4);
        return gpu?gx|(gpu<<4):0;
    }
    for(unsigned slot=0;slot<10;slot++)if(used&(1u<<slot)){
        unsigned mask=fh_vertex_bounds_planes(low[slot],high[slot],slot*3,state,state->position_matrix,cpu?outside:(outside&15));
        if(!mask)return 0;
        if(!cpu)mask|=fh_vertex_bounds_planes(low[slot],high[slot],slot*3,state,pica,outside>>4)<<4;
        outside&=mask;if(!outside || (!cpu && (!(outside&15) || !(outside&240))))return 0;
    }
    return outside;
}
static inline unsigned fh_bounds_projection_valid(const float p[4][4],unsigned candidates){
    for(unsigned row=0;row<4;row++)if((row==3 && candidates) || (row<3 && (candidates&(3u<<(row*2)))))
        for(unsigned c=0;c<4;c++)if(!__builtin_isfinite(p[row][c]) || __builtin_fabsf(p[row][c])>1e18f)return 0;
    return 1;
}
/* A hint changes only test order, never reuses an old visibility decision.
 * Every current box and model is retested with the unchanged conservative
 * arithmetic. Validate all candidate projection rows before narrowing them,
 * retaining the scalar fallback for NaN/Inf even in a non-hinted plane. */
static inline unsigned fh_bounds_set_hinted(const float low[10][3],const float high[10][3],unsigned used,
        const FhDrawState *state,const float pica[4][4],unsigned cpu,unsigned outside,unsigned *hint,unsigned *reused){
    *reused=0;
    unsigned gx=*hint&(cpu?outside:(outside&15)),gpu=cpu?0:(*hint&outside)>>4;
    gx&=0u-gx;gpu&=0u-gpu;
    if(used && gx && (cpu || gpu) && fh_bounds_projection_valid(state->position_matrix,cpu?outside:(outside&15)) &&
       (cpu || fh_bounds_projection_valid(pica,outside>>4))){
        unsigned all=1;
        for(unsigned slot=0;slot<10;slot++)if(used&(1u<<slot)){
            unsigned excluded;
            if(!cpu && fh_bounds_pair_enabled)
                excluded=fh_vertex_bounds_pair(low[slot],high[slot],slot*3,state,pica,gx,gpu);
            else excluded=fh_vertex_bounds_planes(low[slot],high[slot],slot*3,state,state->position_matrix,gx) &&
                          (cpu || fh_vertex_bounds_planes(low[slot],high[slot],slot*3,state,pica,gpu));
            if(!excluded){all=0;break;}
        }
        if(all){*reused=1;return 1;}
    }
    unsigned result=fh_bounds_set_original(low,high,used,state,pica,cpu,outside);
    gx=result&(cpu?63:15);gx&=0u-gx;gpu=cpu?0:result>>4;gpu&=0u-gpu;
    *hint=gx|(gpu<<4);return !!result;
}
#endif
