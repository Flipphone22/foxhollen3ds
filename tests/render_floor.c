#include "fh_render_floor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    unsigned checked=0;
    for(unsigned pos=0;pos<64;pos++)for(unsigned count=0;count<16;count++){
        unsigned char stream[32];memset(stream,0xa5,sizeof(stream));
        for(unsigned bit=0;bit<4;bit++){
            unsigned at=pos+bit,mask=1u<<(at&7);
            stream[at>>3]=(stream[at>>3]&~mask)|(((count>>bit)&1)<<(at&7));
        }
        unsigned before=pos,decoded=0;
        for(unsigned bit=0;bit<4;bit++,before++)decoded|=((stream[before>>3]>>(before&7))&1)<<bit;
        for(unsigned i=0;i<decoded;i++)for(unsigned bit=0;bit<8;bit++)++before;
        assert(fh_render_floor_matrix_end(stream,pos)==before);++checked;
    }
    printf("%u model instruction cursor cases passed\n",checked);
}
