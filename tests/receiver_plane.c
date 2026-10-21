#include "fh_pica_receiver_plane.h"
#include "fh_pica_planar_shadow.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned seed=74105;
static float randomf(void){seed=seed*1664525u+1013904223u;return (seed>>8)/8388608.f-1;}
int main(void){
    unsigned cases=0;double max_error=0;
    const float camera[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    for(unsigned i=0;i<30000;i++){
        float n[3]={randomf(),randomf(),randomf()},len=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
        if(len<.1f)continue;
        for(unsigned k=0;k<3;k++)n[k]/=len;
        float distance=.5f+fabsf(randomf())*2000,point[3],view[3][4];
        for(unsigned k=0;k<3;k++)point[k]=-distance*n[k];
        assert(fh_pica_planar_shadow_view(view,camera,point,n,n));
        float p[4][4]={{0,1.8f,randomf()*.5f,0},{-1.3f,0,randomf()*.5f,0},{0,0,-.001001001f,-10.01001f},{0,0,-1,0}};
        float near_z=(i&1)?.2f:0,far_z=(i&1)?.8f:1;
        FhPicaReceiverPlane result;assert(fh_pica_receiver_plane(&result,view,p,near_z,far_z));
        for(unsigned j=0;j<16;j++){
            double x=j<4?((j&1)?1:-1):randomf(),y=j<4?((j&2)?1:-1):randomf();
            double rx=(p[3][2]*y-p[1][2])/p[1][0],ry=(p[3][2]*x-p[0][2])/p[0][1];
            double inverse_z=(n[0]*rx+n[1]*ry+n[2])/(-distance-.25);
            double raw=(p[2][2]+p[2][3]*inverse_z)/p[3][2];
            double expected=(near_z-far_z)*raw+1-far_z;
            double clip=result.equation[0]*x+result.equation[1]*y+result.equation[3];
            assert(clip>=-.90001 && clip<=-.09999);
            double actual=result.depth[0]*clip+result.depth[1],error=fabs(actual-expected);
            if(error>max_error)max_error=error;
            assert(error<3e-6*(1+fabs(expected)));cases++;
        }
    }
    FhPicaReceiverPlane o;float v[3][4]={{0}},p[4][4]={{0}};
    assert(!fh_pica_receiver_plane(&o,v,p,0,1));
    printf("PASS %u receiver quad samples including horizon, rotated viewport, near/far crossings; max depth error %.9g\n",cases,max_error);
}
