#ifndef FH_GX_RING_H
#define FH_GX_RING_H
#include <stdint.h>
#include <stddef.h>
/* Single producer/consumer, following pinned Melee gx_fifo.c's monotonic
 * counters and release/acquire publication. Payloads are never published
 * until complete. Storage is caller-owned and must outlive both endpoints. */
typedef struct {
    uint32_t *data,capacity,write,reserved,cached_read,cache_reads;
    uint32_t published __attribute__((aligned(32)));
    uint32_t read __attribute__((aligned(32)));
} FhGxRing;
enum { FH_GX_RING_WRAP=255, FH_GX_RING_BAD=-1, FH_GX_RING_EMPTY=0, FH_GX_RING_READY=1 };
static inline int fh_gx_ring_init(FhGxRing *q,uint32_t *data,unsigned capacity){
    if(!data || ((uintptr_t)data&3) || capacity<4 || capacity>(1u<<23) || (capacity&(capacity-1)))return 0;
    q->data=data;q->capacity=capacity;q->write=q->reserved=q->cached_read=q->cache_reads=0;
    __atomic_store_n(&q->published,0,__ATOMIC_RELAXED);__atomic_store_n(&q->read,0,__ATOMIC_RELAXED);return 1;
}
static inline uint32_t *fh_gx_ring_reserve(FhGxRing *q,unsigned words){
    if(q->reserved || !words || words>q->capacity/2)return NULL;
    unsigned offset=q->write&(q->capacity-1),pad=offset+words>q->capacity?q->capacity-offset:0;
    unsigned used=q->write-q->cached_read;
    /* A stale retirement cursor only withholds free space. Refresh before
     * rejecting a reservation, never reuse unpublished/unretired storage. */
    if(!q->cache_reads || used>q->capacity || words+pad>q->capacity-used){
        q->cached_read=__atomic_load_n(&q->read,__ATOMIC_ACQUIRE);
        used=q->write-q->cached_read;
    }
    if(used>q->capacity || words+pad>q->capacity-used)return NULL;
    if(pad){q->data[offset]=((uint32_t)FH_GX_RING_WRAP<<24)|pad;q->write+=pad;offset=0;}
    q->reserved=words;return q->data+offset;
}
static inline int fh_gx_ring_commit(FhGxRing *q,unsigned op){
    if(!q->reserved || !op || op>=FH_GX_RING_WRAP)return 0;
    q->data[q->write&(q->capacity-1)]=(op<<24)|q->reserved;
    q->write+=q->reserved;q->reserved=0;return 1;
}
static inline int fh_gx_ring_cancel(FhGxRing *q){
    if(!q->reserved)return 0;
    /* reserve may have emitted a valid padding record. Keep its monotonic
     * cursor so publication/retirement can consume that wrap independently. */
    q->reserved=0;return 1;
}
static inline void fh_gx_ring_publish(FhGxRing *q){
    /* A producer with an uncommitted reservation must not expose its wrap. */
    if(!q->reserved)__atomic_store_n(&q->published,q->write,__ATOMIC_RELEASE);
}
static inline int fh_gx_ring_peek(FhGxRing *q,const uint32_t **record,unsigned *words,unsigned *op){
    for(unsigned wraps=0;wraps<2;wraps++){
        unsigned read=__atomic_load_n(&q->read,__ATOMIC_RELAXED),published=__atomic_load_n(&q->published,__ATOMIC_ACQUIRE);
        unsigned available=published-read;if(!available)return FH_GX_RING_EMPTY;
        if(available>q->capacity)return FH_GX_RING_BAD;
        unsigned offset=read&(q->capacity-1),header=q->data[offset],size=header&0xffffff,kind=header>>24;
        if(!size || !kind || size>available || size>q->capacity-offset)return FH_GX_RING_BAD;
        if(kind==FH_GX_RING_WRAP){
            if(size!=q->capacity-offset)return FH_GX_RING_BAD;
            __atomic_store_n(&q->read,read+size,__ATOMIC_RELEASE);continue;
        }
        *record=q->data+offset;*words=size;*op=kind;return FH_GX_RING_READY;
    }
    return FH_GX_RING_BAD;
}
static inline int fh_gx_ring_release(FhGxRing *q,const uint32_t *record,unsigned words){
    unsigned read=__atomic_load_n(&q->read,__ATOMIC_RELAXED);
    if(record!=q->data+(read&(q->capacity-1)) || !words || words!=(record[0]&0xffffff) ||
       words>__atomic_load_n(&q->published,__ATOMIC_ACQUIRE)-read)return 0;
    __atomic_store_n(&q->read,read+words,__ATOMIC_RELEASE);return 1;
}
/* One acquire owns this immutable published prefix. The producer cannot reuse
 * any part until finish releases the consumed prefix; later publications are
 * deliberately left for the next bounded worker slice. Single consumer only. */
typedef struct {uint32_t origin,read,end;} FhGxRingBatch;
static inline int fh_gx_ring_batch_begin(FhGxRing *q,FhGxRingBatch *b){
    b->origin=b->read=__atomic_load_n(&q->read,__ATOMIC_RELAXED);
    b->end=__atomic_load_n(&q->published,__ATOMIC_ACQUIRE);
    return b->end-b->read<=q->capacity;
}
static inline int fh_gx_ring_batch_peek(FhGxRing *q,FhGxRingBatch *b,const uint32_t **record,unsigned *words,unsigned *op){
    for(unsigned wraps=0;wraps<2;wraps++){
        unsigned available=b->end-b->read;if(!available)return FH_GX_RING_EMPTY;
        if(available>q->capacity)return FH_GX_RING_BAD;
        unsigned offset=b->read&(q->capacity-1),header=q->data[offset],size=header&0xffffff,kind=header>>24;
        if(!size || !kind || size>available || size>q->capacity-offset)return FH_GX_RING_BAD;
        if(kind==FH_GX_RING_WRAP){
            if(size!=q->capacity-offset)return FH_GX_RING_BAD;
            b->read+=size;continue;
        }
        *record=q->data+offset;*words=size;*op=kind;return FH_GX_RING_READY;
    }
    return FH_GX_RING_BAD;
}
static inline int fh_gx_ring_batch_release(FhGxRing *q,FhGxRingBatch *b,const uint32_t *record,unsigned words){
    if(record!=q->data+(b->read&(q->capacity-1)) || !words || words!=(record[0]&0xffffff) ||
       words>b->end-b->read)return 0;
    b->read+=words;return 1;
}
static inline int fh_gx_ring_batch_finish(FhGxRing *q,const FhGxRingBatch *b){
    if(__atomic_load_n(&q->read,__ATOMIC_RELAXED)!=b->origin || b->read-b->origin>b->end-b->origin)return 0;
    if(b->read!=b->origin)__atomic_store_n(&q->read,b->read,__ATOMIC_RELEASE);
    return 1;
}
#endif
