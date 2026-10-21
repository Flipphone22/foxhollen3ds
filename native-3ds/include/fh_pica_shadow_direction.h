#ifndef FH_PICA_SHADOW_DIRECTION_H
#define FH_PICA_SHADOW_DIRECTION_H
#include <math.h>
/* Source newshadows.c direction filter, without its render-target machinery.
 * incoming points away from the light; offset points towards it. These three
 * live parameters feed the existing GPU projection, never rebuilt vertices. */
static inline int fh_pica_shadow_direction(float offset[3],const float incoming[3]){
    float v[3],dot=0;
    for(unsigned i=0;i<3;i++){
        if(!isfinite(offset[i])||!isfinite(incoming[i]))return 0;
        v[i]=incoming[i];dot-=offset[i]*v[i];
    }
    if(dot<1.f && dot>-1.f){
        float p[3],length=0;
        for(unsigned i=0;i<3;i++){p[i]=-.9f*offset[i]+.1f*v[i];length+=p[i]*p[i];}
        float mag=sqrtf(length);
        if(mag>0.f){float inv=1.f/mag;for(unsigned i=0;i<3;i++)v[i]=p[i]*inv;}
    }
    if(v[1]>-.707f){
        v[1]=-.707f;
        float inv=1.f/sqrtf(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]);
        for(unsigned i=0;i<3;i++)v[i]*=inv;
    }
    for(unsigned i=0;i<3;i++)offset[i]=-v[i];
    return 1;
}
#endif
