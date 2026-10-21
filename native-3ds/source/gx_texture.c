#include "gx_internal.h"
#include "fh_pica_texture.h"
#include "fh_retained_geometry.h"
#include <string.h>
#include <stdio.h>
typedef struct { FhTexture texture;void *user; } TextureObject;
typedef struct { const void *pixels;u32 format,count; } PaletteObject;
_Static_assert(sizeof(TextureObject)==sizeof(GXTexObj),"Native texture object must fit Foxhollow ABI");
_Static_assert(sizeof(PaletteObject)<=sizeof(GXTlutObj),"Palette object ABI");
static u32 serial;
extern unsigned fhTHPVideoGetNativeFormat(const void *pixels) __attribute__((weak));
extern void fhTHPVideoGetTextureSize(const void *pixels,unsigned *width,unsigned *height) __attribute__((weak));
static unsigned movie_format(const void *pixels,unsigned format) {
    return (format==0x46 || format==FH_TEXTURE_PICA_ETC1) && fhTHPVideoGetNativeFormat?fhTHPVideoGetNativeFormat(pixels):format;
}
static TextureObject *object(GXTexObj *texture) { return (TextureObject*)texture; }
#if defined(FH_GX_PRODUCER_IMPL) || !defined(FH_GX_STATE_BATCH_BUILD)
int fh_gx_texture_view(const void *texture,FhTexture *out){
    if(!texture || !out)return 0;
    *out=((const TextureObject*)texture)->texture;
    if(out->format>=8 && out->format<=10){
        unsigned slot=out->palette_count;if(slot>=32)return 0;
        out->palette=fh_gx.palettes[slot];out->palette_format=fh_gx.palette_formats[slot];
        out->palette_count=fh_gx.palette_counts[slot];
    }
    return out->pixels!=NULL;
}
#endif
void GXInitTexObj(GXTexObj *texture,const void *pixels,u16 width,u16 height,GXTexFmt format,GXTexWrapMode s,GXTexWrapMode t,GXBool mipmap) {
    if(format==0x46)format=movie_format(pixels,format);
    if(format==FH_TEXTURE_PICA_ETC1 && fhTHPVideoGetTextureSize){unsigned w=width,h=height;fhTHPVideoGetTextureSize(pixels,&w,&h);width=w;height=h;}
    if(!texture || !fh_texture_source_bytes(format,width,height)) {
        char message[160];snprintf(message,sizeof(message),"Invalid GX texture: object=%p pixels=%p extent=%ux%u format=%u",(void*)texture,pixels,width,height,(unsigned)format);fh_platform_panic(message);
    }
    memset(texture,0,sizeof(*texture));FhTexture *value=&object(texture)->texture;
    value->pixels=pixels;value->width=width;value->height=height;value->format=format;
    value->wrap_s=s;value->wrap_t=t;value->min_filter=mipmap?GX_LIN_MIP_LIN:GX_LINEAR;value->mag_filter=GX_LINEAR;
    value->serial=++serial;if(mipmap) { unsigned d=width>height?width:height;while(d>1) { value->max_lod++;d>>=1; } }
}
void GXInitTexObjCI(GXTexObj *texture,const void *pixels,u16 width,u16 height,GXCITexFmt format,GXTexWrapMode s,GXTexWrapMode t,GXBool mipmap,u32 palette) {
    GXInitTexObj(texture,pixels,width,height,(GXTexFmt)format,s,t,mipmap);object(texture)->texture.palette_count=palette;
}
void GXInitTexObjLOD(GXTexObj *texture,GXTexFilter min,GXTexFilter mag,f32 min_lod,f32 max_lod,f32 bias,GXBool clamp,GXBool edge,GXAnisotropy anisotropy) {
    (void)clamp;(void)edge;(void)anisotropy;FhTexture *value=&object(texture)->texture;
    value->min_filter=min;value->mag_filter=mag;value->min_lod=min_lod;value->max_lod=max_lod;value->lod_bias=bias;
}
void GXInitTexObjUserData(GXTexObj *texture,void *user) { object(texture)->user=user; }
void *GXGetTexObjUserData(const GXTexObj *texture) { return ((const TextureObject*)texture)->user; }
void GXInitTexObjData(GXTexObj *texture,const void *pixels) { FhTexture *t=&object(texture)->texture;t->format=movie_format(pixels,t->format);if((t->format==0x46 || t->format==FH_TEXTURE_PICA_ETC1) && fhTHPVideoGetTextureSize){unsigned w=t->width,h=t->height;fhTHPVideoGetTextureSize(pixels,&w,&h);t->width=w;t->height=h;}t->pixels=pixels;t->serial=++serial; }
void GXInitTexObjTlut(GXTexObj *texture,u32 palette) { object(texture)->texture.palette_count=palette; }
void GXInitTexObjFilter(GXTexObj *texture,GXTexFilter min,GXTexFilter mag) { object(texture)->texture.min_filter=min;object(texture)->texture.mag_filter=mag; }
void GXInitTexObjWrapMode(GXTexObj *texture,GXTexWrapMode s,GXTexWrapMode t) { object(texture)->texture.wrap_s=s;object(texture)->texture.wrap_t=t; }
void GXLoadTexObj(GXTexObj *texture,GXTexMapID id) {
    fh_gx_require((unsigned)id<8 && texture,"Invalid texture binding");
    FhTexture value=object(texture)->texture;
    if(value.format>=8 && value.format<=10) {
        unsigned palette=value.palette_count;fh_gx_require(palette<32,"Invalid texture palette slot");
        value.palette=fh_gx.palettes[palette];value.palette_format=fh_gx.palette_formats[palette];value.palette_count=fh_gx.palette_counts[palette];
    }
    fh_gx.draw.textures[id]=value;
}
void GXLoadTexObjPreLoaded(GXTexObj *texture,GXTexRegion *region,GXTexMapID id) { (void)region;GXLoadTexObj(texture,id); }
void GXPreLoadEntireTexture(const GXTexObj *texture,const GXTexRegion *region) { (void)texture;(void)region; }
u16 GXGetTexObjWidth(GXTexObj *texture) { return ((const TextureObject*)texture)->texture.width; }
u16 GXGetTexObjHeight(GXTexObj *texture) { return ((const TextureObject*)texture)->texture.height; }
GXTexFmt GXGetTexObjFmt(GXTexObj *texture) { return ((const TextureObject*)texture)->texture.format; }
u32 GXGetTexBufferSize(u16 width,u16 height,u32 format,GXBool mipmap,u8 max_lod) {
    size_t bytes=0;
    for(unsigned level=0;;level++) {
        size_t size=fh_texture_source_bytes(format,width,height);fh_gx_require(size!=0,"Invalid texture buffer format");bytes+=size;
        if(!mipmap || level>=max_lod || (width==1 && height==1)) break;
        if(width>1) width>>=1;if(height>1) height>>=1;
    }
    return bytes;
}
void GXInitTlutObj(GXTlutObj *palette,const void *pixels,GXTlutFmt format,u16 count) {
    fh_gx_require(palette && pixels && count && count<=16384 && (unsigned)format<3,"Invalid palette");
    memset(palette,0,sizeof(*palette));*(PaletteObject*)palette=(PaletteObject){pixels,format,count};
}
void GXLoadTlut(const GXTlutObj *palette,u32 id) {
    fh_gx_require(id<32 && palette,"Invalid palette binding");const PaletteObject *value=(const PaletteObject*)palette;
    fh_gx.palettes[id]=value->pixels;fh_gx.palette_formats[id]=value->format;fh_gx.palette_counts[id]=value->count;
}
void GXInvalidateTexAll(void) { fh_renderer_texture_hint(); }
extern void fh_renderer_cache_read(const void*,size_t) __attribute__((weak));
void fh_gx_cache_range(void *address,size_t bytes,int cpu_written) {
#ifdef FH_GX_DISPATCH_BUILD
    extern int fh_gx_dispatch_cache_write(const void*,unsigned);
    if(cpu_written && fh_gx_dispatch_cache_write(address,bytes))return;
    extern void fh_gx_dispatch_barrier(void);fh_gx_dispatch_barrier();
#endif
    fh_retained_scope_invalidate(address,bytes);
    if(!cpu_written && fh_renderer_cache_read)fh_renderer_cache_read(address,bytes);
    else fh_renderer_invalidate(address,bytes);
}
void GXTexModeSync(void) {}
