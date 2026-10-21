#ifndef FH_MATERIAL_REGISTER_H
#define FH_MATERIAL_REGISTER_H
#include "fh_material.h"
#include <math.h>
#include <string.h>

/* Pinned Melee separates plain-material colour changes from TEV analysis.
 * Recognize two complete single-stage expressions, then bind live registers.
 * No stage address, game object, or previous-frame value is part of admission.
 * Declined expressions retain the general symbolic compiler unchanged. */
typedef struct { float values[4];int map,coord;unsigned textured; } FhRegisterMaterial;
static int fh_material_register_key(FhRegisterMaterial *key,const FhDrawState *state){
    if(state->stages!=1)return 0;
    const FhTevStage *s=state->tev;
    if(s->color_op || s->alpha_op || s->color_bias || s->alpha_bias ||
       s->color_scale || s->alpha_scale || !s->color_clamp || !s->alpha_clamp)return 0;
    for(unsigned i=0;i<9;i++)if(s->indirect[i])return 0;
    unsigned copy=s->color[0]==15 && s->color[1]==15 && s->color[2]==15 && s->color[3]<8 &&
        s->alpha[0]==7 && s->alpha[1]==7 && s->alpha[2]==7 && s->alpha[3]<4;
    unsigned textured=s->color[0]==15 && s->color[1]==8 && s->color[2]<8 && s->color[3]==15 &&
        s->alpha[0]==7 && s->alpha[1]==4 && s->alpha[2]<4 && s->alpha[3]==7;
    if(!copy && !textured)return 0;
    if(textured){
        if(s->map>=8 || s->coord>=8)return 0;
        for(unsigned c=0;c<4;c++)if(state->swaps[s->tex_swap&3][c]!=c)return 0;
    }
    unsigned rgb=s->color[copy?3:2],alpha=s->alpha[copy?3:2];
    float values[4];unsigned mask=0;
    for(unsigned c=0;c<4;c++){
        float v=c==3?state->tev_colors[alpha][3]:state->tev_colors[rgb/2][rgb&1?3:c];
        if(!isfinite(v) || (textured && !(v>=0 && v<=1)))return 0;
        /* The generic compiler canonicalizes clamped zero, including -0. */
        values[c]=v<=0?0:v>=1?1:v;
        if(textured && values[c]>0)mask|=1u<<c;
    }
    if(mask&7)mask|=7;
    memcpy(key->values,values,sizeof(values));key->textured=mask;
    key->map=mask?(int)s->map:-1;key->coord=mask?(int)s->coord:-1;
    return 1;
}
static void fh_material_register_plan(FhMaterial *out,const FhRegisterMaterial *key){
    memset(out,0,sizeof(*out));out->map=out->coord=-1;
    out->scale[0]=out->scale[1]=out->sparse_colors=1;
    for(unsigned c=0;c<4;c++){
        out->color[c][0]=key->values[c];out->textured[c]=(key->textured>>c)&1;
    }
    out->map=key->map;out->coord=key->coord;
}
static inline int fh_material_register(FhMaterial *out,const FhDrawState *state){
    FhRegisterMaterial key;if(!fh_material_register_key(&key,state))return 0;
    fh_material_register_plan(out,&key);
    return 1;
}
#endif
