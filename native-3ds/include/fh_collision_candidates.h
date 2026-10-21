#ifndef FH_COLLISION_CANDIDATES_H
#define FH_COLLISION_CANDIDATES_H
#include "fh_ground_mask.h"
/* Main game thread. Only immutable topology masks are cached, never geometry.
 * Load/forget invalidates every key before source storage can be reused. */
enum {FH_COLLISION_CANDIDATE_SLOTS=8,FH_COLLISION_CANDIDATE_LIMIT=1024};
typedef struct {
    const void *owner,*source;
    unsigned count,mask,length,valid;
    uint16_t indices[FH_COLLISION_CANDIDATE_LIMIT];
} FhCollisionCandidates;
typedef struct {
    FhCollisionCandidates slot[FH_COLLISION_CANDIDATE_SLOTS];
    unsigned next,hits,misses,overflow;
} FhCollisionCandidateStore;
static inline void fh_collision_candidates_clear(FhCollisionCandidateStore*s){
    for(unsigned i=0;i<FH_COLLISION_CANDIDATE_SLOTS;i++)s->slot[i].valid=0;
    s->next=0;
}
static inline const FhCollisionCandidates *fh_collision_candidates_get(
        FhCollisionCandidateStore*s,FhGroundMaskStore*m,const void*owner,
        const void*source,unsigned count,unsigned mask){
    mask&=65535;
    for(unsigned i=0;i<FH_COLLISION_CANDIDATE_SLOTS;i++){
        const FhCollisionCandidates*p=s->slot+i;
        if(p->valid&&p->owner==owner&&p->source==source&&p->count==count&&p->mask==mask){
            s->hits++;return p->valid==1?p:NULL;
        }
    }
    s->misses++;
    const FhGroundMaskIndex*index=fh_ground_mask_find(m,owner,source,count);
    if(!index)return NULL;
    FhCollisionCandidates*p=s->slot+s->next;
    s->next=(s->next+1)%FH_COLLISION_CANDIDATE_SLOTS;
    p->valid=0;p->owner=owner;p->source=source;p->count=count;p->mask=mask;p->length=0;
    FhGroundMaskCursor cursor;fh_ground_mask_begin(&cursor,index,0,count,mask);
    for(unsigned t=fh_ground_mask_next(&cursor);t<count;t=fh_ground_mask_next(&cursor)){
        if(p->length==FH_COLLISION_CANDIDATE_LIMIT){
            /* Remember overflow, so broad queries do not rebuild on every call. */
            p->valid=2;s->overflow++;return NULL;
        }
        p->indices[p->length++]=(uint16_t)t;
    }
    p->valid=1;return p;
}
static inline unsigned fh_collision_candidates_lower(const FhCollisionCandidates*p,unsigned first){
    unsigned lo=0,hi=p->length;
    while(lo<hi){unsigned mid=lo+(hi-lo)/2;if(p->indices[mid]<first)lo=mid+1;else hi=mid;}
    return lo;
}
#endif
