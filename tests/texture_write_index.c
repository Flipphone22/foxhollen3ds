#include <assert.h>
#include <stdio.h>
#include <string.h>
#define FH_WRITE_SLOTS 512
#include "fh_texture_write_index.h"
static uint32_t seed=0x1274fa32;
static uint32_t random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
static FhTextureWriteIndex index_data;
static uintptr_t addresses[512][2];
static size_t lengths[512][2];
static unsigned checks;
static void check(uintptr_t address,size_t bytes){
    uint32_t found[FH_WRITE_WORDS];
    fh_write_index_query(&index_data,(void*)address,bytes,found);
    for(unsigned s=0;s<512;s++){
        int expected=0;
        for(unsigned r=0;r<2;r++)if(addresses[s][r] && lengths[s][r]){
            /* Independent widened interval oracle, excluding wraparound. */
            __uint128_t a=addresses[s][r],b=address;
            expected|=!address || (bytes && a<b+bytes && b<a+lengths[s][r]);
        }
        int actual=(found[s/32]>>(s%32))&1;
        if(expected)assert(actual);
        if(actual){
            int exact=0;
            for(unsigned r=0;r<2;r++)exact|=fh_write_range_overlap((void*)addresses[s][r],lengths[s][r],(void*)address,bytes);
            /* Null is the global invalidation hint, including empty slots. */
            if(address)assert(exact==expected);
        }
        checks++;
    }
}
int main(void){
    for(unsigned round=0;round<6000;round++){
        unsigned slot=random32()%512;
        fh_write_index_remove(&index_data,slot);
        for(unsigned b=0;b<256;b++)assert(!(index_data.page[b][slot/32]&(1u<<(slot%32))));
        for(unsigned r=0;r<2;r++){
            uintptr_t address=(uintptr_t)0x10000000+(random32()%0x400000);
            size_t length=random32()%65537;
            if(round%37==0)address=UINTPTR_MAX-8192;
            if(round%41==0)length=2*1024*1024;
            addresses[slot][r]=address;lengths[slot][r]=length;
            fh_write_index_add(&index_data,slot,(void*)address,length);
            check(address,length);check(address,0);
            if(length && length<=UINTPTR_MAX-address){
                check(address+length-1,1);check(address+length,1);
            }
        }
        check(0,0);check(0x10000000+random32()%0x400000,random32()%65537);
        check(UINTPTR_MAX-32,128);
    }
    memset(&index_data,0,sizeof(index_data));
    memset(addresses,0,sizeof(addresses));memset(lengths,0,sizeof(lengths));
    check(0x12345,1);
    printf("Texture write index: %u slot comparisons, sizeof=%zu; no missed overlaps\n",checks,sizeof(index_data));
    return 0;
}
