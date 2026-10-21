#ifndef FH_PICA_DIRECTIONAL_H
#define FH_PICA_DIRECTIONAL_H
#include <math.h>
/* The point/directional tag remains exactly 1/0. Only directions are constant
 * over a draw. Preserve the shader's small-vector floor, including zero. */
static inline void fh_pica_directional_prepare(float position[4]){
    if(position[3]!=0.f)return;
    float length2=position[0]*position[0]+position[1]*position[1]+position[2]*position[2];
    float scale=1.f/sqrtf(fmaxf(1.e-9f,length2));
    for(unsigned i=0;i<3;i++)position[i]*=scale;
}
typedef struct { float input[3],output[3];unsigned valid; } FhPicaDirectionalCache;
/* Same single renderer owner as the current light snapshot. A slot is keyed
 * by all three source components, not a source pointer or object lifetime. */
static inline unsigned fh_pica_directional_cached(FhPicaDirectionalCache *cache,float position[4]){
    if(position[3]!=0.f)return 0;
    unsigned changed=!cache->valid || cache->input[0]!=position[0] ||
        cache->input[1]!=position[1] || cache->input[2]!=position[2];
    if(changed){
        for(unsigned i=0;i<3;i++)cache->input[i]=position[i];
        fh_pica_directional_prepare(position);
        for(unsigned i=0;i<3;i++)cache->output[i]=position[i];
        cache->valid=1;
    }else for(unsigned i=0;i<3;i++)position[i]=cache->output[i];
    return changed;
}
#endif
