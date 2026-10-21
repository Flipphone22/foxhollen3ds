#ifndef FH_FUR_GPU_H
#define FH_FUR_GPU_H
#include "fh_fragment.h"
#include <math.h>
#include <string.h>
/* The observed SFA family uses four equal multiples of 1/32, exponent -1.
 * Restrict the specialization to that exact matrix family; arbitrary GX
 * indirect matrices retain the ordinary coordinate preparation. */
static inline unsigned fh_fur_gpu_zero_second(const FhDrawState *s){
    if(s->tev[3].channel!=255 || s->indirect_exponent[1]!=-15)return 0;
    for(unsigned i=0;i<6;i++)if(((const float*)s->indirect_matrix[1])[i]!=0)return 0;
    return 1;
}
static inline int fh_fur_gpu_strength(const FhDrawState *s,float *strength){
    unsigned zero_second=fh_fur_gpu_zero_second(s);
    if((s->raw_position!=1 && s->raw_position!=3 && s->raw_position!=7) || s->indirect_exponent[0]!=-1 || (!zero_second && s->indirect_exponent[1]!=-1))return 0;
    if(s->tev[3].channel!=255 && (s->indirect_scale[1][0] || s->indirect_scale[1][1] || s->textures[0].wrap_s!=s->textures[2].wrap_s ||
       s->textures[0].wrap_t!=s->textures[2].wrap_t || s->textures[0].mag_filter!=s->textures[2].mag_filter))return 0;
    float f=s->indirect_matrix[0][0][0],n=f*32;
    if(!(n>=0 && n<=16) || n!=(float)(unsigned)n)return 0;
    const unsigned active[4]={0,4,7,11};
    for(unsigned i=0;i<12;i++){
        unsigned live=0;for(unsigned j=0;j<(zero_second?2:4);j++)live|=i==active[j];
        if(((const float*)s->indirect_matrix)[i]!=(live?f:0))return 0;
    }
    unsigned w=s->textures[3].width,h=s->textures[3].height;
    if(!w || !h || w>1024 || h>1024 || (w&(w-1)) || (h&(h-1)))return 0;
    *strength=f;return 1;
}
static inline int fh_fur_gpu_coordinates_valid(const FhVertex *v,unsigned count,const FhDrawState *s){
    if(s->tev[3].channel!=255 && (s->indirect_scale[1][0] || s->indirect_scale[1][1] || s->textures[0].wrap_s!=s->textures[2].wrap_s ||
       s->textures[0].wrap_t!=s->textures[2].wrap_t || s->textures[0].mag_filter!=s->textures[2].mag_filter))return 0;
    for(unsigned i=0;i<count;i++){
        if(((s->raw_position&4) && !v[i].uv[6][0]) || v[i].matrix_index<0 || (s->tev[3].channel!=255 && memcmp(v[i].uv[2],v[i].uv[3],12)))return 0;
        for(unsigned c=0;c<5;c++){
            if(!(fabsf(v[i].uv[c][2])>=1e-10f))return 0;
            for(unsigned j=0;j<3;j++)if(!isfinite(v[i].uv[c][j]) || fabsf(v[i].uv[c][j])>4096)return 0;
        }
    }
    return 1;
}
static inline int fh_fur_gpu_coefficients(float out[][3],const FhVertex *v,unsigned count,const FhDrawState *s){
    if(!fh_fur_gpu_coordinates_valid(v,count,s))return 0;
    unsigned zero_second=fh_fur_gpu_zero_second(s);
    for(unsigned i=0;i<count;i++){
        float samples[2][3];fh_fur_samples(samples,s,v+i);
        out[i][0]=(zero_second?samples[0][0]-128:samples[0][0]+samples[1][1]-256)*.5f/s->textures[3].width;
        out[i][1]=(zero_second?samples[0][1]-128:samples[0][1]+samples[1][2]-256)*.5f/s->textures[3].height;
        out[i][2]=0;
    }
    return 1;
}
int fh_fur_gpu_coefficients_cached(float out[][3],const FhVertex *v,unsigned count,const FhDrawState *s);
#endif
