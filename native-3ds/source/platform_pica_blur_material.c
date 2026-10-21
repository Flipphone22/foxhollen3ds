#include "fh_pica_blur_material.h"

unsigned fh_pica_blur_material_mode(unsigned one_sided,unsigned strong){
    return strong?3:one_sided?2:1;
}
void fh_pica_blur_material_threshold(FhPicaBlurMaterial*m,unsigned threshold){
    for(unsigned mode=1;mode<4;mode++)for(unsigned stage=0;stage<2;stage++)
        m->stages[mode][stage].color=(m->stages[mode][stage].color&0xffffffu)|((threshold&255u)<<24);
}
void fh_pica_blur_material_init(FhPicaBlurMaterial*m,unsigned depth_alpha){
    GPU_TEVOP_A inverted_depth=depth_alpha?GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA:GPU_TEVOP_A_ONE_MINUS_SRC_G;
    for(unsigned mode=0;mode<4;mode++)for(unsigned i=0;i<6;i++)
        C3D_TexEnvInit(&m->stages[mode][i]);
    /* Source K1=102/255, final /2; its two identical upward samples combine
     * to 102/255. Remaining three samples each contribute 51/255. Partial
     * sum is <=.8, so the PICA's per-stage clamp cannot truncate HDR sums. */
    C3D_TexEnv*e=m->stages[0];
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);C3D_TexEnvColor(e,0xff666666);
    for(unsigned i=1;i<3;i++){
        C3D_TexEnvSrc(e+i,C3D_RGB,i==1?GPU_TEXTURE1:GPU_TEXTURE2,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvFunc(e+i,C3D_RGB,GPU_MULTIPLY_ADD);C3D_TexEnvColor(e+i,0xff333333);
    }
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);
    for(unsigned mode=1;mode<4;mode++){
        e=m->stages[mode];
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);C3D_TexEnvColor(e,0x00333333);
        C3D_TexEnvSrc(e+1,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE0,0);
        C3D_TexEnvFunc(e+1,C3D_RGB,GPU_ADD);
        /* Original Z8 direction D is one minus captured native depth. */
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE2,GPU_CONSTANT,0);
        C3D_TexEnvOpAlpha(e,inverted_depth,GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvFunc(e,C3D_Alpha,GPU_SUBTRACT);
        if(mode!=2){
            C3D_TexEnvSrc(e+1,C3D_Alpha,GPU_CONSTANT,GPU_TEXTURE2,0);
            C3D_TexEnvOpAlpha(e+1,GPU_TEVOP_A_SRC_ALPHA,inverted_depth,0);
            C3D_TexEnvFunc(e+1,C3D_Alpha,GPU_SUBTRACT);
            C3D_TexEnvSrc(e+2,C3D_Alpha,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
            C3D_TexEnvFunc(e+2,C3D_Alpha,GPU_ADD);
        }
        /* Source normal: /2,/2,*4,*4,*4,*4 = *64; strong: *4^4.
         * Algebraic equivalence is before GX/PICA intermediate quantization;
         * source byte-rounding and actual pixels need separate acceptance. */
        if(mode==3)C3D_TexEnvScale(e+2,C3D_Alpha,GPU_TEVSCALE_4);
        for(unsigned i=3;i<6;i++)C3D_TexEnvScale(e+i,C3D_Alpha,GPU_TEVSCALE_4);
    }
}
