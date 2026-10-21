#ifndef FH_EFB_RGB565_H
#define FH_EFB_RGB565_H
/* Adapted from melee-3ds port/3ds/efb_rgb565.h at
 * c2ebe7c769b3287f2b3748173e8fe5ec24ef408e. GPL-3.0; see
 * native-3ds/licenses and vendor/melee/PROVENANCE.md.
 * Foxhollow retains a 640x528 logical EFB, sampled from its rotated 400x264
 * target. Caller validates the rectangle. */
#include <stdint.h>
#include <string.h>
#include "fh_screen.h"
typedef uint32_t FhEfbWord __attribute__((may_alias));
static int fh_efb_rgb565_stride(void *destination,const uint32_t *source,
    unsigned stride,unsigned width,unsigned height,const uint32_t rect[4]) {
    if((uintptr_t)destination&3)return 0;
    if(stride!=FH_EFB_NATIVE_HEIGHT){
        unsigned columns[1024],rows[1024],pitch=(width+3)/4;
        memset(destination,0,pitch*((height+3)/4)*32);
        for(unsigned x=0;x<width;x++)columns[x]=fh_screen_x(rect[0]+x*rect[2]/width)*stride;
        for(unsigned y=0;y<height;y++)rows[y]=FH_EFB_NATIVE_HEIGHT-1-(rect[1]+y*rect[3]/height)/2;
        for(unsigned by=0;by<height;by+=4)for(unsigned bx=0;bx<width;bx+=4){
            uint16_t *tile=(uint16_t*)destination+((by/4)*pitch+bx/4)*16;
            for(unsigned x=bx;x<width && x<bx+4;x++)for(unsigned y=by;y<height && y<by+4;y++){
                unsigned color=source[columns[x]+rows[y]];
                unsigned value=((color>>16)&0xf800)|((color>>13)&0x7e0)|((color>>11)&0x1f);
                tile[(y-by)*4+x-bx]=(uint16_t)((value>>8)|(value<<8));
            }
        }
        return 1;
    }
    unsigned columns[1024];uint16_t row[1024];
    unsigned pitch=(width+3)/4,previous_y=~0u;
    memset(destination,0,pitch*((height+3)/4)*32);
    for(unsigned x=0;x<width;x++)columns[x]=fh_screen_x(rect[0]+x*rect[2]/width)*stride;
    for(unsigned x=width;x<pitch*4;x++)row[x]=0;
    for(unsigned y=0;y<height;y++) {
        unsigned sy=(rect[1]+y*rect[3]/height)/2;
        if(sy!=previous_y) {
            const uint32_t *line=source+FH_EFB_NATIVE_HEIGHT-1-sy;
            for(unsigned x=0;x<width;) {
                unsigned column=columns[x],color=line[column];
                unsigned value=((color>>16)&0xf800)|((color>>13)&0x7e0)|((color>>11)&0x1f);
                uint16_t encoded=(uint16_t)((value>>8)|(value<<8));
                do{row[x++]=encoded;}while(x<width && columns[x]==column);
            }
            previous_y=sy;
        }
        FhEfbWord *tile=(FhEfbWord*)((uint8_t*)destination+(y/4)*pitch*32+(y&3)*8);
        for(unsigned x=0;x<pitch*4;x+=4,tile+=8) {
            tile[0]=(uint32_t)row[x]|((uint32_t)row[x+1]<<16);
            tile[1]=(uint32_t)row[x+2]|((uint32_t)row[x+3]<<16);
        }
    }
    return 1;
}
static int fh_efb_rgb565(void *destination,const uint32_t *source,unsigned width,unsigned height,const uint32_t rect[4]) {
    return fh_efb_rgb565_stride(destination,source,FH_EFB_NATIVE_HEIGHT,width,height,rect);
}
#endif
