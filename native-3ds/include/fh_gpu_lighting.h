#ifndef FH_GPU_LIGHTING_H
#define FH_GPU_LIGHTING_H
#include "fh_graphics.h"
#include <math.h>
/* Fixed-matrix spot lighting. Owned copies survive batching/command flushes.
 * Uniform rows 44..87; no change to the captured vertex/draw-state ABI. */
typedef struct {
    /* With LIVE_MATERIAL, W lanes hold RGB material. DP3 ignores these lanes;
     * renderer preparation canonicalizes lights[0][3].w as the shader flag. */
    float normal[3][4];
    float lights[8][5][4]; /* position, direction, color, cosine, distance */
    float ambient[4];
    unsigned mask;
} FhGpuLighting;
_Static_assert(offsetof(FhGpuLighting,mask)==44*16 && sizeof(FhGpuLighting)==708,"Lighting uniform layout");
enum { FH_LIGHT_MODEL_NORMAL=1u<<8,FH_LIGHT_LIVE_MATERIAL=1u<<9,FH_LIGHT_NORMAL_PALETTE=1u<<10,FH_LIGHT_FLAT=1u<<11,FH_LIGHT_LIVE_ALPHA=1u<<12,FH_LIGHT_SPHERE=1u<<13 };
enum { FH_LIGHT_DIFFUSE_FILL=1u<<14 };
extern unsigned fh_light_lod_enabled,fh_light_lod_draws,fh_light_lod_source_lights,fh_light_lod_palette_draws;
/* raw_position bit64 identifies the separately bound two-channel program. */
extern unsigned fh_gpu_sphere_disabled,fh_gpu_sphere_draws;
/* Six packed lights, followed by two ambient and two material rows. Alpha
 * uses constant distance attenuation; unsupported coefficients stay on ARM. */
static inline int fh_gpu_sphere_validate(FhGpuLighting *l){
    for(unsigned i=0;i<3;i++)for(unsigned c=0;c<4;c++)
        if(!isfinite(l->normal[i][c]) || fabsf(l->normal[i][c])>16)return 0;
    for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++)
        if(!(l->lights[6][i][c]>=0 && l->lights[6][i][c]<=1))return 0;
    for(unsigned i=0;i<6;i++){
        float largest=1;
        for(unsigned r=0;r<5;r++)for(unsigned c=0;c<(r==2?4:3);c++){
            float value=l->lights[i][r][c];
            if(!isfinite(value) || fabsf(value)>(r==0?1e18f:100.f))return 0;
            if(!r && fabsf(value)>largest)largest=fabsf(value);
        }
        if(i%3==2 && (l->lights[i][4][1]!=0 || l->lights[i][4][2]!=0))return 0;
        l->lights[i][0][3]=1.f/largest;
    }
    l->mask=FH_LIGHT_SPHERE|63;return 1;
}
/* With at most two active lights, rows otherwise occupied by lights 2..7
 * hold all ten independent normal matrices. No uniform or capture ABI grows. */
static inline const float *fh_gpu_normal_row(const FhDrawState *s,const FhGpuLighting *l,unsigned row,unsigned c) {
    if(l->mask&FH_LIGHT_NORMAL_PALETTE)return ((const float*)l->lights+10*4)+(row+c)*4;
    return (l->mask&FH_LIGHT_MODEL_NORMAL)?s->model_palette[row+c]:l->normal[c];
}
const FhGpuLighting *fh_renderer_prepared_lighting(void);
int fh_renderer_prepare_lighting(const FhDrawState *state,const FhGpuLighting *lighting);
static inline void fh_gpu_sphere_resolve(FhVertex *v,const FhDrawState *s,const FhGpuLighting *l){
    unsigned row=(unsigned)v->matrix_index;float eye[3],normal[3];
    for(unsigned c=0;c<3;c++){
        const float *m=s->model_palette[row+c],*n=l->normal[c];
        eye[c]=m[0]*v->position[0]+m[1]*v->position[1]+m[2]*v->position[2]+m[3];
        normal[c]=n[0]*v->uv[7][0]+n[1]*v->uv[7][1]+n[2]*v->uv[7][2]+n[3]*0.f;
    }
    float length=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
    if(length>1e-20f)for(unsigned c=0;c<3;c++)normal[c]/=length;
    for(unsigned channel=0;channel<2;channel++){
        float sum[4];for(unsigned c=0;c<4;c++)sum[c]=l->lights[6][channel][c];
        for(unsigned n=0;n<3;n++){
            const float (*a)[4]=l->lights[channel*3+n];float delta[3];
            for(unsigned c=0;c<3;c++)delta[c]=a[0][c]-eye[c];
            float distance=sqrtf(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
            if(distance>0)for(unsigned c=0;c<3;c++)delta[c]/=distance;
            float facing=normal[0]*delta[0]+normal[1]*delta[1]+normal[2]*delta[2];
            const float *direction=n==2?delta:normal;
            float cosine=direction[0]*a[1][0]+direction[1]*a[1][1]+direction[2]*a[1][2];
            if(cosine<0 || (n!=2 && facing<0))cosine=0;
            float denominator_input=n==2?distance:cosine;
            float numerator=a[3][0]+a[3][1]*cosine+a[3][2]*cosine*cosine;
            float denominator=a[4][0]+a[4][1]*denominator_input+a[4][2]*denominator_input*denominator_input;
            float attenuation=numerator>0 && denominator>0?numerator/denominator:0;
            if(n==2)sum[3]+=a[2][3]*(facing<0?0:facing)*attenuation;
            else for(unsigned c=0;c<3;c++)sum[c]+=a[2][c]*1.f*attenuation;
        }
        for(unsigned c=0;c<4;c++)v->color[channel][c]=l->lights[6][channel+2][c]*(sum[c]<0?0:sum[c]>1?1:sum[c]);
    }
}
/* Complete deferred spot lighting before a material falls back to ARM.
 * Match GX's scalar operation order; retain alpha and the second channel.
 * Negative matrix indices already carry CPU-lit colors and clip positions. */
static inline void fh_gpu_lighting_resolve(FhVertex *v,const FhDrawState *s,const FhGpuLighting *l) {
    if(!(s->raw_position&2) || v->matrix_index<0)return;
    if(l->mask&FH_LIGHT_SPHERE){fh_gpu_sphere_resolve(v,s,l);return;}
    if(l->mask&FH_LIGHT_FLAT){
        for(unsigned c=0;c<3;c++)v->color[0][c]=v->color[0][c]*l->normal[c][3]*l->ambient[c];
        if(l->mask&FH_LIGHT_LIVE_ALPHA)v->color[0][3]*=l->ambient[3];
        return;
    }
    unsigned row=(unsigned)v->matrix_index;
    float eye[3],normal[3],light[3];
    for(unsigned c=0;c<3;c++) {
        const float *m=s->model_palette[row+c],*n=fh_gpu_normal_row(s,l,row,c);
        eye[c]=m[0]*v->position[0]+m[1]*v->position[1]+m[2]*v->position[2]+m[3];
        normal[c]=n[0]*v->uv[7][0]+n[1]*v->uv[7][1]+n[2]*v->uv[7][2]+n[3]*0.f;
        light[c]=l->ambient[c];
    }
    float length=sqrtf(normal[0]*normal[0]+normal[1]*normal[1]+normal[2]*normal[2]);
    if(length>1e-20f)for(unsigned c=0;c<3;c++)normal[c]/=length;
    if(l->mask&FH_LIGHT_DIFFUSE_FILL)for(unsigned c=0;c<3;c++){
        float fill=l->lights[1][c][3];
        for(unsigned axis=0;axis<3;axis++)fill+=l->normal[c][axis]*normal[axis];
        light[c]+=fill>0?fill:0;
    }
    for(unsigned i=0;i<8;i++)if(l->mask&(1u<<i)) {
        const float (*a)[4]=l->lights[i];float delta[3];
        for(unsigned c=0;c<3;c++)delta[c]=a[0][c]-eye[c];
        float distance=sqrtf(delta[0]*delta[0]+delta[1]*delta[1]+delta[2]*delta[2]);
        if(distance>0)for(unsigned c=0;c<3;c++)delta[c]/=distance;
        float diffuse=normal[0]*delta[0]+normal[1]*delta[1]+normal[2]*delta[2];
        if(diffuse<0)diffuse=0;
        float cosine=delta[0]*a[1][0]+delta[1]*a[1][1]+delta[2]*a[1][2];
        if(cosine<0)cosine=0;
        float numerator=a[3][0]+a[3][1]*cosine+a[3][2]*cosine*cosine;
        float denominator=a[4][0]+a[4][1]*distance+a[4][2]*distance*distance;
        float attenuation=numerator>0 && denominator>0?numerator/denominator:0;
        for(unsigned c=0;c<3;c++)light[c]+=a[2][c]*diffuse*attenuation;
    }
    for(unsigned c=0;c<3;c++) {
        if(l->mask&FH_LIGHT_LIVE_MATERIAL)v->color[0][c]*=l->normal[c][3];
        v->color[0][c]*=light[c]<0?0:light[c]>1?1:light[c];
    }
}
#endif
