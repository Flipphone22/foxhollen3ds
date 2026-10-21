#ifndef FH_PICA_WATER_BOUNDS_H
#define FH_PICA_WATER_BOUNDS_H
#include <math.h>
/* Adapt pinned Melee's conservative homogeneous side-plane rejection, as
 * used by our world renderer. Keep eye/clip intervals separate to preserve
 * uncertainty through cancellation; never divide by W or test depth/LOD.
 * The grid only displaces texture coordinates, so its original quad bounds
 * enclose the complete submitted geometry. No vertex walk or GPU state. */
typedef struct {float lo[3],hi[3];} FhWaterBounds;
static inline void fh_water_bounds_prepare(FhWaterBounds *b,const float origin[3],const float extent[3]){
    for(unsigned k=0;k<3;k++){
        float a=origin[k],z=a+extent[k];
        float margin=.002f*(fabsf(a)+fabsf(extent[k])+1.f);
        b->lo[k]=(a<z?a:z)-margin;b->hi[k]=(a>z?a:z)+margin;
    }
}
static inline int fh_water_bounds_outside(const FhWaterBounds *b,const float view[3][4],const float projection[4][4]){
    float eye_lo[3],eye_hi[3],clip_lo[3],clip_hi[3];
    for(unsigned k=0;k<3;k++)if(!isfinite(b->lo[k])||!isfinite(b->hi[k])||b->lo[k]>b->hi[k])return 0;
    for(unsigned row=0;row<3;row++){
        float lo=view[row][3],hi=lo,mag=fabsf(lo)+1.f;
        for(unsigned k=0;k<3;k++){
            float a=view[row][k]*b->lo[k],z=view[row][k]*b->hi[k];
            lo+=a<z?a:z;hi+=a>z?a:z;
            a=fabsf(a);z=fabsf(z);mag+=a>z?a:z;
        }
        eye_lo[row]=lo-.002f*mag;eye_hi[row]=hi+.002f*mag;
        if(!isfinite(eye_lo[row])||!isfinite(eye_hi[row]))return 0;
    }
    for(unsigned row=0;row<3;row++){
        const float *p=projection[row==2?3:row];
        float lo=p[3],hi=lo,mag=fabsf(lo)+1.f;
        for(unsigned k=0;k<3;k++){
            float a=p[k]*eye_lo[k],z=p[k]*eye_hi[k];
            lo+=a<z?a:z;hi+=a>z?a:z;
            a=fabsf(a);z=fabsf(z);mag+=a>z?a:z;
        }
        clip_lo[row]=lo-.002f*mag;clip_hi[row]=hi+.002f*mag;
        if(!isfinite(clip_lo[row])||!isfinite(clip_hi[row]))return 0;
    }
    /* Retain camera-plane crossings/behind-camera cases. This only rejects
     * known front-facing homogeneous boxes completely beyond a side plane. */
    if(clip_lo[2]<=.001f)return 0;
    for(unsigned axis=0;axis<2;axis++){
        /* The caller supplies viewport-folded, UNROTATED projection. */
        float half=axis?120.f:200.f,edge=clip_hi[2]*(1.f+.0625f/half);
        if(isfinite(edge)&&(clip_hi[axis]<-edge||clip_lo[axis]>edge))return 1;
    }
    return 0;
}
#endif
