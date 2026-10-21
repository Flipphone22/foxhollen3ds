#ifndef FH_LIGHT_LOD_H
#define FH_LIGHT_LOD_H
#include "fh_gpu_lighting.h"
#include <string.h>
/* Visual-only first-order diffuse approximation. Keep two spatial lights;
 * all other active lights contribute live RGB directional fill at the model
 * center. No game light/register is edited. Caller must supply GPU normals,
 * freeing normal.xyz for the nine fill coefficients within the existing ABI. */
static inline int fh_light_lod_reduce(FhGpuLighting *data,const float center[3]){
    unsigned mask=data->mask&255;
    if(__builtin_popcount(mask)<=2 || (data->mask&(FH_LIGHT_SPHERE|FH_LIGHT_FLAT|FH_LIGHT_DIFFUSE_FILL)))return 0;
    for(unsigned c=0;c<3;c++)if(!isfinite(center[c]))return 0;
    float energy[8][3]={{0}},direction[8][3]={{0}},score[8]={0};
    unsigned first=8,second=8;
    for(unsigned i=0;i<8;i++)if(mask&(1u<<i)){
        const float (*l)[4]=data->lights[i];float distance2=0;
        for(unsigned c=0;c<3;c++){
            direction[i][c]=l[0][c]-center[c];distance2+=direction[i][c]*direction[i][c];
            if(!isfinite(l[2][c]) || l[2][c]<0 || l[2][c]>1)return 0;
        }
        if(!isfinite(distance2))return 0;
        float distance=sqrtf(distance2),cosine=0;
        if(distance>0)for(unsigned c=0;c<3;c++)direction[i][c]/=distance;
        for(unsigned c=0;c<3;c++)cosine+=direction[i][c]*l[1][c];
        if(cosine<0)cosine=0;
        float numerator=l[3][0]+l[3][1]*cosine+l[3][2]*cosine*cosine;
        float denominator=l[4][0]+l[4][1]*distance+l[4][2]*distance*distance;
        if(!isfinite(numerator) || !isfinite(denominator))return 0;
        float attenuation=numerator>0 && denominator>0?numerator/denominator:0;
        if(!isfinite(attenuation) || attenuation>16)return 0;
        for(unsigned c=0;c<3;c++){
            energy[i][c]=l[2][c]*attenuation;
            if(energy[i][c]>score[i])score[i]=energy[i][c];
        }
        if(first==8 || score[i]>score[first]){second=first;first=i;}
        else if(second==8 || score[i]>score[second])second=i;
    }
    if(second==8)return 0;
    float retained[2][5][4];memcpy(retained[0],data->lights[first],sizeof(retained[0]));memcpy(retained[1],data->lights[second],sizeof(retained[1]));
    float dc[3]={0},coeff[3][3]={{0}};
    for(unsigned i=0;i<8;i++)if((mask&(1u<<i)) && i!=first && i!=second){
        for(unsigned c=0;c<3;c++){
            dc[c]+=.25f*energy[i][c];
            for(unsigned axis=0;axis<3;axis++)coeff[c][axis]+=.5f*energy[i][c]*direction[i][axis];
        }
    }
    for(unsigned c=0;c<3;c++){
        if(dc[c]>16)return 0;
        for(unsigned axis=0;axis<3;axis++)if(fabsf(coeff[c][axis])>16)return 0;
    }
    memcpy(data->lights,retained,sizeof(retained));
    for(unsigned c=0;c<3;c++){
        memcpy(data->normal[c],coeff[c],12);data->lights[1][c][3]=dc[c];
    }
    data->mask=(data->mask&~255u)|3|FH_LIGHT_DIFFUSE_FILL;return 1;
}
#endif
