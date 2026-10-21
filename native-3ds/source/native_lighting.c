#include "fh_native_lighting.h"
#include <math.h>
#include <string.h>
/* GX enum values: diffuse NONE/SIGN/CLAMP=0/1/2, attenuation
 * SPEC/SPOT/NONE=0/1/2. Adapted from pinned Melee vertex lighting. */
static int constant(const FhLight *l,unsigned attenuation,float *value){
    if(attenuation==2){*value=1;return 1;}
    if(l->a[1] || l->a[2] || l->k[1] || l->k[2])return 0;
    *value=l->k[0]>0?fmaxf(l->a[0],0)/l->k[0]:0;return 1;
}
int fh_native_lighting_pack(FhNativeLighting *out,const FhLight lights[8],
    const FhChannel channels[4],const float ambient[4],const float material[4]){
    FhNativeLighting p={0};const FhChannel *rgb=channels,*alpha=channels+2;
    unsigned mask=(rgb->enabled?rgb->mask:0)|(alpha->enabled?alpha->mask:0);
    unsigned complex[8]={0},order[8],n=0;float factor[8][2]={{0}};
    for(unsigned i=0;i<8;i++)if(mask&(1u<<i)){
        const FhLight *l=lights+i;
        for(unsigned j=0;j<3;j++)if(!isfinite(l->a[j]) || !isfinite(l->k[j]) ||
            !isfinite(l->position[j]) || !isfinite(l->direction[j]))return 0;
        for(unsigned j=0;j<4;j++)if(!isfinite(l->color[j]))return 0;
        unsigned ra=rgb->enabled && (rgb->mask&(1u<<i));
        unsigned aa=alpha->enabled && (alpha->mask&(1u<<i));
        if(ra && !constant(l,rgb->attenuation,&factor[i][0]))complex[i]=1;
        if(aa && !constant(l,alpha->attenuation,&factor[i][1]))complex[i]=1;
        /* Separate alpha attenuation requires a second evaluation; explicitly
         * reject until that material family is implemented. Never drop a light. */
        if(complex[i] && ra && aa && rgb->attenuation!=alpha->attenuation)return 0;
        p.complex+=complex[i];
    }
    if(p.complex>3)return 0;
    for(unsigned pass=0;pass<2;pass++)for(unsigned i=0;i<8;i++)
        if((mask&(1u<<i)) && complex[i]==!pass)order[n++]=i;
    p.lights=n;p.booleans=1; /* bit0 lighting, bits1..8 active */
    for(unsigned j=0;j<n;j++){
        unsigned i=order[j];const FhLight *l=lights+i;
        unsigned ra=rgb->enabled && (rgb->mask&(1u<<i));
        unsigned aa=alpha->enabled && (alpha->mask&(1u<<i));
        p.booleans|=1u<<(1+j);
        memcpy(p.uniform[j],l->position,12);
        if(complex[i]){
            unsigned attenuation=ra?rgb->attenuation:alpha->attenuation;
            p.uniform[j][3]=attenuation==0?2:1;
            float *v=j==0?p.uniform[19]:p.fixed[j*2-2];
            memcpy(v,l->direction,12);v[3]=l->k[0];
            v=j==0?p.uniform[20]:p.fixed[j*2-1];memcpy(v,l->a,12);v[3]=l->k[1];
            p.fixed[4][j]=l->k[2];
            factor[i][0]=factor[i][1]=1;
        }
        for(unsigned c=0;c<4;c++)p.uniform[8+j][c]=l->color[c]*(c==3?(aa?factor[i][1]:0):(ra?factor[i][0]:0));
    }
    for(unsigned c=0;c<4;c++){
        const FhChannel *ch=c==3?alpha:rgb;
        p.uniform[16][c]=ch->enabled?ambient[c]:1;
        p.uniform[17][c]=material[c];
    }
    p.uniform[18][0]=rgb->diffuse==0;p.uniform[18][1]=rgb->diffuse==2;
    p.uniform[18][2]=alpha->diffuse==0;p.uniform[18][3]=alpha->diffuse==2;
    p.booleans|=(!!(rgb->enabled && rgb->ambient_source)<<9)|
        (!!(alpha->enabled && alpha->ambient_source)<<10)|
        (!!rgb->material_source<<11)|(!!alpha->material_source<<12);
    *out=p;return 1;
}
