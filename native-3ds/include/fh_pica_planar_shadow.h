#ifndef FH_PICA_PLANAR_SHADOW_H
#define FH_PICA_PLANAR_SHADOW_H
#include <math.h>
/* Directional projection onto one live receiver plane. The resident skinned
 * mesh supplies the silhouette; this composes only a camera matrix on CPU.
 * Grazing/back-facing receivers are rejected, never stretched to infinity. */
static inline int fh_pica_planar_shadow_view(float out[3][4],const float view[3][4],
    const float point[3],const float normal[3],const float light[3]){
    float dot=0,length=0,d=0;
    for(unsigned i=0;i<3;i++){dot+=normal[i]*light[i];length+=light[i]*light[i];d+=normal[i]*point[i];}
    if(!isfinite(dot)||!isfinite(length)||!isfinite(d)||length<1e-8f||dot<=.15f*sqrtf(length))return 0;
    float p[3][4];
    for(unsigned r=0;r<3;r++){
        float l=light[r]/dot;
        for(unsigned c=0;c<3;c++)p[r][c]=(r==c?1.f:0.f)-l*normal[c];
        p[r][3]=l*d;
    }
    for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++){
        float v=c==3?view[r][3]:0;
        for(unsigned k=0;k<3;k++)v+=view[r][k]*p[k][c];
        if(!isfinite(v))return 0;
        out[r][c]=v;
    }
    return 1;
}
#endif
