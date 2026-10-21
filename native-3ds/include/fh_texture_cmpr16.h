#ifndef FH_TEXTURE_CMPR16_H
#define FH_TEXTURE_CMPR16_H
/* CMPR fallback adapted from pinned Melee texture_decode.h, revision
 * c2ebe7c769b3287f2b3748173e8fe5ec24ef408e (GPL-3.0; vendor provenance).
 * Keep its GameCube endpoint/interpolation and transparent RGB semantics.
 * Deliberate 3DS detail reduction: round RGB to five bits, preserve binary
 * alpha exactly. Prepared ETC images are selected before this fallback. */
#include "../vendor/melee/texture_decode.h"
static unsigned fh_cmpr_rgb5(unsigned value){return (value*31+127)/255;}
static uint16_t fh_cmpr_5551(uint32_t rgba){
    return (uint16_t)((fh_cmpr_rgb5(rgba>>24)<<11) |
        (fh_cmpr_rgb5((rgba>>16)&255)<<6) |
        (fh_cmpr_rgb5((rgba>>8)&255)<<1) | ((rgba&255)!=0));
}
static void fh_cmpr_to_5551(uint16_t *dst,unsigned width,unsigned height,
    const uint8_t *src,unsigned sw,unsigned sh){
    unsigned pitch=(sw+7)/8;
    for(unsigned by=0;by<height;by+=4)for(unsigned bx=0;bx<width;bx+=4){
        unsigned sx=bx<sw?bx:sw-1,sy=by<sh?by:sh-1;
        const uint8_t *block=src+((sy/8)*pitch+sx/8)*32+((sy%8)/4*2+(sx%8)/4)*8;
        uint32_t rgba[4];uint16_t palette[4];mp_cmpr_palette(block,rgba);
        for(unsigned i=0;i<4;i++)palette[i]=fh_cmpr_5551(rgba[i]);
        for(unsigned y=by;y<by+4;y++)for(unsigned x=bx;x<bx+4;x++){
            unsigned tx=x<sw?x:sw-1,ty=y<sh?y:sh-1;
            unsigned selector=(block[4+ty%4]>>(6-2*(tx%4)))&3;
            dst[((y/8)*(width/8)+x/8)*64+mp_texture_morton(x,y)]=palette[selector];
        }
    }
}
#endif
