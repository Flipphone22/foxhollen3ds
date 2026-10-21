#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fh_minimal_graphics.h"
typedef uint8_t u8;
typedef struct {int16_t x,y,z,pad6,u,v;u8 padc[3],a;} WaterVtx;
unsigned fh_minimal_rejected;
float gCameraModelViewMatrix[3][4]={{1,0,0,17},{0,1,0,23},{0,0,1,29}};
static FhMinimalEffectVertex captured[6];
static unsigned calls;
static void GXGetScissor(unsigned *x,unsigned *y,unsigned *w,unsigned *h){*x=3;*y=5;*w=600;*h=420;}
void fh_minimal_water_effect_draw(const FhMinimalEffectVertex v[6],const float view[3][4],const unsigned scissor[4]){
    assert(view==gCameraModelViewMatrix);
    assert(scissor[0]==3 && scissor[1]==5 && scissor[2]==600 && scissor[3]==420);
    memcpy(captured,v,sizeof(captured));calls++;
}
#include "waterfx_hook.inc"
int main(void){
    WaterVtx v[4]={{-300,0,300,0,0,0,{0},7},{-300,0,-300,0,0,127,{0},9},
                  {300,0,-300,0,127,127,{0},11},{300,0,300,0,127,0,{0},13}};
    const unsigned order[6]={3,1,0,3,2,1};
    u8 desc[32]={0,3,1,0};desc[17]=3;desc[18]=2;desc[19]=1;
    for(unsigned alpha=0;alpha<=255;alpha+=51){
        fh_waterfx_triangles(v,desc,alpha);
        for(unsigned i=0;i<6;i++){
            WaterVtx *s=v+order[i];FhMinimalEffectVertex *d=captured+i;
            assert(d->position[0]==s->x && d->position[1]==s->y && d->position[2]==s->z && d->position[3]==1);
            assert(d->uv[0]==s->u/128.f && d->uv[1]==s->v/128.f);
            /* Original A1 lifetime alpha, not vertex color alpha or tint alpha. */
            assert(fabsf(d->color[3]-alpha/255.f)<1e-6f);
        }
    }
    assert(calls==6 && fh_minimal_rejected==0);
    desc[18]=4;fh_waterfx_triangles(v,desc,255);
    assert(calls==6 && fh_minimal_rejected==1);
    puts("36 indexed vertices/lifetime checks; malformed index rejected; original view/scissor retained");
}
