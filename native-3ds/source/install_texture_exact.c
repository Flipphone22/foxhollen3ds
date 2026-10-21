#include "fh_install_texture_exact.h"
#include "fh_pica_texture.h"
#include "../vendor/melee/texture_repack.h"
#include "../vendor/melee/texture_visibility.h"

static int overlap(const void*a,size_t an,const void*b,size_t bn){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x>UINTPTR_MAX-an||y>UINTPTR_MAX-bn||(x<y+bn&&y<x+an);
}
int fh_install_texture_exact(const FhInstallExactTexture*s,void*out,size_t capacity,unsigned*format,unsigned*bytes,FhInstallExactProgress progress,void*context){
    if(format)*format=0;if(bytes)*bytes=0;
    if(!s||!out||!format||!bytes||!s->pixels||((uintptr_t)out&3)||!s->width||!s->height||s->width>1024||s->height>1024)return 0;
    switch(s->format){case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 8:case 9:case 10:case 14:break;default:return 0;}
    size_t needed=mp_texture_source_bytes(s->format,s->width,s->height);
    if(s->pixel_bytes!=needed)return 0;
    unsigned indexed=s->format>=8&&s->format<=10;
    if(indexed){
        unsigned maximum=s->format==8?16:s->format==9?256:16384;
        if(!s->palette||!s->palette_count||s->palette_count>maximum||s->palette_format>2||s->palette_bytes!=s->palette_count*2u)return 0;
    }else if(s->palette||s->palette_count||s->palette_bytes)return 0;
    unsigned w=fh_texture_extent(s->width),h=fh_texture_extent(s->height),fmt=mp_texture_format(s->format,s->palette_format);
    unsigned n=w*h*mp_texture_bits(fmt)/8;
    if(capacity<n||overlap(out,n,s->pixels,s->pixel_bytes)||(indexed&&overlap(out,n,s->palette,s->palette_bytes)))return 0;
    if(progress&&!progress(context,0,h))return -1;
    if(s->format==14)mp_cmpr_to_native(out,w,h,s->pixels,s->width,s->height);
    else{
        MPTextureSource source={s->pixels,s->palette,s->width,s->height,s->format,s->palette_format,s->palette_count};
        if(!mp_texture_repack(out,w,h,&source))return 0;
    }
    if(progress&&!progress(context,h,h))return -1;
    *format=fmt;*bytes=n;return 1;
}
