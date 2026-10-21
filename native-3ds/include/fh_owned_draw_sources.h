#ifndef FH_OWNED_DRAW_SOURCES_H
#define FH_OWNED_DRAW_SOURCES_H
#include "gx_internal.h"
#include "fh_owned_sources.h"
#include "fh_texture_dependencies.h"

/* The caller owns the GX state separately (the producer patch stream does
 * that). This transaction owns all CPU source bytes referenced by one draw.
 * GPU-backed/deferred EFB images need a reader barrier or materialization;
 * ready() must decline them before copying stale CPU bytes. No GPU resource
 * pointer is leased by this module. */
typedef int (*FhOwnedSourceReady)(const void *,size_t,void *);
static inline int fh_owned_draw_add(FhOwnedSources *pool,FhOwnedSourceLease *lease,
    const void *data,size_t bytes,FhOwnedSourceReady ready,void *context){
    return data && bytes && ready(data,bytes,context) && fh_owned_source_acquire(pool,lease,data,bytes);
}
static inline int fh_owned_draw_geometry(FhOwnedSources *pool,FhOwnedSourceLease *lease,
    FhOwnedSourceLease *model,const void *data,size_t bytes,FhOwnedSourceReady ready,void *context){
    if(!data || !bytes || !ready(data,bytes,context))return 0;
    if(model){
        if(fh_owned_source_share(pool,lease,model,data,bytes))return 1;
        if(fh_owned_source_acquire(pool,model,data,bytes) &&
           fh_owned_source_share(pool,lease,model,data,bytes))return 1;
    }
    return fh_owned_source_acquire(pool,lease,data,bytes);
}
static inline int fh_owned_draw_texture(FhOwnedSources *pool,FhOwnedSourceLease *lease,
    FhOwnedSourceLease *resources,unsigned validate,const void *data,size_t bytes,FhOwnedSourceReady ready,void *context){
    if(!data || !bytes || !ready(data,bytes,context))return 0;
    if(resources && validate){
        const void *copy=fh_owned_source_resolve(resources,data,bytes);
        if(copy && memcmp(copy,data,bytes))fh_platform_panic("Texture resource changed without write barrier");
    }
    return fh_owned_draw_geometry(pool,lease,resources,data,bytes,ready,context);
}
/* Resource reuse requires ordered CPU-write/EFB/invalidate notifications.
 * The producer forgets changed ranges before acquiring a new version; GPU-only
 * sources still decline. Without this contract pass resources=NULL. */
static inline int fh_owned_draw_capture_resources(FhOwnedSources *pool,FhOwnedSourceLease *lease,
    FhOwnedSourceLease *model,FhOwnedSourceReady immutable,FhOwnedSourceLease *resources,unsigned validate,const FhGx *state,
    const void *list,unsigned bytes,FhOwnedSourceReady ready,void *context){
    if(!pool || !lease || lease->count || !state || !ready)return 0;
    if(!fh_owned_draw_geometry(pool,lease,model && immutable && immutable(list,bytes,context)?model:NULL,
        list,bytes,ready,context))goto fail;
    for(unsigned i=GX_VA_POS;i<=GX_VA_TEX7;i++)if(state->descriptors[i]>=GX_INDEX8){
        const FhVtxArray *a=&state->arrays[i];
        if(!fh_owned_draw_geometry(pool,lease,model && immutable && immutable(a->data,a->bytes,context)?model:NULL,
            a->data,a->bytes,ready,context))goto fail;
    }
    /* GX retains bindings across draws. Own all declared TEV/indirect inputs,
     * not stale unused bindings (which can even be GPU-only destinations).
     * Descriptor metadata is already in the owned state patch stream. */
    unsigned used=fh_texture_dependencies(&state->draw);
    for(unsigned i=0;i<8;i++)if(used&(1u<<i)){
        const FhTexture *t=&state->draw.textures[i];if(!t->pixels)continue;
        if(!t->width || t->width>1024 || !t->height || t->height>1024 || t->palette_count>16384)goto fail;
        size_t length=fh_texture_source_bytes(t->format,t->width,t->height);
        if(!fh_owned_draw_texture(pool,lease,resources,validate,t->pixels,length,ready,context))goto fail;
        if(t->palette && t->palette_count &&
           !fh_owned_draw_texture(pool,lease,resources,validate,t->palette,t->palette_count*2u,ready,context))goto fail;
    }
    return 1;
fail:
    if(!fh_owned_source_release(pool,lease))fh_platform_panic("Owned draw source rollback failed");
    return 0;
}
static inline int fh_owned_draw_capture_model(FhOwnedSources *pool,FhOwnedSourceLease *lease,
    FhOwnedSourceLease *model,FhOwnedSourceReady immutable,const FhGx *state,
    const void *list,unsigned bytes,FhOwnedSourceReady ready,void *context){
    return fh_owned_draw_capture_resources(pool,lease,model,immutable,NULL,0,state,list,bytes,ready,context);
}
static inline int fh_owned_draw_capture(FhOwnedSources *pool,FhOwnedSourceLease *lease,
    const FhGx *state,const void *list,unsigned bytes,FhOwnedSourceReady ready,void *context){
    return fh_owned_draw_capture_model(pool,lease,NULL,NULL,state,list,bytes,ready,context);
}
/* Remap only a private consumer state, never the producer or a shared GX
 * state. Cache identity remains available in the lease's original addresses.
 * The consumer must not retain remapped pointers after the lease's CPU fence. */
static inline const void *fh_owned_draw_bind(FhGx *state,const FhOwnedSourceLease *lease,
                                            const void *list,unsigned bytes){
    const void *owned=fh_owned_source_resolve(lease,list,bytes);if(!owned)return NULL;
    /* Validate every binding before modifying any state. */
    const void *arrays[26]={0},*pixels[8]={0},*palettes[8]={0};
    for(unsigned i=GX_VA_POS;i<=GX_VA_TEX7;i++)if(state->descriptors[i]>=GX_INDEX8){
        const FhVtxArray *a=&state->arrays[i];
        arrays[i]=fh_owned_source_resolve(lease,a->data,a->bytes);if(!arrays[i])return NULL;
    }
    unsigned used=fh_texture_dependencies(&state->draw);
    for(unsigned i=0;i<8;i++)if(used&(1u<<i)){
        const FhTexture *t=&state->draw.textures[i];if(!t->pixels)continue;
        pixels[i]=fh_owned_source_resolve(lease,t->pixels,fh_texture_source_bytes(t->format,t->width,t->height));
        if(!pixels[i])return NULL;
        if(t->palette && t->palette_count){
            palettes[i]=fh_owned_source_resolve(lease,t->palette,t->palette_count*2u);if(!palettes[i])return NULL;
        }
    }
    for(unsigned i=GX_VA_POS;i<=GX_VA_TEX7;i++)if(arrays[i])state->arrays[i].data=arrays[i];
    for(unsigned i=0;i<8;i++)if(pixels[i]){
        state->draw.textures[i].pixels=pixels[i];if(palettes[i])state->draw.textures[i].palette=palettes[i];
    }
    return owned;
}
#endif
