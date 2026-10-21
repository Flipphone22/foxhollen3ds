#ifndef FH_OWNED_SOURCES_H
#define FH_OWNED_SOURCES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Producer-owned metadata, immutable CPU-reader payloads. Adapted in concept
 * from pinned Melee source_snapshot_cache.h; SFA has no complete source-write
 * epoch, so every new acquisition compares bytes. Explicit immutable owners
 * can share a validated snapshot without rereading originals. Never give this normal-memory
 * arena to the GPU. A lease is released only after its CPU-reader fence.
 *
 * Unlike Melee's malloc-per-entry cache, storage is supplied once by the caller
 * and cannot grow. Pinned versions survive updates to the original address.
 * No compaction moves a reader's pointer. Exhaustion declines before eviction.
 */
enum { FH_OWNED_SOURCE_SLOTS=128, FH_OWNED_SOURCE_ALIGNMENT=8,
       FH_OWNED_LEASE_SOURCES=32 };
typedef struct {
    uintptr_t identity;
    const unsigned char *data;
    uint32_t bytes,slot,generation;
} FhOwnedSource;
typedef struct {
    uintptr_t identity;
    uint32_t bytes,offset,span,generation,pins,stamp;
} FhOwnedSourceEntry;
typedef struct {
    unsigned char *storage;
    uint32_t capacity,cursor,serial,clock,used,peak;
    uint32_t copies,reuses,declines,shares;
    uint64_t copied_bytes,reused_bytes,shared_bytes;
    FhOwnedSourceEntry entry[FH_OWNED_SOURCE_SLOTS];
} FhOwnedSources;
typedef struct {
    FhOwnedSource source[FH_OWNED_LEASE_SOURCES];
    uint32_t count;
} FhOwnedSourceLease;

static inline int fh_owned_sources_init(FhOwnedSources *pool,void *storage,size_t bytes){
    if(!pool || !storage || (uintptr_t)storage%FH_OWNED_SOURCE_ALIGNMENT ||
       !bytes || bytes>UINT32_MAX || bytes%FH_OWNED_SOURCE_ALIGNMENT ||
       bytes>UINTPTR_MAX-(uintptr_t)storage)return 0;
    memset(pool,0,sizeof(*pool));pool->storage=storage;pool->capacity=(uint32_t)bytes;
    return 1;
}
static inline int fh_owned_overlap(uintptr_t a,size_t n,uintptr_t b,size_t m){
    return n && m && (a>=b?a-b<m:b-a<n);
}
/* Different resource lifetimes leave pinned islands. Search bounded gaps
 * instead of allowing one long-lived model at the ring head to block every
 * subsequent texture version. No metadata or payload changes on failure. */
static inline int fh_owned_source_space(const FhOwnedSources *pool,uint32_t span,uint32_t *result){
    for(unsigned pass=0;pass<2;pass++){
        uint32_t at=pass?0:pool->cursor;
        for(unsigned step=0;step<=FH_OWNED_SOURCE_SLOTS && at<=pool->capacity-span;step++){
            uint32_t next=at;
            for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++){
                const FhOwnedSourceEntry *e=pool->entry+i;
                if(e->pins && fh_owned_overlap(at,span,e->offset,e->span) && e->offset+e->span>next)
                    next=e->offset+e->span;
            }
            if(next==at){*result=at;return 1;}
            at=next;
        }
    }
    return 0;
}
static inline int fh_owned_source_acquire(FhOwnedSources *pool,FhOwnedSourceLease *lease,
                                         const void *source,size_t bytes){
    uintptr_t identity=(uintptr_t)source;
    if(!pool || !lease || !pool->storage || !source || !bytes ||
       bytes>pool->capacity || bytes>UINTPTR_MAX-identity ||
       lease->count>=FH_OWNED_LEASE_SOURCES ||
       fh_owned_overlap(identity,bytes,(uintptr_t)pool->storage,pool->capacity))return 0;
    /* A transaction has one coherent view of overlapping original ranges.
     * Source changes during construction must decline, not make resolution
     * depend on the order of two contradictory snapshots. */
    for(unsigned i=0;i<lease->count;i++){
        const FhOwnedSource *s=&lease->source[i];
        if(!fh_owned_overlap(identity,bytes,s->identity,s->bytes))continue;
        uintptr_t begin=identity>s->identity?identity:s->identity;
        size_t end_a=bytes-(begin-identity),end_b=s->bytes-(begin-s->identity);
        size_t common=end_a<end_b?end_a:end_b;
        if(memcmp((const unsigned char*)source+(begin-identity),s->data+(begin-s->identity),common))return 0;
        if(identity>=s->identity && bytes<=s->bytes-(identity-s->identity))return 1;
    }
    /* Clock wrap affects replacement preference only, never identity. */
    if(!++pool->clock){for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++)pool->entry[i].stamp=0;++pool->clock;}
    unsigned slot=FH_OWNED_SOURCE_SLOTS;
    for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++){
        FhOwnedSourceEntry *e=&pool->entry[i];
        if(e->bytes && e->identity==identity && e->bytes==bytes && e->pins<UINT32_MAX &&
           !memcmp(pool->storage+e->offset,source,bytes)){
            ++e->pins;e->stamp=pool->clock;
            lease->source[lease->count++]=(FhOwnedSource){identity,pool->storage+e->offset,(uint32_t)bytes,i,e->generation};
            ++pool->reuses;pool->reused_bytes+=bytes;return 1;
        }
        if(!e->pins && (slot==FH_OWNED_SOURCE_SLOTS || !e->bytes ||
           (pool->entry[slot].bytes && e->stamp<pool->entry[slot].stamp)))slot=i;
    }
    if(slot==FH_OWNED_SOURCE_SLOTS || pool->serial==UINT32_MAX){++pool->declines;return 0;}
    uint32_t span=((uint32_t)bytes+7u)&~7u;
    if(span<bytes || span>pool->capacity){++pool->declines;return 0;}
    /* Check all readers BEFORE replacing metadata or writing payload bytes.
     * The caller can drain a fence and retry; allocation never waits here. */
    uint32_t offset;
    if(!fh_owned_source_space(pool,span,&offset)){++pool->declines;return 0;}
    for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++){
        FhOwnedSourceEntry *e=&pool->entry[i];
        if(e->bytes && (i==slot || fh_owned_overlap(offset,span,e->offset,e->span))){
            pool->used-=e->span;memset(e,0,sizeof(*e));
        }
    }
    memcpy(pool->storage+offset,source,bytes);
    FhOwnedSourceEntry *e=&pool->entry[slot];
    *e=(FhOwnedSourceEntry){identity,(uint32_t)bytes,offset,span,++pool->serial,1,pool->clock};
    pool->cursor=offset+span;pool->used+=span;if(pool->used>pool->peak)pool->peak=pool->used;
    lease->source[lease->count++]=(FhOwnedSource){identity,pool->storage+offset,(uint32_t)bytes,slot,e->generation};
    ++pool->copies;pool->copied_bytes+=bytes;return 1;
}
static inline const void *fh_owned_source_resolve(const FhOwnedSourceLease *lease,
                                                const void *source,size_t bytes){
    uintptr_t identity=(uintptr_t)source;
    if(!lease || lease->count>FH_OWNED_LEASE_SOURCES || !source || !bytes ||
       bytes>UINTPTR_MAX-identity)return NULL;
    for(unsigned i=0;i<lease->count;i++){
        const FhOwnedSource *s=&lease->source[i];
        if(identity>=s->identity && identity-s->identity<=s->bytes &&
           bytes<=s->bytes-(identity-s->identity))return s->data+(identity-s->identity);
    }
    return NULL;
}
/* Transfer a reference to an already-owned immutable snapshot. This never
 * reads the original address. The caller must establish that the owner still
 * describes the desired source version (e.g. a model's immutable scope).
 * Metadata belongs to the producer; consumers read only published leases. */
static inline int fh_owned_source_share(FhOwnedSources *pool,FhOwnedSourceLease *to,
    const FhOwnedSourceLease *owner,const void *source,size_t bytes){
    uintptr_t address=(uintptr_t)source;
    if(!pool || !to || !owner || !source || !bytes || bytes>UINTPTR_MAX-address ||
       to->count>FH_OWNED_LEASE_SOURCES || owner->count>FH_OWNED_LEASE_SOURCES)return 0;
    for(unsigned i=0;i<owner->count;i++){
        const FhOwnedSource *s=owner->source+i;
        if(address<s->identity || address-s->identity>s->bytes || bytes>s->bytes-(address-s->identity))continue;
        if(s->slot>=FH_OWNED_SOURCE_SLOTS)return 0;
        FhOwnedSourceEntry *entry=pool->entry+s->slot;
        if(!entry->pins || entry->generation!=s->generation || entry->identity!=s->identity ||
           entry->bytes!=s->bytes || pool->storage+entry->offset!=s->data)return 0;
        for(unsigned j=0;j<to->count;j++){
            const FhOwnedSource *old=to->source+j;
            if(old->slot==s->slot && old->generation==s->generation)return 1;
            if(fh_owned_overlap(s->identity,s->bytes,old->identity,old->bytes)){
                uintptr_t begin=s->identity>old->identity?s->identity:old->identity;
                size_t a=s->bytes-(begin-s->identity),b=old->bytes-(begin-old->identity);
                if(memcmp(s->data+(begin-s->identity),old->data+(begin-old->identity),a<b?a:b))return 0;
            }
        }
        if(to->count==FH_OWNED_LEASE_SOURCES || entry->pins==UINT32_MAX)return 0;
        ++entry->pins;to->source[to->count++]=*s;++pool->shares;pool->shared_bytes+=bytes;return 1;
    }
    return 0;
}
static inline int fh_owned_source_release(FhOwnedSources *pool,FhOwnedSourceLease *lease){
    if(!pool || !lease || lease->count>FH_OWNED_LEASE_SOURCES)return 0;
    /* The ordinary dispatcher also retires empty leases while copied-source
     * mode is disabled. There are no pins to inspect or change in that case. */
    if(!lease->count)return 1;
    unsigned references[FH_OWNED_SOURCE_SLOTS]={0};
    /* A malformed/stale lease cannot partially unpin another reader's data. */
    for(unsigned i=0;i<lease->count;i++){
        const FhOwnedSource *s=&lease->source[i];
        if(s->slot>=FH_OWNED_SOURCE_SLOTS)return 0;
        const FhOwnedSourceEntry *e=&pool->entry[s->slot];
        if(e->generation!=s->generation || e->identity!=s->identity || e->bytes!=s->bytes ||
           pool->storage+e->offset!=s->data || ++references[s->slot]>e->pins)return 0;
    }
    for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++)pool->entry[i].pins-=references[i];
    lease->count=0;return 1;
}
/* A source write retires the producer's reusable reference only. Child draws
 * already published keep their immutable versions until CPU acknowledgement. */
static inline int fh_owned_source_forget(FhOwnedSources *pool,FhOwnedSourceLease *owner,
                                         const void *address,size_t bytes){
    if(!pool || !owner || owner->count>FH_OWNED_LEASE_SOURCES)return 0;
    if(!owner->count)return 1;
    FhOwnedSourceLease retired={0};uint32_t selected=0;
    for(unsigned i=0;i<owner->count;i++){
        const FhOwnedSource *s=owner->source+i;
        if(!address || fh_owned_overlap((uintptr_t)address,bytes,s->identity,s->bytes)){
            retired.source[retired.count++]=*s;selected|=1u<<i;
        }
    }
    if(!fh_owned_source_release(pool,&retired))return 0;
    unsigned kept=0;
    for(unsigned i=0;i<owner->count;i++)if(!(selected&(1u<<i)))owner->source[kept++]=owner->source[i];
    owner->count=kept;return 1;
}
#endif
