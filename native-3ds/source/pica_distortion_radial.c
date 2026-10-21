#include "fh_pica_distortion_radial.h"
int fh_pica_distortion_radial(uint8_t *output,size_t output_bytes,
    const uint8_t *source,size_t source_bytes)
{
    if(!output||!source||output_bytes!=FH_DISTORTION_RADIAL_BYTES||
       source_bytes!=FH_DISTORTION_RADIAL_BYTES)return 0;
    for(unsigned y=0;y<128;y++)for(unsigned x=0;x<128;x++){
        unsigned sy=127-y;
        unsigned from=(sy>>3)*32+(sy&7)+(x&3)*8+(x>>2)*512;
        unsigned morton=(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2)|
            ((x&4)<<2)|((y&4)<<3);
        unsigned to=((y>>3)*16+(x>>3))*64+morton;
        output[to]=source[from];
    }
    return 1;
}
