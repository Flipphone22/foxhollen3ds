#include "fh_install_texture_layout.h"
#include "../vendor/melee/texture_visibility.h"
#include <string.h>

static unsigned word(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static unsigned chain(unsigned fmt,unsigned w,unsigned h,unsigned levels){
    unsigned bytes=0;
    for(unsigned i=0;i<levels;i++){
        bytes+=mp_texture_source_bytes(fmt,w,h);if(w>1)w>>=1;if(h>1)h>>=1;
    }
    return bytes;
}
int fh_install_texture_layout(const void*record,size_t bytes,FhInstallTextureLayout*out){
    if(!out)return 0;memset(out,0,sizeof(*out));
    if(!record||bytes<96||bytes>4u*1024*1024)return 0;
    const unsigned char*p=record;unsigned w=(unsigned)p[10]<<8|p[11],h=(unsigned)p[12]<<8|p[13];
    unsigned fmt=p[22],mask=word(p+80),min=p[28],max=p[29];
    if(!w||!h||w>1024||h>1024||min>max||max>10)return 0;
    switch(fmt){case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 14:break;default:return 0;}
    unsigned payload=(unsigned)bytes-96,base=mp_texture_source_bytes(fmt,w,h);
    if(base>payload||(mask&&(mask<base||mask>payload||mp_texture_source_bytes(0,w,h)>payload-mask)))return 0;
    FhInstallTextureLayout layout={0};layout.count=mask?2:1;layout.min_lod=min;layout.max_lod=max;layout.chain_complete=1;
    for(unsigned i=0;i<layout.count;i++){
        unsigned format=i?0:fmt,offset=96+(i?mask:0),available=i?payload-mask:mask?mask:payload;
        FhInstallTexturePlane*plane=layout.planes+i;
        plane->offset=offset;plane->available=available;
        /* GXInitTexObj receives original dimensions/data, and mip enable is
         * maxLod > minLod. The first stored image is always the base level. */
        plane->chain_bytes=chain(format,w,h,max>min?max+1:1);
        plane->image=(FhInstallExactTexture){p+offset,NULL,w,h,format,0,0,mp_texture_source_bytes(format,w,h),0};
        if(available!=plane->chain_bytes)layout.chain_complete=0;
    }
    *out=layout;return 1;
}
