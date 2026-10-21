#include "fh_pica_rigid_bounds.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    FhModelCompactVertex v[4]={0};FhPicaRigidBound b;
    for(unsigned i=0;i<4;i++){
        v[i].position[0]=(int)i*256;v[i].position[3]=1;
        v[i].skin[0]=v[i].skin[1]=63;v[i].tail[0]=-.75f;
    }
    fh_pica_rigid_bound_compile(&b,v,4,1,3);assert(b.slot_plus_one==22);
    assert(b.box.lo[0]<=-.75f && b.box.hi[0]>=2.25f);
    for(unsigned which=0;which<7;which++){
        FhModelCompactVertex bad[4];memcpy(bad,v,sizeof(v));
        if(which==0)bad[3].skin[1]=0;
        if(which==1)bad[3].skin[0]=bad[3].skin[1]=3;
        if(which==2)bad[3].skin[0]=bad[3].skin[1]=64;
        if(which==3)bad[3].position[3]=0;
        if(which==4)bad[3].tail[2]=NAN;
        if(which==5)bad[3].tail[2]=INFINITY;
        if(which==6)bad[3].skin[0]=bad[3].skin[1]=255;
        memset(&b,0xa5,sizeof(b));fh_pica_rigid_bound_compile(&b,bad,4,1,3);
        FhPicaRigidBound zero={0};assert(!memcmp(&b,&zero,sizeof(b)));
    }
    FhModelVertex f[2]={0};f[0].position[3]=f[1].position[3]=1;
    fh_pica_rigid_bound_compile(&b,f,2,0,3);assert(b.slot_plus_one==1);
    f[1].skin[0]=f[1].skin[1]=NAN;
    fh_pica_rigid_bound_compile(&b,f,2,0,3);assert(!b.slot_plus_one);
    fh_pica_rigid_bound_compile(&b,NULL,4,1,3);assert(!b.slot_plus_one);
    fh_pica_rigid_bound_compile(&b,v,0,1,3);assert(!b.slot_plus_one);
    fh_pica_rigid_bound_compile(&b,v,4,1,3);
    float pose[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,0}};
    float planes[4][4]={{1,0,0,-100},{-1,0,0,100},{0,1,0,1},{0,-1,0,1}};
    assert(fh_pica_rigid_bound_outside(&b,pose,planes));
    pose[0][3]=NAN;assert(!fh_pica_rigid_bound_outside(&b,pose,planes));
    pose[0][3]=100;assert(!fh_pica_rigid_bound_outside(&b,pose,planes));
    assert(!fh_pica_rigid_bound_outside(NULL,pose,planes));
    puts("PASS rigid eligibility, atomic fail-open bounds, blend/slot/nonfinite rejection, local tail, live pose and boundary margins");
}
