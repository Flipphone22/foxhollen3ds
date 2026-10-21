#ifndef FH_STATE_DELTA_H
#define FH_STATE_DELTA_H
#include <stdint.h>
#include <string.h>

/* CPU-owned state patches, not GPU command lists. Each run contains a word
 * offset/count followed by owned words. Byte copies avoid type-punning the
 * original structs and also preserve signed zero and NaN payloads. */
static inline unsigned fh_state_delta_encode(uint32_t *out, unsigned capacity,
    const void *current, void *previous, unsigned begin,const unsigned *ends,
    unsigned groups, uint32_t dirty) {
    const unsigned char *src=current;unsigned char *old=previous;
    unsigned used=0;
    for(unsigned group=0;group<groups;begin=ends[group++]) {
        if(!(dirty&(1u<<group)))continue;
        unsigned at=begin;
        while(at<ends[group]) {
            uint32_t a,b;memcpy(&a,src+at*4,4);memcpy(&b,old+at*4,4);
            if(a==b){++at;continue;}
            unsigned first=at++;
            while(at<ends[group]) {
                memcpy(&a,src+at*4,4);memcpy(&b,old+at*4,4);
                if(a==b)break;
                ++at;
            }
            unsigned count=at-first;
            if(first>65535 || count>65535 || used+1+count>capacity)return 0;
            out[used++]=first|(count<<16);
            memcpy(out+used,src+first*4,count*4);
            memcpy(old+first*4,src+first*4,count*4);used+=count;
        }
    }
    return used;
}
static inline int fh_state_delta_apply(void *state,unsigned state_words,
    const uint32_t *patch,unsigned words,unsigned check) {
    unsigned at=0;
    /* Validate the complete record before writing any state. Runs are ordered
     * and disjoint, including across separately dirty groups. */
    unsigned end=0;
    while(at<words) {
        unsigned header=patch[at++],offset=header&65535,count=header>>16;
        if(!count || offset<end || offset>state_words || count>state_words-offset || count>words-at)return 0;
        if(check && memcmp((unsigned char*)state+offset*4,patch+at,count*4))return 0;
        at+=count;end=offset+count;
    }
    if(check)return 1;
    at=0;
    while(at<words) {
        unsigned header=patch[at++],offset=header&65535,count=header>>16;
        memcpy((unsigned char*)state+offset*4,patch+at,count*4);at+=count;
    }
    return 1;
}
#endif
