#include "fh_graphics.h"
#include <string.h>
#include "fh_texture_16.h"
#include "fh_texture_8.h"
#include "fh_pica_texture.h"
#include "../vendor/melee/texture_repack.h"
#include "../vendor/melee/texture_visibility.h"
#include "fh_texture_cmpr16.h"

int fh_texture_convert_cmpr16(void *output,size_t output_bytes,unsigned width,unsigned height,
    const FhTexture *texture,size_t source_bytes){
    if(!texture || texture->format!=14 || !texture->pixels || !output || ((uintptr_t)output&3))return 0;
    size_t required=fh_texture_source_bytes(14,texture->width,texture->height);
    if(!required || source_bytes<required || !fh_texture_native_bytes(texture,width,height) ||
       width<texture->width || height<texture->height || output_bytes<(size_t)width*height*2)return 0;
    fh_cmpr_to_5551(output,width,height,texture->pixels,texture->width,texture->height);return 1;
}

size_t fh_texture_source_bytes(unsigned format,unsigned width,unsigned height) {
    if(!width || !height || width>1024 || height>1024) return 0;
    if(format==0x46) return (size_t)width*height*4;
    if(format==FH_TEXTURE_PICA_ETC1) return (size_t)fh_texture_extent(width)*fh_texture_extent(height)/2;
    switch(format) { case 0:case 1:case 2:case 3:case 4:case 5:case 6:case 8:case 9:case 10:case 14: break;default:return 0; }
    return mp_texture_source_bytes(format,width,height);
}
unsigned fh_texture_native_format(const FhTexture *texture) { return texture->format==FH_TEXTURE_PICA_ETC1?12:mp_texture_format(texture->format,texture->palette_format); }
size_t fh_texture_native_bytes(const FhTexture *texture,unsigned width,unsigned height) {
    if(!texture || width<8 || height<8 || width>1024 || height>1024 || (width&(width-1)) || (height&(height-1))) return 0;
    if(texture->format==FH_TEXTURE_PICA_ETC1) return (size_t)width*height/2;
    return (size_t)width*height*mp_texture_bits(fh_texture_native_format(texture))/8;
}
int fh_texture_convert(void *output,size_t output_bytes,unsigned width,unsigned height,const FhTexture *texture,size_t source_bytes) {
    if(!texture || !texture->pixels || !output || ((uintptr_t)output&3)) return 0;
    size_t required=fh_texture_source_bytes(texture->format,texture->width,texture->height);
    size_t destination=fh_texture_native_bytes(texture,width,height);
    if(!required || source_bytes<required || !destination || output_bytes<destination || width<texture->width || height<texture->height) return 0;
    if(texture->format==FH_TEXTURE_PICA_ETC1) {
        if(width!=fh_texture_extent(texture->width) || height!=fh_texture_extent(texture->height))return 0;
        memcpy(output,texture->pixels,destination);return 1;
    }
    if(texture->format>=8 && texture->format<=10 && (!texture->palette || !texture->palette_count || texture->palette_count>16384 || texture->palette_format>2)) return 0;
    if(texture->format==14) { mp_cmpr_to_native(output,width,height,texture->pixels,texture->width,texture->height);return 1; }
    if(texture->format==1 && !((uintptr_t)texture->pixels&3)){
        fh_texture_repack8(output,width,height,texture->pixels,texture->width,texture->height);return 1;
    }
    if((texture->format==3 || texture->format==4) && !((uintptr_t)texture->pixels&3)){
        fh_texture_repack16(output,width,height,texture->pixels,texture->width,texture->height,texture->format==4);return 1;
    }
    if(texture->format==0x46) {
        const uint8_t *source=texture->pixels;uint32_t *pixels=output;
        if((uintptr_t)source&3){
            /* Do not depend on ARM11's unaligned-load control setting. */
            for(unsigned y=0;y<height;y++)for(unsigned x=0;x<width;x++){
                unsigned sx=x<texture->width?x:texture->width-1,sy=y<texture->height?y:texture->height-1;
                const volatile uint8_t *p=source+(sy*texture->width+sx)*4;
                pixels[((y/8)*(width/8)+x/8)*64+mp_texture_morton(x,y)]=mp_rgba(p[0],p[1],p[2],p[3]);
            }
            return 1;
        }
        source=__builtin_assume_aligned(source,4);
        static const unsigned spread[8]={0,1,4,5,16,17,20,21};
        /* Like Melee's tiled repacker, compute tile bases outside the pixel
         * loop. Native movie RGBA is linear; REV supplies the PICA byte order.
         * Every source pixel is still read, including exact edge replication. */
        for(unsigned by=0;by<height;by+=8)for(unsigned bx=0;bx<width;bx+=8){
            uint32_t *tile=pixels+by*width+bx*8;
            for(unsigned y=0;y<8;y++){
                unsigned sy=by+y<texture->height?by+y:texture->height-1;
                const uint8_t *row=source+sy*texture->width*4;
                unsigned oy=spread[y]*2;
                for(unsigned x=0;x<8;x++){
                    unsigned sx=bx+x<texture->width?bx+x:texture->width-1;
                    uint32_t value;memcpy(&value,row+sx*4,4);
                    tile[oy+spread[x]]=__builtin_bswap32(value);
                }
            }
        }
        return 1;
    }
    MPTextureSource source={texture->pixels,texture->palette,texture->width,texture->height,texture->format,texture->palette_format,texture->palette_count};
    return mp_texture_repack(output,width,height,&source);
}
