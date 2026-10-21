#ifndef FH_GPU_TEXGEN_H
#define FH_GPU_TEXGEN_H
#include "fh_graphics.h"
#include "fh_gpu_lighting.h"
/* Normal-derived coordinate zero uses the same rigid palette as positions.
 * The post matrix remains live; stored geometry retains untransformed normals.
 * uv[6][0] marks individual eligible vertices. Mixed CPU fallbacks keep UV0. */
int fh_renderer_prepare_texgen(const FhDrawState *state,const float post[3][4]);
int fh_renderer_texgens_unused(const FhDrawState *state);
unsigned fh_renderer_texgen_inputs(const FhDrawState *state);
int fh_renderer_prepare_uvgen(const FhDrawState *state,const float matrix[3][4]);
const float (*fh_renderer_prepared_texgen(void))[4];
static inline void fh_gpu_texgen_resolve(FhVertex *v,const FhDrawState *s,const float post[3][4]) {
    if(!(s->raw_position&4) || !v->uv[6][0] || v->matrix_index<0)return;
    unsigned row=(unsigned)v->matrix_index;float source[3];
    if(s->raw_position&16) {
        source[0]=v->uv[0][0];source[1]=v->uv[0][1];source[2]=v->uv[0][2];
    } else {
    for(unsigned r=0;r<2;r++) {
        const FhGpuLighting *l=fh_renderer_prepared_lighting();
        const float *m=(l->mask&FH_LIGHT_NORMAL_PALETTE)?fh_gpu_normal_row(s,l,row,r):s->model_palette[row+r],*n=v->uv[7];
        source[r]=m[0]*n[0]+m[1]*n[1]+m[2]*n[2];
    }
    source[2]=1;
    }
    for(unsigned r=0;r<3;r++)v->uv[0][r]=post[r][0]*source[0]+post[r][1]*source[1]+post[r][2]*source[2]+post[r][3];
    v->uv[6][0]=0;
}
#endif
