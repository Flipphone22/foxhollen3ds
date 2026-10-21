#include "fh_material.h"
#include <string.h>
static void fur_fixture(FhDrawState *s) {
    const unsigned colors[4][4]={{15,15,15,8},{15,8,6,10},{15,0,4,15},{8,14,0,0}};
    const unsigned alpha[4][4]={{7,7,7,7},{7,7,7,3},{7,7,7,0},{7,4,5,0}};
    const unsigned maps[]={0,1,255,3},coords[]={2,0,255,4};
    const unsigned indirect[4][9]={{0},{0},{0,0,7,1,6,6,0,0,0},{1,0,7,2,0,0,1,0,1}};
    s->stages=4;s->indirect_count=2;
    s->indirect_order[0][0]=1;s->indirect_order[0][1]=4;
    s->indirect_order[1][0]=3;s->indirect_order[1][1]=2;
    for(unsigned i=0;i<4;i++) {
        s->tev[i]=(FhTevStage){.map=maps[i],.coord=coords[i],.color_dest=i==1?2:0,.channel=i==3?8:4,.color_op=i==3,.color_bias=i==3,.color_clamp=1,.alpha_clamp=i==3,.kcolor=4};
        memcpy(s->tev[i].color,colors[i],16);memcpy(s->tev[i].alpha,alpha[i],16);memcpy(s->tev[i].indirect,indirect[i],36);
        for(unsigned c=0;c<4;c++)s->swaps[i][c]=c;
    }
}
