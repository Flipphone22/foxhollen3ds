#ifndef FH_TEXTURE_16_H
#define FH_TEXTURE_16_H
/* Adapted from pinned Melee port/3ds/texture_repack.h at
 * c2ebe7c769b3287f2b3748173e8fe5ec24ef408e. GPL-3.0; see licenses and
 * vendor/melee/PROVENANCE.md. Same 4x4-to-Morton mapping and edge replication,
 * with bounded word transfers for native RGB565 / IA8 source tiles. */
#include <stdint.h>
#include <string.h>
static inline uint32_t fh_texture_swap16(uint32_t v) {
#ifdef __arm__
    uint32_t out;__asm__("rev16 %0, %1":"=r"(out):"r"(v));return out;
#else
    return ((v&0x00ff00ffu)<<8)|((v>>8)&0x00ff00ffu);
#endif
}
static inline uint32_t fh_texture_word(const uint8_t *p,unsigned swap) {
    uint32_t v;memcpy(&v,p,4);return swap?fh_texture_swap16(v):v;
}
static void fh_texture_repack16(void *output,unsigned width,unsigned height,
    const uint8_t *source,unsigned sw,unsigned sh,unsigned swap) {
    uint32_t *out=output;unsigned pitch=(sw+3)/4;
    static const unsigned mx[16]={0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3};
    static const unsigned my[16]={0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
    for(unsigned by=0;by<height;by+=4)for(unsigned bx=0;bx<width;bx+=4){
        uint32_t *tile=out+((by/8)*(width/8)+bx/8)*32+((by&4)?16:0)+((bx&4)?8:0);
        if(bx+4<=sw && by+4<=sh){
            const uint8_t *p=source+((by/4)*pitch+bx/4)*32;
            tile[0]=fh_texture_word(p,swap);tile[1]=fh_texture_word(p+8,swap);
            tile[2]=fh_texture_word(p+4,swap);tile[3]=fh_texture_word(p+12,swap);
            tile[4]=fh_texture_word(p+16,swap);tile[5]=fh_texture_word(p+24,swap);
            tile[6]=fh_texture_word(p+20,swap);tile[7]=fh_texture_word(p+28,swap);
        }else if(bx>=sw){
            /* Full right padding repeats the final column in each row. */
            uint32_t row[4];unsigned x=sw-1;
            for(unsigned i=0;i<4;i++){
                unsigned y=by+i<sh?by+i:sh-1;
                const uint8_t *p=source+((y/4)*pitch+x/4)*32+(y&3)*8+(x&3)*2;
                unsigned v=swap?((unsigned)p[0]<<8)|p[1]:p[0]|((unsigned)p[1]<<8);
                row[i]=v|(v<<16);
            }
            tile[0]=tile[2]=row[0];tile[1]=tile[3]=row[1];
            tile[4]=tile[6]=row[2];tile[5]=tile[7]=row[3];
        }else if(by>=sh && bx+4<=sw){
            unsigned y=sh-1;const uint8_t *p=source+((y/4)*pitch+bx/4)*32+(y&3)*8;
            uint32_t a=fh_texture_word(p,swap),b=fh_texture_word(p+4,swap);
            tile[0]=tile[1]=tile[4]=tile[5]=a;tile[2]=tile[3]=tile[6]=tile[7]=b;
        }else{
            /* Partial edge tiles retain exact per-pixel clamping. */
            for(unsigned q=0;q<16;q+=2){
                uint32_t pair=0;
                for(unsigned k=0;k<2;k++){
                    unsigned x=bx+mx[q+k],y=by+my[q+k];if(x>=sw)x=sw-1;if(y>=sh)y=sh-1;
                    const uint8_t *p=source+((y/4)*pitch+x/4)*32+(y&3)*8+(x&3)*2;
                    unsigned v=swap?((unsigned)p[0]<<8)|p[1]:p[0]|((unsigned)p[1]<<8);
                    pair|=v<<(k*16);
                }
                tile[q/2]=pair;
            }
        }
    }
}
#endif
