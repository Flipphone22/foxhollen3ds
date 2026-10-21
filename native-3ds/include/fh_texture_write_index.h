#ifndef FH_TEXTURE_WRITE_INDEX_H
#define FH_TEXTURE_WRITE_INDEX_H
#include <stdint.h>
#include <stddef.h>
/* Renderer-owner-only conservative page index. Collisions add candidates;
 * callers still check exact byte ranges. No pointers into GPU storage. */
enum { FH_WRITE_BUCKETS=256,FH_WRITE_WORDS=(FH_WRITE_SLOTS+31)/32 };
typedef struct {uint32_t page[FH_WRITE_BUCKETS][FH_WRITE_WORDS];} FhTextureWriteIndex;
static unsigned fh_write_pages(uintptr_t address,size_t bytes){
    if(!bytes)return 0;
    if(bytes-1>UINTPTR_MAX-address)return FH_WRITE_BUCKETS;
    uintptr_t count=((address+bytes-1)>>12)-(address>>12)+1;
    return count>=FH_WRITE_BUCKETS?FH_WRITE_BUCKETS:(unsigned)count;
}
static void fh_write_index_add(FhTextureWriteIndex *index,unsigned slot,const void *source,size_t bytes){
    if(!source)return;
    unsigned count=fh_write_pages((uintptr_t)source,bytes),first=(unsigned)((uintptr_t)source>>12);
    for(unsigned i=0;i<count;i++)index->page[(first+i)&255][slot/32]|=1u<<(slot%32);
}
static void fh_write_index_remove(FhTextureWriteIndex *index,unsigned slot){
    uint32_t mask=~(1u<<(slot%32));
    for(unsigned i=0;i<FH_WRITE_BUCKETS;i++)index->page[i][slot/32]&=mask;
}
static void fh_write_index_query(const FhTextureWriteIndex *index,const void *address,size_t bytes,uint32_t *out){
    unsigned count=address?fh_write_pages((uintptr_t)address,bytes):FH_WRITE_BUCKETS;
    if(count==FH_WRITE_BUCKETS){
        for(unsigned w=0;w<FH_WRITE_WORDS;w++)out[w]=UINT32_MAX;
        return;
    }
    for(unsigned w=0;w<FH_WRITE_WORDS;w++)out[w]=0;
    unsigned first=(unsigned)((uintptr_t)address>>12);
    for(unsigned i=0;i<count;i++)for(unsigned w=0;w<FH_WRITE_WORDS;w++)out[w]|=index->page[(first+i)&255][w];
}
static int fh_write_range_overlap(const void *source,size_t size,const void *address,size_t bytes){
    if(!address)return 1;
    if(!source || !size || !bytes)return 0;
    uintptr_t a=(uintptr_t)source,b=(uintptr_t)address;
    return b>=a?b-a<size:a-b<bytes;
}
#endif
