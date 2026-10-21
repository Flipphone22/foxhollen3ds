#ifndef FH_PICA_FOG_H
#define FH_PICA_FOG_H
#include <math.h>
/* Game exponential distance fog, before height-dependent heavy-fog stages. */
static inline float fh_pica_fog_transmission(float distance,float start,float end){
    if(end==start)return 1;
    float amount=fmaxf(0,fminf(1,(distance-start)/(end-start)));
    return exp2f(-8*amount);
}
/* Clamped affine curves differ most at one of their four breakpoints.
 * exp2(-8*x) has slope magnitude <=8*ln(2). A cached curve within one
 * procedural-LUT unit (1/4095) is below the input table's precision. */
static inline int fh_pica_fog_same_curve(float start,float end,float old_start,float old_end){
    if(!(end>start) || !(old_end>old_start))return 0;
    const float point[4]={start,end,old_start,old_end};float bound=0;
    for(unsigned i=0;i<4;i++){
        float a=fmaxf(0,fminf(1,(point[i]-start)/(end-start)));
        float b=fmaxf(0,fminf(1,(point[i]-old_start)/(old_end-old_start)));
        bound=fmaxf(bound,fabsf(a-b));
    }
    return bound*(8*.693147181f)+1e-7f <= 1.f/4095;
}
void fh_pica_fog_init(int uniform);
void fh_pica_fog_range(float start,float end,float near_z,float far_z);
void fh_pica_fog_color(unsigned r,unsigned g,unsigned b);
void fh_pica_fog_invalidate(void);
void fh_pica_fog_bind(unsigned enabled);
/* Call bind first. This scale is unused when the fog stage is disabled. */
float fh_pica_fog_distance_scale(void);
void fh_pica_fog_report(void);
#endif
