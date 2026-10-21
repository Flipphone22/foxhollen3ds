#ifndef FH_GROUND_RESULT_H
#define FH_GROUND_RESULT_H
#include "fh_ground_receiver.h"
enum { FH_GROUND_RESULT_SETS=16,FH_GROUND_RESULT_WAYS=4,FH_GROUND_RESULTS=64 };
typedef struct {
    float position[3];int origin[2];unsigned count;
    uintptr_t cells[16];int grid[16][2];uint64_t generation;
} FhGroundResultKey;
typedef struct {FhGroundResultKey key;FhGroundQuery result;} FhGroundResultEntry;
typedef struct {
    FhGroundResultEntry entry[FH_GROUND_RESULTS];
    uint8_t next[FH_GROUND_RESULT_SETS];
    uint64_t generation,hits,misses,invalidations;
} FhGroundResults;
static inline void fh_ground_results_changed(FhGroundResults*s){
    s->invalidations++;
    if(++s->generation==0){memset(s->entry,0,sizeof(s->entry));s->generation=1;}
}
static inline unsigned fh_ground_result_set(const FhGroundResultKey*k){
    uint32_t bits[3];memcpy(bits,k->position,sizeof(bits));
    uint32_t h=2166136261u;
    for(unsigned i=0;i<3;i++)h=(h^bits[i])*16777619u;
    h^=h>>16;h*=0x7feb352du;h^=h>>15;
    return h&(FH_GROUND_RESULT_SETS-1);
}
static inline int fh_ground_result_get(FhGroundResults*s,const FhGroundResultKey*k,FhGroundQuery*q){
    unsigned set=fh_ground_result_set(k);
    if(k->generation)for(unsigned i=0;i<FH_GROUND_RESULT_WAYS;i++){
        FhGroundResultEntry*e=s->entry+set*FH_GROUND_RESULT_WAYS+i;
        if(!memcmp(&e->key,k,sizeof(*k))){*q=e->result;s->hits++;return 1;}
    }
    s->misses++;return 0;
}
static inline void fh_ground_result_put(FhGroundResults*s,const FhGroundResultKey*k,const FhGroundQuery*q){
    if(!k->generation)return;
    unsigned set=fh_ground_result_set(k),way=s->next[set]++&(FH_GROUND_RESULT_WAYS-1);
    s->entry[set*FH_GROUND_RESULT_WAYS+way]=(FhGroundResultEntry){*k,*q};
}
/* Main-thread terrain/collision mutation publication; no GPU source is shared. */
void fh_native_ground_changed(void);
#endif
