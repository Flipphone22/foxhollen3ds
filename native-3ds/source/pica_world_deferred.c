#include "fh_pica_world_deferred.h"
#include <string.h>
#include <math.h>

int fh_pica_world_sort_bounds(FhPicaWorldSortBounds *out,const void *source_lists,
    unsigned bytes,unsigned list){
    if(!out || !source_lists || list>=bytes/28)return 0;
    const unsigned char *p=(const unsigned char*)source_lists+list*28;
    FhPicaWorldSortBounds b;
    for(unsigned k=0;k<3;k++){
        unsigned lo=(unsigned)p[6+k*2]*256+p[7+k*2];
        unsigned hi=(unsigned)p[12+k*2]*256+p[13+k*2];
        b.lo[k]=(int16_t)(lo<32768?(int)lo:(int)lo-65536);
        b.hi[k]=(int16_t)(hi<32768?(int)hi:(int)hi-65536);
        if(b.lo[k]>b.hi[k])return 0;
    }
    b.selector=p[24];*out=b;return 1;
}

int fh_pica_world_sort_key(const FhPicaWorldSortBounds *b,float scale,
    const float translation[3],const float camera[4],uint32_t *key){
    if(!b || !translation || !camera || !key || !isfinite(scale))return 0;
    float center[3];
    for(unsigned k=0;k<3;k++){
        if(b->lo[k]>b->hi[k] || !isfinite(translation[k]) || !isfinite(camera[k]))return 0;
        /* Retain source order: translating the midpoint instead may move a
         * depth key by one at large coordinates due to float32 rounding. */
        center[k]=.5f*((b->lo[k]*scale+translation[k])+(b->hi[k]*scale+translation[k]));
    }
    float depth=-(((camera[0]*center[0]+camera[1]*center[1])+camera[2]*center[2])+camera[3]);
    if(!isfinite(depth))return 0;
    /* Clamp before conversion to avoid undefined float-to-int overflow. This
     * agrees with source integer truncation throughout its defined domain. */
    uint32_t t=depth<=0?0:depth>=134217727.f?0x7ffffffu:(uint32_t)depth;
    *key=t|((b->selector&255u)<<27);return 1;
}

unsigned fh_pica_world_deferred_family(unsigned flags){
    /* tex_dolphin.c mapBlockRender_callList(passSelect=0): hidden precedes
     * all effects; water wins over transparent/projected falloff flags. */
    if(flags&2u)return 0;
    if(flags&0x80000000u)return 5;
    if(flags&(0x40000000u|0x2000u))return 4;
    return 0;
}

unsigned fh_pica_world_deferred_compile(const FhSceneAsset *a,unsigned stream,
    FhPicaWorldDeferred *out,unsigned capacity){
    if(stream<1 || stream>2 || !fh_pica_world_compile_stream(a,stream,NULL,0))return 0;
    unsigned sizes[3],first,end,count=0;
    memcpy(sizes,a->meta+80,sizeof sizes);
    first=sizes[0]+(stream==2?sizes[1]:0);end=first+sizes[stream];
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;
        if(!fh_native_scene_group(a,i,&g))return 0;
        if(g.first<first)continue;
        if(g.first>=end)break;
        for(unsigned j=0;j<g.draws;j++){
            FhSceneSection s;
            if(!fh_native_scene_section(a,g.first_section+j,&s) ||
                s.first>g.indices || s.count>g.indices-s.first)return 0;
            unsigned family=fh_pica_world_deferred_family(s.shader_flags);
            if(!family)continue;
            if(out){
                if(count>=capacity)return 0;
                out[count]=(FhPicaWorldDeferred){i,g.first_section+j,s.list,
                    s.shader,s.first,s.count,family};
            }
            count++;
        }
    }
    return count;
}
