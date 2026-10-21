#include "fh_primitive.h"
#include <assert.h>
#include <stdio.h>
static void near(float a,float b) { assert(fabsf(a-b)<.0005f); }
int main(void) {
    FhVertex a={.position={0,0,-.5f,1},.color={{1,.25f,.5f,1},{.1f,.2f,.3f,.4f}},.uv={{.25f,.5f,2},{.2f,.3f,1}}},b=a,q[4];
    unsigned samples=0;
    for(unsigned width=1;width<=255;width++) for(unsigned depth=1;depth<=1000;depth*=10) for(unsigned half=1;half<=2;half++) {
        a.position[3]=depth;a.position[2]=-(float)depth*.5f;
        float vw=640.f/half,vh=480.f/half;
        assert(fh_expand_point(&a,width,1,.0625f,vw,vh,q));
        near((q[1].position[0]-q[0].position[0])*vw/2/depth,width/6.f);
        near((q[0].position[1]-q[2].position[1])*vh/2/depth,width/6.f);
        near(q[3].uv[0][0],.375f);near(q[3].uv[0][1],.625f);near(q[3].uv[1][0],.2f);
        for(unsigned c=0;c<2;c++) for(unsigned k=0;k<4;k++) near(q[2].color[c][k],a.color[c][k]);samples++;
    }
    a.position[3]=1;a.position[2]=-.5f;b=a;b.position[0]=.5f;
    assert(fh_expand_line(&a,&b,12,1,.125f,640,480,q));
    near(q[0].position[0],0);near(q[2].position[0],.5f);near((q[0].position[1]-q[1].position[1])*240,2);near(q[1].uv[0][0],.5f);
    b.position[0]=0;b.position[1]=.5f;assert(fh_expand_line(&a,&b,18,0,0,640,480,q));
    near((q[1].position[0]-q[0].position[0])*320,3);
    a.position[2]=-2;a.color[1][0]=0;b.color[1][0]=1;b.position[2]=0;
    assert(fh_expand_line(&a,&b,6,0,0,640,480,q));near(q[0].position[2],-1);near(q[0].color[1][0],.5f);
    b.position[2]=-3;assert(!fh_expand_line(&a,&b,6,0,0,640,480,q));assert(!fh_expand_point(&a,6,0,0,640,480,q));
    assert(!fh_expand_point(&b,0,0,0,640,480,q));assert(!fh_expand_line(&a,&b,0,0,0,640,480,q));
    near(fh_tex_offset(5),1);near(fh_tex_offset(1),.0625f);near(fh_tex_offset(9),0);
    printf("Primitive expansion passed %u width/depth/viewport combinations, offsets, clipping and both color channels\n",samples);
}
