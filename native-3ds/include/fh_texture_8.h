#ifndef FH_TEXTURE_8_H
#define FH_TEXTURE_8_H
/* Pinned Melee texture_repack.h mapping, specialized for GX I8 -> PICA L8.
 * GPL-3.0; see licenses and vendor/melee/PROVENANCE.md. Source tiles are
 * 8x4, destination tiles 8x8 Morton. Pair adjacent rows with bounded aligned
 * word loads. Replicated padding remains byte-exact; no depth values change. */
#include <stdint.h>
#include <string.h>
static inline uint32_t fh_texture_i8_word(const uint8_t *p){
    uint32_t v;memcpy(&v,p,4);return v;
}
static void fh_texture_repack8(void *output,unsigned width,unsigned height,
    const uint8_t *source,unsigned sw,unsigned sh){
    uint32_t *out=output;unsigned pitch=(sw+7)/8;
    static const uint8_t mx[16]={0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3};
    static const uint8_t my[16]={0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
    for(unsigned by=0;by<height;by+=4)for(unsigned bx=0;bx<width;bx+=4){
        uint32_t *tile=out+((by/8)*(width/8)+bx/8)*16+((by&4)?8:0)+((bx&4)?4:0);
        uint32_t a,b,c,d;
        if(bx+4<=sw && by+4<=sh){
            const uint8_t *p=source+((by/4)*pitch+bx/8)*32+(bx&4);
            a=fh_texture_i8_word(p);b=fh_texture_i8_word(p+8);
            c=fh_texture_i8_word(p+16);d=fh_texture_i8_word(p+24);
        }else if(bx>=sw){
            unsigned x=sw-1;uint32_t rows[4];
            for(unsigned i=0;i<4;i++){
                unsigned y=by+i<sh?by+i:sh-1;
                rows[i]=source[((y/4)*pitch+x/8)*32+(y&3)*8+(x&7)]*0x01010101u;
            }
            a=rows[0];b=rows[1];c=rows[2];d=rows[3];
        }else if(by>=sh && bx+4<=sw){
            unsigned y=sh-1;
            a=b=c=d=fh_texture_i8_word(source+((y/4)*pitch+bx/8)*32+(y&3)*8+(bx&4));
        }else{
            uint8_t *pixels=(uint8_t*)tile;
            for(unsigned q=0;q<16;q++){
                unsigned x=bx+mx[q],y=by+my[q];if(x>=sw)x=sw-1;if(y>=sh)y=sh-1;
                pixels[q]=source[((y/4)*pitch+x/8)*32+(y&3)*8+(x&7)];
            }
            continue;
        }
        tile[0]=(a&65535u)|(b<<16);tile[1]=(a>>16)|(b&0xffff0000u);
        tile[2]=(c&65535u)|(d<<16);tile[3]=(c>>16)|(d&0xffff0000u);
    }
}
#endif
