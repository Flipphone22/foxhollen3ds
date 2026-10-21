#ifndef FH_NATIVE_NORMAL_PALETTE_H
#define FH_NATIVE_NORMAL_PALETTE_H
#include <math.h>
#include "fh_native_model.h"
/* Resolve finite, difficult transforms on ARM instead of sending them back to
 * PICA inversion. The physical replay of draw61 stalls on that GPU path, while
 * the same positions/material with CPU inverse normals completes. A common
 * positive scale bounds uploads without changing two-bone normal blend ratios.
 * This slow path is only used after the ordinary float inverse declines. */
static inline int fh_native_normal_palette_precise(float palette[FH_MODEL_BONES][3][4],unsigned count){
    if(!count || count>11)return 0;
    double values[11][3][3],largest=0;
    for(unsigned i=0;i<count;i++){
        const float (*m)[4]=palette[i];
        double a=m[0][0],b=m[0][1],c=m[0][2],d=m[1][0],e=m[1][1],f=m[1][2],
               g=m[2][0],h=m[2][1],j=m[2][2];
        double co[3][3]={{e*j-f*h,f*g-d*j,d*h-e*g},
                          {h*c-j*b,j*a-g*c,g*b-h*a},
                          {b*f-c*e,c*d-a*f,a*e-b*d}};
        double det=a*co[0][0]+b*co[0][1]+c*co[0][2];
        if(!isfinite(det))return 0;
        double inverse=fabs(det)>1e-12?1.0/det:0;
        for(unsigned r=0;r<3;r++)for(unsigned k=0;k<3;k++){
            double v=co[r][k]*inverse;
            if(!isfinite(v))return 0;
            values[i][r][k]=v;
            if(fabs(v)>largest)largest=fabs(v);
        }
    }
    double scale=largest>1e6?1e6/largest:1;
    for(unsigned i=0;i<count;i++)for(unsigned r=0;r<3;r++){
        for(unsigned k=0;k<3;k++)palette[11+i][r][k]=(float)(values[i][r][k]*scale);
        palette[11+i][r][3]=0;
    }
    return 1;
}
/* Eleven position matrices and eleven inverse-transpose matrices share the
 * existing 66 float rows. All packaged opening model groups fit eleven bones.
 * Larger/invalid input palettes are rejected; finite ill-conditioned palettes
 * use the ARM double path above, never the small-determinant GPU inverse.
 * Fully collapsed transforms publish the shader's guarded zero normal instead.
 * Positions in the first half are never changed; partial second-half output
 * on rejection is unpublished and unused by the fallback shader. */
static inline int fh_native_normal_palette(float palette[FH_MODEL_BONES][3][4],unsigned count){
    if(!count || count>11)return 0;
    for(unsigned i=0;i<count;i++){
        const float (*m)[4]=palette[i];
        float a=m[0][0],b=m[0][1],c=m[0][2],d=m[1][0],e=m[1][1],f=m[1][2],
              g=m[2][0],h=m[2][1],j=m[2][2];
        if(!isfinite(a) || !isfinite(b) || !isfinite(c) || !isfinite(d) ||
           !isfinite(e) || !isfinite(f) || !isfinite(g) || !isfinite(h) || !isfinite(j))return 0;
        float ar=fabsf(a),br=fabsf(b),cr=fabsf(c);
        float dr=fabsf(d),er=fabsf(e),fr=fabsf(f);
        float gr=fabsf(g),hr=fabsf(h),jr=fabsf(j);
        float s0=ar>br?ar:br;s0=s0>cr?s0:cr;
        float s1=dr>er?dr:er;s1=s1>fr?s1:fr;
        float s2=gr>hr?gr:hr;s2=s2>jr?s2:jr;
        float scale=(s0*s1)*s2;
        /* abs(det) <= 6*s0*s1*s2. This bound leaves >16x margin below
         * the existing PICA 1e-12 determinant guard, including float24
         * quantization. Opening cutscenes scale hidden models to 1e-6;
         * their normal is already zero in the inverse shader. Publish that
         * exact result without sending collapsed matrices down the fallback.
         * Keep positions, triangles and all material work unchanged. */
        if(isfinite(scale) && scale<=1e-14f){
            for(unsigned r=0;r<3;r++)for(unsigned c=0;c<4;c++)palette[11+i][r][c]=0;
            continue;
        }
        /* Same cyclic cofactors and evaluation order as the original helper.
         * Explicit fixed-size expressions avoid ARM loop/index overhead. */
        float co[3][3]={{e*j-f*h,f*g-d*j,d*h-e*g},
                        {h*c-j*b,j*a-g*c,g*b-h*a},
                        {b*f-c*e,c*d-a*f,a*e-b*d}};
        float det=m[0][0]*co[0][0]+m[0][1]*co[0][1]+m[0][2]*co[0][2];
        /* Avoid changing the old float24 determinant guard near singularity.
         * No orthogonality approximation: admitted matrices use the full inverse. */
        if(!isfinite(scale) || !isfinite(det) || fabsf(det)<=1e-10f || fabsf(det)<=scale*1e-4f)
            return fh_native_normal_palette_precise(palette,count);
        float inverse=1.f/det;
        float n00=co[0][0]*inverse,n01=co[0][1]*inverse,n02=co[0][2]*inverse;
        float n10=co[1][0]*inverse,n11=co[1][1]*inverse,n12=co[1][2]*inverse;
        float n20=co[2][0]*inverse,n21=co[2][1]*inverse,n22=co[2][2]*inverse;
        /* Ordered <= rejects NaN and infinity as well as oversized values. */
        if(!(fabsf(n00)<=1e6f) || !(fabsf(n01)<=1e6f) || !(fabsf(n02)<=1e6f) ||
           !(fabsf(n10)<=1e6f) || !(fabsf(n11)<=1e6f) || !(fabsf(n12)<=1e6f) ||
           !(fabsf(n20)<=1e6f) || !(fabsf(n21)<=1e6f) || !(fabsf(n22)<=1e6f))
            return fh_native_normal_palette_precise(palette,count);
        float (*out)[4]=palette[11+i];
        out[0][0]=n00;out[0][1]=n01;out[0][2]=n02;out[0][3]=0;
        out[1][0]=n10;out[1][1]=n11;out[1][2]=n12;out[1][3]=0;
        out[2][0]=n20;out[2][1]=n21;out[2][2]=n22;out[2][3]=0;
    }
    return 1;
}
#endif
