#ifndef FH_PICA_SHADOW_RECEIVER_DEPTH_H
#define FH_PICA_SHADOW_RECEIVER_DEPTH_H
#include <math.h>
/* A projected caster lies on one plane. Moving that plane away from the eye
 * by a fixed normal distance scales every ray intersection by the same ratio.
 * For perspective Z, z' = ratio*z + (1-ratio)*z_infinity. Apply this through
 * DepthMap, retaining IDENTICAL clip coordinates, coverage and skinning.
 * No CPU vertices, depth readback or constant depth epsilon (which grows into
 * a large world-space gap at distance). This is only a single-plane receiver
 * band, not multi-receiver or self-shadow reconstruction. */
static inline int fh_pica_shadow_receiver_depth(float out[2],
    const float view[3][4],const float projection[4][4],float near_z,float far_z){
    if(projection[2][0]!=0 || projection[2][1]!=0 ||
       projection[3][0]!=0 || projection[3][1]!=0 ||
       projection[3][3]!=0 || !(fabsf(projection[3][2])>1e-8f) ||
       !(far_z>near_z))return 0;
    float best=0,normal[3]={0,0,0};
    for(unsigned a=0;a<3;a++){
        unsigned b=(a+1)%3;
        float n[3]={view[1][a]*view[2][b]-view[2][a]*view[1][b],
                    view[2][a]*view[0][b]-view[0][a]*view[2][b],
                    view[0][a]*view[1][b]-view[1][a]*view[0][b]};
        float length=n[0]*n[0]+n[1]*n[1]+n[2]*n[2];
        if(length>best){best=length;for(unsigned k=0;k<3;k++)normal[k]=n[k];}
    }
    if(!isfinite(best)||best<1e-12f)return 0;
    float d=fabsf(normal[0]*view[0][3]+normal[1]*view[1][3]+normal[2]*view[2][3])/sqrtf(best);
    if(!isfinite(d)||d<.001f)return 0;
    /* Existing projected surface is raised 0.1 units. A 0.25-unit band
     * contains that receiver but excludes lower floors across real ledges. */
    float ratio=d/(d+.25f),infinity=projection[2][2]/projection[3][2];
    float scale=near_z-far_z,offset=1.f-far_z;
    out[0]=scale*ratio;out[1]=offset+scale*(1.f-ratio)*infinity;
    return isfinite(out[0])&&isfinite(out[1]);
}
#endif
