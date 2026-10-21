#include "fh_pica_shadow_receiver_depth.h"
#include "fh_pica_planar_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=1789;
static float rnd(void){seed=seed*1664525u+1013904223u;return ((seed>>8)/16777216.f)*2-1;}
int main(void){
    unsigned cases=0;
    const float projection[4][4]={{1.3f,0,0,0},{0,1.8f,0,0},{0,0,-.001001001f,-10.01001f},{0,0,-1,0}};
    const float camera[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    for(unsigned i=0;i<80000;i++){
        float n[3]={rnd()*.7f,rnd()*.7f,1};
        float length=sqrtf(n[0]*n[0]+n[1]*n[1]+1);for(unsigned k=0;k<3;k++)n[k]/=length;
        float distance=5+fabsf(rnd())*2000,point[3],light[3]={0,0,1},view[3][4],depth[2];
        for(unsigned k=0;k<3;k++)point[k]=-n[k]*distance;
        assert(fh_pica_planar_shadow_view(view,camera,point,n,light));
        assert(fh_pica_shadow_receiver_depth(depth,view,projection,0,1));
        float source[3]={point[0]+rnd()*50,point[1]+rnd()*50,point[2]+rnd()*50};
        double p[3];for(unsigned k=0;k<3;k++)p[k]=view[k][0]*source[0]+view[k][1]*source[1]+view[k][2]*source[2]+view[k][3];
        if(p[2]>-11)continue;
        double z=-(projection[2][2]+projection[2][3]/p[2]);
        double expected_z=-(projection[2][2]+projection[2][3]/(p[2]*(distance+.25)/distance));
        double got=depth[0]*z+depth[1];
        assert(fabs(got+expected_z)<2e-6);
        assert(got < -z);
        /* A real receiver 0.1 normal units away belongs to the band; a floor
         * two units beyond the edge does not. Background depth zero must not
         * enter via the lower-depth clamp; the prepass comparison is strict. */
        double floor=projection[2][2]+projection[2][3]/(p[2]*(distance+.1)/distance);
        double lower=projection[2][2]+projection[2][3]/(p[2]*(distance+2)/distance);
        assert(got<floor && -z>=floor);assert(!(got<lower && -z>=lower));
        cases++;
    }
    float out[2],zero[3][4]={{0}},view[3][4]={{1,0,0,0},{0,1,0,0},{0,0,0,-50}};
    assert(!fh_pica_shadow_receiver_depth(out,zero,projection,0,1));
    assert(!fh_pica_shadow_receiver_depth(out,view,projection,1,0));
    float invalid[4][4];memcpy(invalid,projection,sizeof invalid);invalid[3][3]=1;
    assert(!fh_pica_shadow_receiver_depth(out,view,invalid,0,1));
    printf("PASS %u random plane/perspective receiver bands, physical-distance oracle and ledge exclusions\n",cases);
}
