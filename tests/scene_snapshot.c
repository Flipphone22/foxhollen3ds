#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "fh_scene_snapshot.h"
int main(void){
    unsigned cases=0;
    for(unsigned bytes=0;bytes<2048;bytes++){
        unsigned space=fh_scene_snapshot_space(bytes);
        unsigned char *storage=malloc(space+32),*source=malloc(bytes+4);
        assert(storage && source && space>=bytes+3 && !(space&3));
        memset(storage,0xa5,space+32);unsigned at=16;
        for(unsigned i=0;i<bytes;i++)storage[at+i]=(i*73+bytes)&255;
        for(unsigned pass=0;pass<20;pass++){
            unsigned align=(pass*3)&3;
            for(unsigned i=0;i<bytes;i++)source[align+i]=(i*73+bytes)&255;
            at=fh_scene_snapshot_align(storage,at,source+align,bytes);
            assert((((uintptr_t)(storage+at)^(uintptr_t)(source+align))&3)==0);
            assert(at>=16 && at+bytes<=16+space);
            assert(!memcmp(storage+at,source+align,bytes));
            for(unsigned i=0;i<16;i++)assert(storage[i]==0xa5 && storage[16+space+i]==0xa5);
            if(bytes){source[align+bytes/2]^=128;assert(memcmp(storage+at,source+align,bytes));}
            cases++;
        }
        free(storage);free(source);
    }
    printf("PASS: %u changing snapshot alignments, exact bytes, mismatch detection and guards\n",cases);
}
