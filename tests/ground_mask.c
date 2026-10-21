#include "fh_ground_mask.h"
#include <assert.h>
#include <stdio.h>
static uint32_t rng=901;
static uint32_t rand32(void){return rng=rng*1664525u+1013904223u;}
int main(void){
    FhGroundMaskStore store={0};uint16_t tris[4096*4];unsigned checks=0;
    for(unsigned trial=0;trial<100;trial++){
        unsigned count=1+rand32()%4096;
        for(unsigned t=0;t<count;t++)tris[4*t+3]=(uint16_t)rand32();
        assert(fh_ground_mask_load(&store,tris,tris,count));
        const FhGroundMaskIndex*p=fh_ground_mask_find(&store,tris,tris,count);assert(p);
        for(unsigned query=0;query<1000;query++){
            unsigned begin=rand32()%count,end=begin+rand32()%(count-begin+1),mask=rand32()&65535;
            FhGroundMaskCursor cursor;fh_ground_mask_begin(&cursor,p,begin,end,mask);
            unsigned actual=fh_ground_mask_next(&cursor);
            for(unsigned t=begin;t<end;t++)if((mask&tris[4*t+3]&255)&&(mask&tris[4*t+3]&65280)){
                assert(actual==t);actual=fh_ground_mask_next(&cursor);checks++;
            }
            assert(actual==end);
        }
        /* A missing index enumerates every source triangle, preserving the
         * caller's original per-triangle test and order rather than omitting. */
        FhGroundMaskCursor c;fh_ground_mask_begin(&c,NULL,3,32,1);
        for(unsigned i=3;i<32;i++)assert(fh_ground_mask_next(&c)==i);
        assert(fh_ground_mask_next(&c)==32);
    }
    fh_ground_mask_forget(&store,tris,sizeof(tris));assert(!store.bytes);
    /* Explicit reload replaces topology even at an identical allocation. */
    tris[3]=0x0101;assert(fh_ground_mask_load(&store,tris,tris,1));
    tris[3]=0x0202;assert(fh_ground_mask_load(&store,tris,tris,1));
    FhGroundMaskCursor c;fh_ground_mask_begin(&c,fh_ground_mask_find(&store,tris,tris,1),0,1,0x0101);
    assert(fh_ground_mask_next(&c)==1);
    fh_ground_mask_forget(&store,tris,sizeof(tris));
    unsigned char owners[FH_GROUND_MASK_OWNERS+1];
    for(unsigned i=0;i<FH_GROUND_MASK_OWNERS;i++)assert(fh_ground_mask_load(&store,owners+i,tris,1));
    assert(!fh_ground_mask_load(&store,owners+FH_GROUND_MASK_OWNERS,tris,1));
    fh_ground_mask_forget(&store,owners,sizeof(owners));assert(!store.bytes);
    uint16_t *large=calloc(65535*4,sizeof(uint16_t));assert(large);
    assert(fh_ground_mask_load(&store,large,large,65535));assert(store.bytes==FH_GROUND_MASK_BUDGET);
    assert(!fh_ground_mask_load(&store,owners,tris,1));
    fh_ground_mask_forget(&store,large,1);free(large);assert(!store.bytes);
    printf("PASS %u exact ordered mask selections, same-address reload, owner/byte caps and scan fallback\n",checks);
}
