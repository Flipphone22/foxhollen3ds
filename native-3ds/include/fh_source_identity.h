#ifndef FH_SOURCE_IDENTITY_H
#define FH_SOURCE_IDENTITY_H
#include "fh_owned_sources.h"
#include "fh_graphics.h"
#ifdef FH_GX_DISPATCH_BUILD
/* Only the renderer owner reads/writes this view, during one synchronous
 * translation call. It supplies cache identity, never readable original data. */
extern const FhOwnedSourceLease *fh_owned_active_sources;
static inline const FhOwnedSource *fh_source_view(const void *data,size_t bytes){
    const FhOwnedSourceLease *lease=fh_owned_active_sources;
    if(!lease || !data)return NULL;
    uintptr_t p=(uintptr_t)data;
    for(unsigned i=0;i<lease->count;i++){
        const FhOwnedSource *s=lease->source+i;uintptr_t b=(uintptr_t)s->data;
        if(p>=b && p-b<=s->bytes && bytes<=s->bytes-(p-b))return s;
    }
    return NULL;
}
static inline const void *fh_source_identity(const void *data,size_t bytes){
    const FhOwnedSource *s=fh_source_view(data,bytes);
    return s?(const void*)(s->identity+((uintptr_t)data-(uintptr_t)s->data)):data;
}
static inline unsigned fh_source_version(const void *data,size_t bytes){
    const FhOwnedSource *s=fh_source_view(data,bytes);return s?s->generation:0;
}
#else
static inline const void *fh_source_identity(const void *data,size_t bytes){(void)bytes;return data;}
static inline unsigned fh_source_version(const void *data,size_t bytes){(void)data;(void)bytes;return 0;}
#endif
static inline void fh_texture_identity(FhTexture *texture){
    texture->pixels=fh_source_identity(texture->pixels,fh_texture_source_bytes(texture->format,texture->width,texture->height));
    texture->palette=fh_source_identity(texture->palette,texture->palette_count*2u);
}
#endif
