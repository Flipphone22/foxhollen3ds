#ifndef FH_PICA_TEXTURE_H
#define FH_PICA_TEXTURE_H
#include <stdint.h>
#include <stddef.h>
enum { FH_TEXTURE_PICA_RGB8=0x41, FH_TEXTURE_PICA_ETC1=0x4c };
static inline unsigned fh_texture_extent(unsigned n){unsigned p=8;while(p<n)p*=2;return p;}
/* ETC1 bit layout and modifier interpretation adapted from Citra/Azahar
 * src/video_core/texture/etc1.cpp, Copyright 2017 Citra Emulator Project,
 * GPL-2.0-or-later. See licenses/citra-GPL-2.0.txt and ADAPTATIONS.md.
 * PICA stores four 4x4 subtiles in each 8x8 tile, with little-endian blocks. */
static inline uint32_t fh_pica_etc1_pixel(const void *pixels,unsigned width,unsigned x,unsigned y){
    const uint8_t *p=(const uint8_t*)pixels+((y/8)*(width/8)+x/8)*32+((y&4)?16:0)+((x&4)?8:0);
    uint32_t selectors=(uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;
    uint32_t high=(uint32_t)p[4]|(uint32_t)p[5]<<8|(uint32_t)p[6]<<16|(uint32_t)p[7]<<24;
    unsigned bit=4*(x&3)+(y&3),second=((high&1)?(y&3):(x&3))>=2;
    static const int modifiers[8][2]={{2,8},{5,17},{9,29},{13,42},{18,60},{24,80},{33,106},{47,183}};
    int modifier=modifiers[(high>>(second?2:5))&7][(selectors>>bit)&1];
    if((selectors>>(bit+16))&1)modifier=-modifier;
    uint32_t result=255;
    for(unsigned c=0;c<3;c++){
        unsigned shift=24-c*8;int base;
        if(high&2){
            base=(high>>(shift+3))&31;
            if(second){int delta=(high>>shift)&7;base+=delta>=4?delta-8:delta;}
            if(base<0 || base>31)return 0x000000ff; /* Invalid differential block. */
            base=(base<<3)|(base>>2);
        }else base=((high>>(shift+(second?0:4)))&15)*17;
        int value=base+modifier;if(value<0)value=0;if(value>255)value=255;
        result|=(uint32_t)value<<(24-c*8);
    }
    return result;
}
#endif
