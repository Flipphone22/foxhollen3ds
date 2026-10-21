#ifndef FH_GROUND_MASK_H
#define FH_GROUND_MASK_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
/* Collision topology cell masks are immutable after mapBlockUnpackPolygons.
 * Vertices, group bounds/flags and collision offsets are NOT cached here. */
enum { FH_GROUND_MASK_OWNERS=128,FH_GROUND_MASK_BUDGET=128*1024 };
typedef struct {const void *owner,*source;uint32_t *bits;unsigned count,words;} FhGroundMaskIndex;
typedef struct {
    FhGroundMaskIndex index[FH_GROUND_MASK_OWNERS];
    unsigned bytes,peak,builds,declined,releases,hits,misses;
} FhGroundMaskStore;
typedef struct {const FhGroundMaskIndex *index;unsigned word,start,end,mask,next;uint32_t remaining;} FhGroundMaskCursor;
static inline void fh_ground_mask_forget(FhGroundMaskStore*s,const void *base,unsigned bytes){
    uintptr_t begin=(uintptr_t)base;
    for(unsigned i=0;i<FH_GROUND_MASK_OWNERS;i++){
        FhGroundMaskIndex *p=s->index+i;
        if(p->owner && (uintptr_t)p->owner>=begin && (uintptr_t)p->owner-begin<bytes){
            s->bytes-=p->words*64;free(p->bits);memset(p,0,sizeof(*p));s->releases++;
        }
    }
}
static inline int fh_ground_mask_load(FhGroundMaskStore*s,const void *owner,const uint16_t *polygons,unsigned count){
    fh_ground_mask_forget(s,owner,1); /* Explicit reload, including same address. */
    if(!count)return 1;
    if(!owner||!polygons||count>65535){s->declined++;return 0;}
    unsigned words=(count+31)/32,bytes=words*64,slot=0;
    while(slot<FH_GROUND_MASK_OWNERS && s->index[slot].owner)slot++;
    if(slot==FH_GROUND_MASK_OWNERS || bytes>FH_GROUND_MASK_BUDGET-s->bytes){s->declined++;return 0;}
    uint32_t *bits=calloc(1,bytes);if(!bits){s->declined++;return 0;}
    for(unsigned t=0;t<count;t++){
        unsigned mask=polygons[t*4+3];
        while(mask){unsigned bit=__builtin_ctz(mask);mask&=mask-1;bits[bit*words+t/32]|=1u<<(t%32);}
    }
    s->index[slot]=(FhGroundMaskIndex){owner,polygons,bits,count,words};
    s->bytes+=bytes;if(s->bytes>s->peak)s->peak=s->bytes;s->builds++;return 1;
}
static inline const FhGroundMaskIndex *fh_ground_mask_find(FhGroundMaskStore*s,const void *owner,const void *source,unsigned count){
    for(unsigned i=0;i<FH_GROUND_MASK_OWNERS;i++){
        const FhGroundMaskIndex*p=s->index+i;
        if(p->owner==owner && p->source==source && p->count==count){s->hits++;return p;}
    }
    s->misses++;return NULL; /* No omission: caller uses original mask scan. */
}
static inline void fh_ground_mask_begin(FhGroundMaskCursor*c,const FhGroundMaskIndex*p,unsigned start,unsigned end,unsigned mask){
    *c=(FhGroundMaskCursor){.index=p,.word=start/32,.start=start,.end=end,.mask=mask&65535,.next=start};
    if(p && end>p->count)c->index=NULL; /* Preserve source behavior on unknown ranges. */
}
static inline unsigned fh_ground_mask_next(FhGroundMaskCursor*c){
    if(!c->index)return c->next<c->end?c->next++:c->end;
    for(;;){
        if(c->remaining){unsigned bit=__builtin_ctz(c->remaining);c->remaining&=c->remaining-1;
            unsigned t=(c->word-1)*32+bit;return t<c->end?t:c->end;}
        if(c->word*32>=c->end || !(c->mask&255) || !(c->mask&65280))return c->end;
        const FhGroundMaskIndex*p=c->index;uint32_t x=0,z=0;unsigned mask=c->mask&255;
        while(mask){unsigned bit=__builtin_ctz(mask);mask&=mask-1;x|=p->bits[bit*p->words+c->word];}
        mask=c->mask&65280;
        while(mask){unsigned bit=__builtin_ctz(mask);mask&=mask-1;z|=p->bits[bit*p->words+c->word];}
        c->remaining=x&z;
        if(c->word==c->start/32)c->remaining&=~0u<<(c->start%32);
        c->word++;
    }
}
void fh_native_ground_mask_load(const void*,const void*,unsigned);
void fh_native_ground_mask_forget(const void*,unsigned);
#endif
