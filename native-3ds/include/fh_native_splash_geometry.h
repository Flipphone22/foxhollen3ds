#ifndef FH_NATIVE_SPLASH_GEOMETRY_H
#define FH_NATIVE_SPLASH_GEOMETRY_H
#include "fh_minimal_graphics.h"
/* Exact expansion of the engine19 fifteen 16-vertex triangle strips. Original
 * band transforms already contain the camera view. Winding is preserved so
 * front/back cull passes retain their original transparent ordering. */
static inline void fh_native_splash_geometry(FhMinimalEffectVertex out[630],
    const float positions[16][3],const float uv[128][2],const unsigned char colors[32],
    const float bands[8][3][4]) {
    FhMinimalEffectVertex unique[128];
    for(unsigned m=0;m<8;m++)for(unsigned k=0;k<16;k++) {
        FhMinimalEffectVertex *v=unique+m*16+k;
        for(unsigned r=0;r<3;r++)v->position[r]=bands[m][r][0]*positions[k][0]+
            bands[m][r][1]*positions[k][1]+bands[m][r][2]*positions[k][2]+bands[m][r][3];
        v->position[3]=1;
        v->color[0]=v->color[1]=v->color[2]=1;v->color[3]=colors[m*4+3]/255.f;
        v->uv[0]=uv[m*16+k][0];v->uv[1]=uv[m*16+k][1];
    }
    unsigned next=0;
    for(unsigned k=0;k<15;k++)for(unsigned t=0;t<14;t++)for(unsigned j=0;j<3;j++) {
        unsigned n=t+((t&1)&&j<2?1-j:j);
        unsigned m=7-n/2;
        out[next++]=unique[m*16+k+(n&1)];
    }
}
#endif
