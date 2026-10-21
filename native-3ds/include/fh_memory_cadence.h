#ifndef FH_MEMORY_CADENCE_H
#define FH_MEMORY_CADENCE_H
#include <stdint.h>
/* Sampling stays at the existing game-thread boundary. Only serialization is
 * coalesced: every new sampled pressure maximum emits a full, ordinary row.
 * Fixed storage; no queue, allocation, worker or file operation here. */
enum { FH_MEMORY_PRESSURES=36 };
typedef struct {
    uint64_t next;
    unsigned initialized,samples,rows,pressure[FH_MEMORY_PRESSURES];
} FhMemoryCadence;
static inline int fh_memory_cadence_due(FhMemoryCadence *s,uint64_t now,
    uint64_t second,unsigned force,const unsigned pressure[FH_MEMORY_PRESSURES]){
    unsigned due=force || !s->initialized || now>=s->next;
    s->samples++;
    for(unsigned i=0;i<FH_MEMORY_PRESSURES;i++)if(pressure[i]>s->pressure[i]){
        s->pressure[i]=pressure[i];due=1;
    }
    if(!due)return 0;
    s->initialized=1;s->next=now+5*second;s->rows++;
    return 1;
}
#endif
