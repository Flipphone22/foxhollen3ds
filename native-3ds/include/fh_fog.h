#ifndef FH_FOG_H
#define FH_FOG_H
#include "fh_graphics.h"
#include <math.h>
static inline float fh_fog_transmission(const FhDrawState *s,float depth) {
    unsigned type=s->fog_type&7;if(!type)return 1;
    float amount=0;
    if(s->fog_far!=s->fog_near && s->fog_end!=s->fog_start) {
        float distance;
        if(s->fog_type&8)distance=s->fog_far-depth*(s->fog_far-s->fog_near);
        else {float divisor=depth*(s->fog_far-s->fog_near)+s->fog_near;distance=divisor?s->fog_far*s->fog_near/divisor:s->fog_far;}
        amount=(distance-s->fog_start)/(s->fog_end-s->fog_start);
        amount=fmaxf(0,fminf(1,amount));
    }
    switch(type) {
    case 2:return 1-amount;
    case 4:return exp2f(-8*amount);
    case 5:return exp2f(-8*amount*amount);
    case 6:return 1-exp2f(-8*(1-amount));
    case 7:return 1-exp2f(-8*(1-amount)*(1-amount));
    default:return 1;
    }
}
#endif
