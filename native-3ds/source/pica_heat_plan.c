#include "fh_pica_heat_plan.h"
#include <math.h>
#include <string.h>
_Static_assert(sizeof(FhHeatVertex)==12,"Heat vertex fetch extent");
static int16_t packed(float x){return (int16_t)lroundf(fmaxf(-1,fminf(1,x))*32767);}
void fh_pica_heat_mesh(FhHeatVertex*v,uint16_t*indices){
    /* Separable basis: evaluate each axis once, not four transcendentals per
     * vertex. Temporary 1224-byte stack arrays; identical packed output. */
    int16_t rows[FH_HEAT_X+1][2],cols[FH_HEAT_Y+1][2];
    for(unsigned x=0;x<=FH_HEAT_X;x++){
        float row=.0981875f*(64.f*7.f*x/FH_HEAT_X-.5f);
        rows[x][0]=packed(sinf(row));rows[x][1]=packed(cosf(row));
    }
    for(unsigned y=0;y<=FH_HEAT_Y;y++){
        float col=.39275f*(64.f*7.f*y/FH_HEAT_Y-.5f);
        cols[y][0]=packed(sinf(col));cols[y][1]=packed(cosf(col));
    }
    for(unsigned y=0;y<=FH_HEAT_Y;y++)for(unsigned x=0;x<=FH_HEAT_X;x++){
        FhHeatVertex*p=v+y*(FH_HEAT_X+1)+x;
        *p=(FhHeatVertex){.x=x,.y=y,.basis={rows[x][0],rows[x][1],cols[y][0],cols[y][1]}};
    }
    unsigned n=0;
    for(unsigned y=0;y<FH_HEAT_Y;y++)for(unsigned x=0;x<FH_HEAT_X;x++){
        unsigned a=y*(FH_HEAT_X+1)+x,b=a+1,c=a+FH_HEAT_X+1,d=c+1;
        indices[n++]=a;indices[n++]=b;indices[n++]=c;
        indices[n++]=b;indices[n++]=d;indices[n++]=c;
    }
}
void fh_pica_heat_parameters(FhHeatParameters*p,float sx,float sy,int16_t pitch,uint8_t alpha){
    memset(p,0,sizeof(*p));
    if(!isfinite(sx)||!isfinite(sy)||fabsf(sx)>4096||fabsf(sy)>4096)return;
    /* Source coordinates scroll10x and repeat. Keep uniforms small for float24.
     * The analytic field differs slightly from quantized IA8; audit separately. */
    float fa=sx*10.f,fb=sy*10.f;
    float row=.0981875f*64.f*(fa-floorf(fa));
    float col=.39275f*64.f*(-fb-floorf(-fb));
    p->phase[0]=cosf(row);p->phase[1]=sinf(row);
    p->phase[2]=cosf(col);p->phase[3]=sinf(col);
    float cs=cosf(3.142f*fa)*.5f/64.f,sn=sinf(3.142f*fa)*.5f/64.f;
    p->offset_x[0]=cs/320.f;p->offset_x[1]=sn/320.f;
    p->offset_y[0]=-sn/240.f;p->offset_y[1]=cs/240.f;
    uint8_t k=pitch<0?(uint8_t)((((uint16_t)pitch>>8)-0xc0)*4):255;
    unsigned bottom=(alpha*255u)>>8,top=(k*alpha)>>8;
    p->alpha[0]=top/255.f;p->alpha[1]=((float)bottom-top)/255.f;
}
