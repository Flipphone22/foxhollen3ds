#ifndef FH_ANIMATED_BOUNDS_H
#define FH_ANIMATED_BOUNDS_H
#include <math.h>
#include <stdint.h>
#include "fh_native_model.h"
#include "fh_world_bounds.h"
/* Bounds belong to a particular mutable model backing. used=0 invalidates
 * them whenever positions change. Include both skin influences, even weight0.
 * Pinned Melee's homogeneous-plane test is extended to convex skinning;
 * never use static bind bounds directly against an animated world matrix. */
typedef struct {unsigned used;FhWorldBounds bone[FH_MODEL_BONES];} FhAnimatedBounds;
static inline int fh_animated_bounds_build(FhAnimatedBounds *b,
    const FhModelCompactVertex *v,unsigned n,unsigned bones,unsigned format){
    b->used=0;
    if(!v || !n || n>FH_MODEL_VERTICES || !bones || bones>FH_MODEL_BONES ||
       (format!=3 && format!=5 && format!=6 && format!=7))return 0;
    unsigned used=0;float scale=format==7?1.f:format==5?.125f:1.f/256;
    for(unsigned i=0;i<n;i++){
        if(v[i].skin[2]>4 || v[i].position[3]!=1)return 0;
        for(unsigned j=0;j<2;j++){
            unsigned slot=v[i].skin[j];if(slot%3 || slot>=bones*3)return 0;slot/=3;
            unsigned bit=1u<<slot;FhWorldBounds *box=b->bone+slot;
            for(unsigned c=0;c<3;c++){
                float raw=v[i].position[c]*scale,tail=v[i].tail[c];
                float p=raw+tail,e=1e-4f*(fabsf(raw)+fabsf(tail)+1);
                if(!isfinite(p)||!isfinite(e))return 0;
                if(!(used&bit)||p-e<box->lo[c])box->lo[c]=p-e;
                if(!(used&bit)||p+e>box->hi[c])box->hi[c]=p+e;
            }
            used|=bit;
        }
    }
    b->used=used;return 1;
}
static inline int fh_animated_bounds_outside(const FhAnimatedBounds *b,
    const float palette[][3][4],unsigned count,const float projection[4][4]){
    if(!b || !b->used || !count || count>FH_MODEL_BONES || (b->used>>count))return 0;
    for(unsigned r=0;r<4;r++)for(unsigned c=0;c<4;c++)if(!isfinite(projection[r][c]))return 0;
    float lo[3],hi[3];unsigned first=1;
    for(unsigned i=0;i<count;i++)if(b->used&(1u<<i)){
        const FhWorldBounds *box=b->bone+i;
        for(unsigned r=0;r<3;r++){
            const float *m=palette[i][r];float a=m[3],z=m[3],mag=fabsf(m[3])+1;
            for(unsigned c=0;c<3;c++){
                float x=m[c]*box->lo[c],y=m[c]*box->hi[c];
                if(!isfinite(x)||!isfinite(y))return 0;
                a+=x<y?x:y;z+=x>y?x:y;
                float ax=fabsf(x),ay=fabsf(y);mag+=ax>ay?ax:ay;
            }
            a-=1e-4f*mag;z+=1e-4f*mag;
            if(!isfinite(a)||!isfinite(z))return 0;
            if(first||a<lo[r])lo[r]=a;
            if(first||z>hi[r])hi[r]=z;
        }
        if(first && (b->used&(b->used-1))){
            /* The first transformed box is a subset of the final union. If
             * its center is on-screen, no side plane can reject that union.
             * This avoids transforming the other bones of visible models. */
            float center[3]={(lo[0]+hi[0])*.5f,(lo[1]+hi[1])*.5f,(lo[2]+hi[2])*.5f};
            float clip[3];
            for(unsigned r=0;r<3;r++){
                const float *p=projection[r==2?3:r];
                clip[r]=p[0]*center[0]+p[1]*center[1]+p[2]*center[2]+p[3];
            }
            if(isfinite(clip[0])&&isfinite(clip[1])&&isfinite(clip[2])&&
               fabsf(clip[0])<=clip[2] && fabsf(clip[1])<=clip[2])return 0;
        }
        first=0;
    }
    /* Common side plane must reject the entire union, including both bone
     * influences of every triangle. No perspective divide or near-plane LOD.
     * Include magnitudes BEFORE projection-row cancellation in the margin. */
    for(unsigned plane=0;plane<4;plane++){
        unsigned axis=plane/2;float sign=(plane&1)?-1.f:1.f;
        float maximum=projection[3][3]+sign*projection[axis][3];
        float mag=fabsf(projection[3][3])+fabsf(projection[axis][3])+1;
        for(unsigned c=0;c<3;c++){
            float p=projection[3][c]+sign*projection[axis][c];
            maximum+=p*(p>=0?hi[c]:lo[c]);
            mag+=(fabsf(projection[3][c])+fabsf(projection[axis][c]))*fmaxf(fabsf(lo[c]),fabsf(hi[c]));
        }
        if(isfinite(maximum)&&isfinite(mag)&&maximum<-.002f*mag)return 1;
    }
    return 0;
}
#endif
