#ifndef FH_AUDIO_SPARSE_TAIL_H
#define FH_AUDIO_SPARSE_TAIL_H
#include <stdint.h>

/* One bit for each 32-sample region of the existing 1024-sample stop-tail
 * ring. A clear bit proves zero samples; a set bit may conservatively include
 * zero samples. The synth lock already owns publication and consumption. */
static inline void fh_depop_mark(uint32_t *dirty,unsigned cursor,unsigned count) {
    while(count) {
        unsigned index=cursor&1023u,chunk=index>>5;
        unsigned n=32u-(index&31u);if(n>count)n=count;
        *dirty|=UINT32_C(1)<<chunk;
        cursor+=n;count-=n;
    }
}

static inline void fh_depop_mix(int32_t *dest,int32_t *ring,uint32_t *dirty,
                                unsigned cursor,unsigned count) {
    if(!*dirty)return;
    while(count) {
        unsigned index=cursor&1023u,chunk=index>>5;
        unsigned n=32u-(index&31u);if(n>count)n=count;
        uint32_t bit=UINT32_C(1)<<chunk;
        if(*dirty&bit) {
            for(unsigned i=0;i<n;i++){dest[i]+=ring[index+i];ring[index+i]=0;}
            /* Partial consumption must retain the bit for the rest of a
             * region, including dormant studios and non-aligned calls. */
            if(n==32)*dirty&=~bit;
        }
        cursor+=n;count-=n;dest+=n;
    }
}
#endif
