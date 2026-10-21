#ifndef FH_PICA_DIFFUSE_SLOTS_H
#define FH_PICA_DIFFUSE_SLOTS_H
#include <string.h>
#include "fh_pica_diffuse_map.h"
/* Three logical source slots, at most three exact resident results. No
 * tolerance, quantization or movement-dependent reuse. Images keep their
 * original separate 32x32 extent, filter and clamp behavior. */
typedef struct {
    FhPicaDiffuseMapState state[3];
    unsigned resident,valid,slot[3];
} FhPicaDiffuseSlots;
typedef struct { unsigned valid,slot[3],writes,source[3]; } FhPicaDiffuseAssignment;
static inline void fh_pica_diffuse_assign(const FhPicaDiffuseSlots *cache,
    unsigned mask,const FhPicaDiffuseMapState state[3],FhPicaDiffuseAssignment *out){
    const FhPicaDiffuseMapState *want[3]={0};unsigned reserved=0;
    memset(out,0,sizeof(*out));out->valid=cache->valid|mask;
    /* Reserve EVERY resident match before recycling a target. A later
     * logical slot may still need the image an earlier slot is abandoning. */
    for(unsigned i=0;i<3;i++){
        out->slot[i]=3;
        if(!(out->valid&(1u<<i)))continue;
        want[i]=(mask&(1u<<i))?state+i:cache->state+cache->slot[i];
        for(unsigned j=0;j<3;j++)if((cache->resident&(1u<<j)) &&
            !memcmp(want[i],cache->state+j,sizeof(*want[i]))){
            out->slot[i]=j;reserved|=1u<<j;break;
        }
    }
    for(unsigned i=0;i<3;i++)if(want[i] && out->slot[i]==3){
        for(unsigned j=0;j<3;j++)if((out->writes&(1u<<j)) &&
            !memcmp(want[i],state+out->source[j],sizeof(*want[i]))){out->slot[i]=j;break;}
        if(out->slot[i]!=3)continue;
        unsigned j=0;while(reserved&(1u<<j))j++;
        /* Three logical slots bound this search to the three targets. */
        out->slot[i]=j;out->source[j]=i;out->writes|=1u<<j;reserved|=1u<<j;
    }
}
#endif
