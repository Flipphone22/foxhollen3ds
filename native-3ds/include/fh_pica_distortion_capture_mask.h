#ifndef FH_PICA_DISTORTION_CAPTURE_MASK_H
#define FH_PICA_DISTORTION_CAPTURE_MASK_H
#include <3ds.h>
#include <citro3d.h>
/* Experimental capture material, not connected to the platform owner.
 * Texture0 RGB is retained. Texture1 alpha is reversed depth. Buffer update
 * mask 4 (alpha stage2) publishes the positive HALF-mask for stage5.
 * Computing before filtering is not interchangeable with filtered depth:
 * callers must prove sampling equivalence before using this material. */
static inline void fh_pica_distortion_capture_mask(C3D_TexEnv e[6],
    unsigned threshold,unsigned positive_weight,unsigned negative_weight){
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(e+i);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,0,0);
    for(unsigned branch=0;branch<2;branch++){
        unsigned s=branch*3,w=branch?negative_weight:positive_weight;
        unsigned k=1;while(k<32 && w>=8*k)k*=2;
        unsigned first=k<4?k:4;
        e[s].color=threshold<<24;
        C3D_TexEnvSrc(e+s,C3D_Alpha,branch?GPU_CONSTANT:GPU_TEXTURE1,
            branch?GPU_TEXTURE1:GPU_CONSTANT,0);
        C3D_TexEnvOpAlpha(e+s,branch?0:GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,
            branch?GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA:0,0);
        C3D_TexEnvFunc(e+s,C3D_Alpha,GPU_SUBTRACT);
        C3D_TexEnvScale(e+s,C3D_Alpha,first==4?GPU_TEVSCALE_4:first==2?GPU_TEVSCALE_2:GPU_TEVSCALE_1);
        if(k>4){
            C3D_TexEnvSrc(e+s+1,C3D_Alpha,GPU_PREVIOUS,GPU_PREVIOUS,0);
            C3D_TexEnvFunc(e+s+1,C3D_Alpha,GPU_ADD);
            C3D_TexEnvScale(e+s+1,C3D_Alpha,k==32?GPU_TEVSCALE_4:k==16?GPU_TEVSCALE_2:GPU_TEVSCALE_1);
        }
        e[s+2].color=((32/k)*w)<<24;
        C3D_TexEnvSrc(e+s+2,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,GPU_PREVIOUS_BUFFER);
        C3D_TexEnvFunc(e+s+2,C3D_Alpha,branch?GPU_MULTIPLY_ADD:GPU_MODULATE);
        if(branch)C3D_TexEnvScale(e+s+2,C3D_Alpha,GPU_TEVSCALE_2);
    }
}
#endif
