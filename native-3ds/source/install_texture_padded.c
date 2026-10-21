#include "fh_install_texture_padded.h"
#include "fh_pica_texture.h"
#include "../vendor/melee/texture_decode.h"
#include <stdlib.h>
#include <string.h>
static unsigned smaller(unsigned a,unsigned b){return a<b?a:b;}
static unsigned block(unsigned x,unsigned y,unsigned pitch){return(y/8*pitch+x/8)*32+(y%8/4*2+x%8/4)*8;}
int fh_install_texture_padded_cmpr_modes(const void*source,size_t source_bytes,unsigned w,unsigned h,
    void*output,size_t capacity,FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context,const unsigned char*modes,size_t mode_bytes){
    if(result)memset(result,0,sizeof(*result));
    if(!source||!output||!result||!w||!h||w>1024||h>1024||source_bytes!=(size_t)((w+7)/8)*((h+7)/8)*32)return 0;
    unsigned gw=fh_texture_extent(w),gh=fh_texture_extent(h),bytes=gw*gh/2;
    if(modes?mode_bytes!=gw*gh/64:mode_bytes!=0)return 0;
    uintptr_t a=(uintptr_t)source,b=(uintptr_t)output;
    if(a>UINTPTR_MAX-source_bytes||b>UINTPTR_MAX-capacity||(a<b+capacity&&b<a+source_bytes))return 0;
    if(progress&&!progress(context,0,gw*gh/64))return -1;
    unsigned char*padded=malloc(bytes);if(!padded)return 0;
    const unsigned char*src=source;
    for(unsigned y=0;y<gh;y+=4)for(unsigned x=0;x<gw;x+=4){
        const unsigned char*from=src+block(smaller(x,w-1),smaller(y,h-1),(w+7)/8);
        unsigned char*to=padded+block(x,y,gw/8);memcpy(to,from,4);
        for(unsigned j=0;j<4;j++){
            unsigned selectors=0;
            for(unsigned i=0;i<4;i++)selectors|=((from[4+smaller(y+j,h-1)%4]>>(6-2*(smaller(x+i,w-1)%4)))&3)<<(6-2*i);
            to[4+j]=selectors;
        }
    }
    int ok=modes?fh_install_texture_cmpr_modes(padded,bytes,gw,gh,gw,gh,output,capacity,result,progress,context,modes,mode_bytes):
        fh_install_texture_cmpr(padded,bytes,gw,gh,gw,gh,output,capacity,result,progress,context);
    free(padded);if(ok!=1)return ok;
    /* The padded encoder reports padded error; replace it with visible error.
     * ETC1A4 interleaves an8-byte alpha block before each colour block. */
    result->squared_error=0;result->visible_components=0;
    for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++){
        const unsigned char*p=src+block(x,y,(w+7)/8);uint32_t colors[4];mp_cmpr_palette(p,colors);
        uint32_t original=colors[(p[4+y%4]>>(6-2*(x%4)))&3];
        if(!(original&255))continue;
        unsigned at=block(x,y,gw/8);if(result->format==13)at=at*2+8;
        uint32_t encoded=fh_pica_etc1_pixel((const unsigned char*)output+at,8,x%4,y%4);
        for(unsigned c=0;c<3;c++){int e=(int)((original>>(24-8*c))&255)-(int)((encoded>>(24-8*c))&255);result->squared_error+=(unsigned)(e*e);}
        result->visible_components+=3;
    }
    return 1;
}
int fh_install_texture_padded_cmpr(const void*source,size_t bytes,unsigned w,unsigned h,
 void*output,size_t capacity,FhInstallTextureResult*result,FhInstallTextureProgress progress,void*context){
 return fh_install_texture_padded_cmpr_modes(source,bytes,w,h,output,capacity,result,progress,context,NULL,0);
}
