#ifndef FH_NATIVE_VISIBILITY_H
#define FH_NATIVE_VISIBILITY_H
#include <math.h>
#include <stdint.h>
#include "fh_native_model.h"
static inline int fh_native_box_invisible(const float lo[3],const float hi[3],
    const float projection[4][4]);
/* 0: retain; 1: subpixel LOD only; 2: no covered pixel center; 3: off-screen. */
static inline int fh_native_box_coverage(const float lo[3],const float hi[3],
    const float projection[4][4]);
/* Compact model positions occupy signed16 lanes. Their bind tails and all
 * animated matrices are included in the bound; convex two-bone blending
 * cannot leave its union. Adapted from the pinned Melee geometry_bounds.h
 * conservative-box approach, extended here to our multi-bone compact ABI.
 * This only examines collapsed transforms, so ordinary draws exit cheaply.
 * No allocation, vertex walk, normal inversion or GPU state mutation. */
static inline int fh_native_collapsed_invisible(const float palette[][3][4],
    unsigned count,const uint16_t *bones,const float tails[][3],unsigned format,
    const float projection[4][4]){
    if(!count || count>22 || !bones || !tails)return 0;
    for(unsigned i=0;i<count;i++)for(unsigned r=0;r<3;r++)
        for(unsigned c=0;c<3;c++)if(!(fabsf(palette[i][r][c])<=1e-4f))return 0;
    float raw=format==7?32768.f:format==5?4096.f:128.f;
    float extent[3]={raw,raw,raw},lo[3],hi[3];
    for(unsigned i=0;i<count;i++)for(unsigned c=0;c<3;c++){
        float v=raw+fabsf(tails[bones[i]][c]);
        if(!isfinite(v))return 0;
        if(v>extent[c])extent[c]=v;
    }
    for(unsigned i=0;i<count;i++)for(unsigned r=0;r<3;r++){
        const float *m=palette[i][r];
        float radius=fabsf(m[0])*extent[0]+fabsf(m[1])*extent[1]+fabsf(m[2])*extent[2];
        /* Inflate for PICA float24 input/ALU quantization and CPU rounding. */
        radius+=1e-4f*(fabsf(m[3])+radius+1);
        float a=m[3]-radius,b=m[3]+radius;
        if(!isfinite(a)||!isfinite(b))return 0;
        if(!i||a<lo[r])lo[r]=a;
        if(!i||b>hi[r])hi[r]=b;
    }
    return fh_native_box_invisible(lo,hi,projection);
}
/* Refine only collapsed transforms using actual current compact positions.
 * The full signed16 domain above can be much larger than a tiny character.
 * Every bone transforms the entire decoded box: conservative for all legal
 * two-bone blends. Keep the same arithmetic inflation/pixel-center rule.
 * No cached bounds can become stale after an animated vertex upload. */
static inline int fh_native_collapsed_mesh_coverage(const float palette[][3][4],
    unsigned count,const FhModelCompactVertex *vertices,unsigned vertex_count,
    unsigned format,const float projection[4][4]){
    if(!count || count>22 || !vertices || !vertex_count || vertex_count>FH_MODEL_VERTICES ||
       (format!=3 && format!=5 && format!=6 && format!=7))return 0;
    for(unsigned i=0;i<count;i++)for(unsigned r=0;r<3;r++)
        for(unsigned c=0;c<3;c++)if(!(fabsf(palette[i][r][c])<=1e-3f))return 0;
    float scale=format==7?1.f:format==5?1.f/8:1.f/256;
    float minimum[3],maximum[3];
    for(unsigned i=0;i<vertex_count;i++)for(unsigned c=0;c<3;c++){
        float raw=vertices[i].position[c]*scale,tail=vertices[i].tail[c];
        float v=raw+tail,error=1e-4f*(fabsf(raw)+fabsf(tail)+1);
        if(!isfinite(v)||!isfinite(error))return 0;
        if(!i||v-error<minimum[c])minimum[c]=v-error;
        if(!i||v+error>maximum[c])maximum[c]=v+error;
    }
    float lo[3],hi[3];
    for(unsigned i=0;i<count;i++)for(unsigned r=0;r<3;r++){
        const float *m=palette[i][r];float a=m[3],b=m[3],mag=fabsf(m[3])+1;
        for(unsigned c=0;c<3;c++){
            float x=m[c]*minimum[c],y=m[c]*maximum[c];
            a+=fminf(x,y);b+=fmaxf(x,y);mag+=fmaxf(fabsf(x),fabsf(y));
        }
        a-=1e-4f*mag;b+=1e-4f*mag;
        if(!isfinite(a)||!isfinite(b))return 0;
        if(!i||a<lo[r])lo[r]=a;
        if(!i||b>hi[r])hi[r]=b;
    }
    return fh_native_box_coverage(lo,hi,projection);
}
static inline int fh_native_collapsed_mesh_invisible(const float palette[][3][4],
    unsigned count,const FhModelCompactVertex *vertices,unsigned vertex_count,
    unsigned format,const float projection[4][4]){
    return fh_native_collapsed_mesh_coverage(palette,count,vertices,vertex_count,format,projection)>=2;
}
static inline int fh_native_box_coverage(const float lo[3],const float hi[3],
    const float projection[4][4]){
    float lower[3],upper[3];
    for(unsigned r=0;r<3;r++){
        const float *p=projection[r==2?3:r];float a=p[3],b=p[3],mag=fabsf(p[3])+1;
        for(unsigned c=0;c<3;c++){
            float x=p[c]*lo[c],y=p[c]*hi[c];
            a+=fminf(x,y);b+=fmaxf(x,y);mag+=fmaxf(fabsf(x),fabsf(y));
        }
        lower[r]=a-1e-4f*mag;upper[r]=b+1e-4f*mag;
        if(!isfinite(lower[r])||!isfinite(upper[r]))return 0;
    }
    /* Never make a size decision across/behind the camera plane. */
    if(lower[2]<=1e-3f)return 0;
    /* Independent homogeneous side-plane rejection, adapted from pinned
     * Melee geometry_bounds.h. An off-screen box need not be subpixel.
     * Use the largest possible W and an additional 1/16-pixel margin on
     * top of the inflated matrix/position bounds. Never divide by W here. */
    for(unsigned axis=0;axis<2;axis++){
        float half=axis?200.f:120.f;
        float edge=upper[2]*(1.f+.0625f/half);
        if(isfinite(edge) && (upper[axis]<-edge || lower[axis]>edge))return 3;
    }
    unsigned empty_axis=0;
    for(unsigned axis=0;axis<2;axis++){
        float a=lower[axis]/lower[2],b=lower[axis]/upper[2];
        float c=upper[axis]/lower[2],d=upper[axis]/upper[2];
        float half=axis?200.f:120.f; /* rotated 240 x 400 top viewport */
        float low=(fminf(a,b)+1)*half,high=(fmaxf(c,d)+1)*half;
        if(!isfinite(low)||!isfinite(high)||high-low>=.25f)return 0;
        /* Subpixel size alone is insufficient: retain any box that could
         * contain a pixel center. 1/16 pixel covers raster subpixel rounding. */
        low-=.0625f;high+=.0625f;
        if(ceilf(low-.5f)>floorf(high-.5f))empty_axis=1;
    }
    return empty_axis?2:1;
}
static inline int fh_native_box_invisible(const float lo[3],const float hi[3],
    const float projection[4][4]){
    return fh_native_box_coverage(lo,hi,projection)>=2;
}
#endif
