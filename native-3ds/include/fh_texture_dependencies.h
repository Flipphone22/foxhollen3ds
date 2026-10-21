#ifndef FH_TEXTURE_DEPENDENCIES_H
#define FH_TEXTURE_DEPENDENCIES_H
#include "fh_graphics.h"
/* Resource content includes the palette as well as texels, including for a
 * secondary image used to construct a packed native texture. Subtraction keeps
 * overlap checks valid when an address + length would wrap. */
static inline int fh_texture_source_overlap(const FhTexture *texture,const void *address,size_t bytes){
    if(!address)return 1;
    if(!bytes)return 0;
    uintptr_t changed=(uintptr_t)address;
    const void *sources[2]={texture->pixels,texture->palette};
    size_t sizes[2]={fh_texture_source_bytes(texture->format,texture->width,texture->height),0};
    if(texture->palette){
        if((uint64_t)texture->palette_count*2>SIZE_MAX)return 1;
        sizes[1]=(size_t)texture->palette_count*2;
    }
    for(unsigned i=0;i<2;i++)if(sources[i] && sizes[i]){
        uintptr_t source=(uintptr_t)sources[i];
        if(changed>=source?changed-source<sizes[i]:source-changed<bytes)return 1;
    }
    return 0;
}
/* Conservative source set already used by the CPU fragment materialization
 * path. Keep every declared TEV and indirect sampler, even when its value
 * appears dead. Layered/native programs only specialize these inputs; their
 * generated lookup images are created on the renderer owner. */
static inline unsigned fh_texture_dependencies(const FhDrawState *state){
    if(state->stages>16 || state->indirect_count>4)return 255;
    unsigned used=0;
    for(unsigned i=0;i<state->stages;i++)if(state->tev[i].map<8)used|=1u<<state->tev[i].map;
    for(unsigned i=0;i<state->indirect_count;i++)if(state->indirect_order[i][1]<8)used|=1u<<state->indirect_order[i][1];
    return used;
}
#endif
