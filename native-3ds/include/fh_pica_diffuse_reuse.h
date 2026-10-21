#pragma once
#include <math.h>
#include "fh_pica_diffuse_map.h"
/* Bound the change in the source map before interpolation/8-bit storage.
 * Authored grid normals have length <2, and map positions have |axis|<=2.
 * |normalize(a)-normalize(b)| <= 2|a-b|/|a|. The L1 numerator and maximum
 * component denominator conservatively bound those Euclidean lengths.
 * Each light gets 1/count of a 1/512 RGB error budget. Ambient and light
 * colours must match exactly. Compare against the LAST DRAWN state, never
 * the last request, so small movements cannot accumulate invisible drift. */
static inline int fh_pica_diffuse_reusable(const FhPicaDiffuseMapState *old,
                                         const FhPicaDiffuseMapState *next){
    static const float allowance[9]={0,1.f/512,1.f/1024,1.f/1536,1.f/2048,
        1.f/2560,1.f/3072,1.f/3584,1.f/4096};
    unsigned n=old->count;if(n>8 || n!=next->count)return 0;
    if(!n)return 1; /* Original disabled channel is white. */
    for(unsigned c=0;c<3;c++)if(old->ambient[c]!=next->ambient[c])return 0;
    for(unsigned i=0;i<n;i++){
        float color=0,delta=0,lower=0;
        for(unsigned c=0;c<3;c++){
            float k=old->color[i][c],a=old->position[i][c],b=next->position[i][c];
            if(k!=next->color[i][c] || !(k>=0 && k<=1) || !isfinite(a) || !isfinite(b))return 0;
            if(k>color)color=k;
            float magnitude=fabsf(a);if(magnitude>lower)lower=magnitude;
            delta+=fabsf(a-b);
        }
        lower-=2.f;
        if(!(lower>0) || !(4.f*delta*color<=allowance[n]*lower))return 0;
    }
    return 1;
}
