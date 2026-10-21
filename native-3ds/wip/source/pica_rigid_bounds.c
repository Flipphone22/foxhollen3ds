#include "fh_pica_rigid_bounds.h"
#include <string.h>
void fh_pica_rigid_bound_compile(FhPicaRigidBound*out,const void*vertices,unsigned n,unsigned compact,unsigned format){
    memset(out,0,sizeof(*out));
    if(!vertices || !n || n>FH_MODEL_VERTICES || (format!=3 && format!=5 && format!=6 && format!=7))return;
    FhPicaRigidBound bound={0};float scale=format==7?1.f:format==5?.125f:1.f/256;
    for(unsigned i=0;i<n;i++){
        unsigned slot;float p[3],magnitude[3];
        if(compact){
            const FhModelCompactVertex*v=(const FhModelCompactVertex*)vertices+i;
            if(v->skin[0]!=v->skin[1] || v->skin[0]%3 || v->skin[0]>=FH_MODEL_BONES*3 || v->position[3]!=1)return;
            slot=v->skin[0]/3;
            for(unsigned k=0;k<3;k++){
                float x=v->position[k]*scale;p[k]=x+v->tail[k];magnitude[k]=fabsf(x)+fabsf(v->tail[k])+1;
            }
        }else{
            const FhModelVertex*v=(const FhModelVertex*)vertices+i;
            float row=v->skin[0];
            if(!isfinite(row) || row<0 || row>=FH_MODEL_BONES*3 || row!=(unsigned)row || (unsigned)row%3 || row!=v->skin[1] || v->position[3]!=1)return;
            slot=(unsigned)row/3;
            for(unsigned k=0;k<3;k++){p[k]=v->position[k];magnitude[k]=fabsf(p[k])+1;}
        }
        if(i && bound.slot_plus_one!=slot+1)return;
        bound.slot_plus_one=slot+1;
        for(unsigned k=0;k<3;k++){
            float margin=1e-4f*magnitude[k];
            if(!isfinite(p[k]) || !isfinite(margin))return;
            if(!i || p[k]-margin<bound.box.lo[k])bound.box.lo[k]=p[k]-margin;
            if(!i || p[k]+margin>bound.box.hi[k])bound.box.hi[k]=p[k]+margin;
        }
    }
    *out=bound;
}
int fh_pica_rigid_bound_outside(const FhPicaRigidBound*b,const float pose[3][4],const float planes[4][4]){
    if(!b || !b->slot_plus_one)return 0;
    /* Most admitted objects are visible. If the box centre is inside every
     * side plane, the box cannot be wholly outside any one of them. This only
     * keeps draws; rejection still requires the conservative full-box test. */
    float center[3],world[3];unsigned inside=1;
    for(unsigned k=0;k<3;k++)center[k]=(b->box.lo[k]+b->box.hi[k])*.5f;
    for(unsigned r=0;r<3;r++){
        world[r]=pose[r][3];
        for(unsigned k=0;k<3;k++)world[r]+=pose[r][k]*center[k];
    }
    for(unsigned j=0;j<4;j++){
        float d=planes[j][3];for(unsigned k=0;k<3;k++)d+=planes[j][k]*world[k];
        inside&=isfinite(d) && d>=0;
    }
    if(inside)return 0;
    for(unsigned j=0;j<4;j++){
        float local[4],magnitude[4];
        for(unsigned k=0;k<4;k++){
            local[k]=k==3?planes[j][3]:0;magnitude[k]=fabsf(local[k]);
            for(unsigned r=0;r<3;r++){
                float term=planes[j][r]*pose[r][k];local[k]+=term;magnitude[k]+=fabsf(term);
            }
        }
        float maximum=local[3],error=magnitude[3]+1;
        for(unsigned k=0;k<3;k++){
            maximum+=local[k]*(local[k]>=0?b->box.hi[k]:b->box.lo[k]);
            error+=magnitude[k]*fmaxf(fabsf(b->box.lo[k]),fabsf(b->box.hi[k]));
        }
        if(isfinite(maximum) && isfinite(error) && maximum<-.002f*error)return 1;
    }
    return 0;
}
