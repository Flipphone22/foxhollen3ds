#include "fh_pica_scene.h"
#include <string.h>
unsigned fh_pica_world_compile(const FhSceneAsset *a,FhPicaWorldDraw *out,unsigned capacity){
    return fh_pica_world_compile_stream(a,0,out,capacity);
}
unsigned fh_pica_world_compile_stream(const FhSceneAsset *a,unsigned stream,FhPicaWorldDraw *out,unsigned capacity){
    if(!a || !a->meta || !a->block || !a->direct || a->metadata_bytes<96 || stream>2)return 0;
    unsigned sizes[3],first=0,end;
    memcpy(sizes,a->meta+80,sizeof sizes);
    /* Subtraction checks avoid overflow even on malformed unpublished input. */
    unsigned remaining=a->draws;
    for(unsigned s=0;s<3;s++){
        if(sizes[s]>remaining)return 0;
        remaining-=sizes[s];
        if(s<stream)first+=sizes[s];
    }
    if(remaining || !sizes[stream])return 0;
    end=first+sizes[stream];
    unsigned ordinal=0,count=0;
    for(unsigned i=0;ordinal<end && i<a->groups;i++){
        FhSceneGroup g;FhSceneSection material;
        if(!fh_native_scene_group(a,i,&g) || !fh_native_scene_section(a,g.first_section,&material) ||
            g.first!=ordinal || !g.draws || g.draws>end-ordinal || !g.indices)return 0;
        if(ordinal<first){
            if(g.draws>first-ordinal)return 0;
            ordinal+=g.draws;continue;
        }
        /* Preserve every original index and material boundary. GPU storage
         * was validated/relocated at load; the plan borrows that lifetime. */
        if(out){
            if(count>=capacity)return 0;
            out[count]=(FhPicaWorldDraw){g.vertex,g.index,g.indices,g.vertices,g.format,g.cull,material.shader,material.shader_flags};
        }
        count++;ordinal+=g.draws;
    }
    return ordinal==end?count:0;
}
