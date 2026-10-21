#ifndef FH_NATIVE_CONSTANT_LIGHT_H
#define FH_NATIVE_CONSTANT_LIGHT_H
#include <math.h>
#include "fh_native_lighting.h"
/* Classify the PACKED, live light channels, after attenuation constants and
 * channel masks. A channel is position/normal independent iff every active
 * color is zero or its diffuse mode is NONE, and attenuation is constant.
 * Ambient/material vertex sources remain evaluated per vertex in the shader.
 * Accumulate in the original light order; clamp only after ambient addition. */
static inline int fh_native_constant_light(const FhNativeLighting *p,float fill[4]){
    if(p->complex || p->lights>8)return 0;
    float sum[4]={0};
    for(unsigned i=0;i<p->lights;i++)for(unsigned c=0;c<4;c++){
        float value=p->uniform[8+i][c];
        if(!isfinite(value) || (value!=0 && p->uniform[18][c==3?2:0]!=1))return 0;
        sum[c]+=value;
    }
    for(unsigned c=0;c<4;c++)if(!isfinite(sum[c]))return 0;
    for(unsigned c=0;c<4;c++)fill[c]=sum[c];
    return 1;
}
#endif
