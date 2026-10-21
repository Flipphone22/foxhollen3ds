#ifndef FH_PICA_GROUND_SHADOW_H
#define FH_PICA_GROUND_SHADOW_H
#include <stdint.h>
/* Continuous receiver-plane transform. CPU collision supplies distance/axes;
 * the GPU consumes the same persistent unit quad for every contact shadow. */
static inline void fh_pica_ground_contact_matrix(float out[3][4],const float u[3],const float v[3],float distance){
    for(unsigned row=0;row<3;row++){
        out[row][0]=2*u[row];out[row][1]=2*v[row];out[row][2]=0;
        out[row][3]=(row==1?-distance:0)-u[row]-v[row];
    }
}
/* Original GX_VTXFMT6 positions are signed 8-fractional-bit coordinates.
 * Fit the quantized rectangle symmetrically, rather than snapping three corners
 * and concentrating all quantization error at the fourth. No vertex uploads. */
static inline void fh_pica_ground_shadow_matrix(float out[3][4],const int16_t corners[12],const float translation[3]){
    for(unsigned row=0;row<3;row++){
        float a=corners[row],b=corners[3+row],c=corners[6+row],d=corners[9+row];
        out[row][0]=(b+c-a-d)/512.f;
        out[row][1]=(c+d-a-b)/512.f;
        out[row][2]=0;
        out[row][3]=(3*a+b-c+d)/1024.f+translation[row];
    }
}
#endif
