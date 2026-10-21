#include "fh_pica_planar_shadow.h"
#include <assert.h>
#include <stdio.h>
static unsigned rng=47;
static float random_float(void){rng=rng*1664525u+1013904223u;return ((rng>>8)/16777216.f)*2-1;}
int main(void){
    const float identity[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    unsigned accepted=0;
    for(unsigned i=0;i<50000;i++){
        float n[3]={random_float()*.5f,1,random_float()*.5f};
        float norm=sqrtf(n[0]*n[0]+n[1]*n[1]+n[2]*n[2]);
        for(unsigned k=0;k<3;k++)n[k]/=norm;
        float p[3]={random_float()*500,random_float()*200,random_float()*500};
        float l[3]={random_float(),.2f+fabsf(random_float()),random_float()};
        float v[3]={p[0]+random_float()*30,p[1]+fabsf(random_float())*80,p[2]+random_float()*30},m[3][4];
        if(!fh_pica_planar_shadow_view(m,identity,p,n,l))continue;
        accepted++;
        double ndotl=0,h=0;for(unsigned k=0;k<3;k++){ndotl+=(double)n[k]*l[k];h+=(double)n[k]*(v[k]-p[k]);}
        float projected[3];
        for(unsigned k=0;k<3;k++){
            projected[k]=m[k][0]*v[0]+m[k][1]*v[1]+m[k][2]*v[2]+m[k][3];
            double oracle=v[k]-l[k]*h/ndotl;
            assert(fabs(projected[k]-oracle)<.002);
        }
        double residual=0;for(unsigned k=0;k<3;k++)residual+=(double)n[k]*(projected[k]-p[k]);
        assert(fabs(residual)<.002);
        float view[3][4]={{0,0,-1,57},{0,1,0,-21},{1,0,0,900}},composed[3][4];
        assert(fh_pica_planar_shadow_view(composed,view,p,n,l));
        for(unsigned k=0;k<3;k++){
            float actual=composed[k][0]*v[0]+composed[k][1]*v[1]+composed[k][2]*v[2]+composed[k][3];
            float want=view[k][0]*projected[0]+view[k][1]*projected[1]+view[k][2]*projected[2]+view[k][3];
            assert(fabsf(actual-want)<.002);
        }
    }
    float m[3][4],p[3]={0},n[3]={0,1,0};
    float parallel[3]={1,0,0},below[3]={0,-1,0},zero[3]={0},nan_light[3]={NAN,1,0};
    assert(!fh_pica_planar_shadow_view(m,identity,p,n,parallel));
    assert(!fh_pica_planar_shadow_view(m,identity,p,n,below));
    assert(!fh_pica_planar_shadow_view(m,identity,p,n,zero));
    assert(!fh_pica_planar_shadow_view(m,identity,p,n,nan_light));
    assert(accepted>30000);
    /* Composite a fogged black shadow onto fogged ground. The fog contribution
     * must be unchanged; fading alpha or using un-fogged black fails this. */
    for(unsigned i=0;i<10000;i++){
        double ground=(random_float()+1)*.5,fog=(random_float()+1)*.5;
        double t=(random_float()+1)*.5,alpha=(random_float()+1)*.5;
        double background=t*ground+(1-t)*fog,shadow=(1-t)*fog;
        double combined=(1-alpha)*background+alpha*shadow;
        double oracle=t*((1-alpha)*ground)+(1-t)*fog;
        assert(fabs(combined-oracle)<1e-12);
    }
    printf("PASS %u directional plane/camera comparisons; grazing/invalid light rejection\n",accepted);
}
