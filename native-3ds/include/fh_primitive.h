/* Adapted from melee-3ds primitive_expand.h at c2ebe7c769b3287f2b3748173e8fe5ec24ef408e.
 * GPL-3.0; see ../licenses/melee-GPL-3.0.txt.
 * Expansion occurs before the viewport transform, using its EFB pixel extent. */
#ifndef FH_PRIMITIVE_H
#define FH_PRIMITIVE_H
#include "fh_graphics.h"
#include <math.h>
static inline float fh_tex_offset(unsigned mode) {
    const float values[]={0,1.f/16,1.f/8,1.f/4,1.f/2,1};
    return mode<6?values[mode]:0;
}
static inline void fh_primitive_offset(FhVertex *v,unsigned mask,float s,float t) {
    for(unsigned i=0;i<8;i++) if(mask&(1u<<i)) {
        v->uv[i][0]+=s*v->uv[i][2];v->uv[i][1]+=t*v->uv[i][2];
    }
}
static inline int fh_expand_point(const FhVertex *v,unsigned width,unsigned mask,float tex,float vw,float vh,FhVertex q[4]) {
    if(!width || vw<=0 || vh<=0 || v->position[3]<=0 || v->position[2]<-v->position[3] || v->position[2]>0) return 0;
    float dx=width*v->position[3]/(6*vw),dy=width*v->position[3]/(6*vh);
    for(unsigned i=0;i<4;i++) {
        q[i]=*v;q[i].position[0]+=(i&1)?dx:-dx;q[i].position[1]+=(i&2)?-dy:dy;
        fh_primitive_offset(&q[i],mask,(i&1)?tex:0,(i&2)?tex:0);
    }
    return 1;
}
static inline void fh_clip_vertex(FhVertex *out,const FhVertex *a,const FhVertex *b,float t) {
    const float *s=(const float*)a,*e=(const float*)b;float *d=(float*)out;
    for(unsigned i=0;i<sizeof(*out)/sizeof(float);i++) d[i]=s[i]+(e[i]-s[i])*t;
}
static inline int fh_expand_line(const FhVertex *start,const FhVertex *end,unsigned width,unsigned mask,float tex,float vw,float vh,FhVertex q[4]) {
    if(!width || vw<=0 || vh<=0) return 0;
    FhVertex a=*start,b=*end;
    for(unsigned plane=0;plane<3;plane++) {
        float da=plane==0?a.position[3]-1e-6f:plane==1?a.position[2]+a.position[3]:-a.position[2];
        float db=plane==0?b.position[3]-1e-6f:plane==1?b.position[2]+b.position[3]:-b.position[2];
        if(da<0 && db<0) return 0;
        if(da<0 || db<0) { FhVertex p;fh_clip_vertex(&p,&a,&b,da/(da-db));if(da<0) a=p;else b=p; }
    }
    float x=fabsf(b.position[0]/b.position[3]-a.position[0]/a.position[3])*vw;
    float y=fabsf(b.position[1]/b.position[3]-a.position[1]/a.position[3])*vh;
    float dx=y>x?width/(6*vw):0,dy=y>x?0:-(float)width/(6*vh);
    for(unsigned i=0;i<4;i++) {
        q[i]=i<2?a:b;float side=(i&1)?1:-1;
        q[i].position[0]+=side*dx*q[i].position[3];q[i].position[1]+=side*dy*q[i].position[3];
        fh_primitive_offset(&q[i],mask,(i&1)?tex:0,0);
    }
    return 1;
}
#endif
