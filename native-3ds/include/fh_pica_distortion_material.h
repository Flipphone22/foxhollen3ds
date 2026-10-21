#ifndef FH_PICA_DISTORTION_MATERIAL_H
#define FH_PICA_DISTORTION_MATERIAL_H
#include <3ds.h>
#include <citro3d.h>

/* Texture0: displaced colour; texture1: native reversed depth in alpha;
 * texture2: source radial alpha. Enable ALPHA combiner-buffer update at stage1
 * only (mask=2). Its saved result is consumed at stage4. No RGB buffer update.
 * After the draw disable buffer updates before publishing another material.
 * Caller must retain the source INVSRCALPHA/SRCALPHA blend.
 * Not yet connected to a game draw owner. */
static inline void fh_pica_distortion_material_prepare(C3D_TexEnv e[6]){
    for(unsigned s=0;s<6;s++)C3D_TexEnvInit(e+s);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,0,0);
    for(unsigned s=1;s<4;s++){
        C3D_TexEnvSrc(e+s,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(e+s,s==1?GPU_TEVOP_RGB_SRC_R:s==2?GPU_TEVOP_RGB_SRC_G:GPU_TEVOP_RGB_SRC_B,0,0);
        C3D_TexEnvFunc(e+s,C3D_RGB,GPU_MULTIPLY_ADD);
    }
    /* Positive depth difference, then its radius weight; save stage1 alpha. */
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,0,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_SUBTRACT);
    C3D_TexEnvSrc(e+1,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e+1,C3D_Alpha,GPU_MODULATE);
    /* Negative depth difference and its separate radius weight. */
    C3D_TexEnvSrc(e+2,C3D_Alpha,GPU_CONSTANT,GPU_TEXTURE1,0);
    C3D_TexEnvOpAlpha(e+2,0,GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,0);
    C3D_TexEnvFunc(e+2,C3D_Alpha,GPU_SUBTRACT);
    C3D_TexEnvSrc(e+3,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e+3,C3D_Alpha,GPU_MODULATE);
    C3D_TexEnvSrc(e+4,C3D_Alpha,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
    C3D_TexEnvFunc(e+4,C3D_Alpha,GPU_ADD);
    for(unsigned s=0;s<5;s++)C3D_TexEnvScale(e+s,C3D_Alpha,GPU_TEVSCALE_4);
    C3D_TexEnvSrc(e+5,C3D_Alpha,GPU_TEXTURE2,GPU_PREVIOUS,0);
    C3D_TexEnvOpAlpha(e+5,0,GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,0);
    C3D_TexEnvFunc(e+5,C3D_Alpha,GPU_MODULATE);
}
static inline void fh_pica_distortion_material_parameters(C3D_TexEnv e[6],const uint32_t c[4]){
    e[0].color=(c[3]&0xffffffu)|(c[0]&0xff000000u);
    e[1].color=(c[0]&0xffffffu)|(c[3]&0xff000000u);
    e[2].color=(c[1]&0xffffffu)|(c[0]&0xff000000u);
    e[3].color=(c[2]&0xffffffu)|(c[1]&0xff000000u);
    /* The depth branches cannot both be positive. Their stage4 sum is scaled
     * by four and saturated. Fold small weights completely before rounding;
     * for larger weights double the input before multiply, halving the later
     * scale. The ADD_MULTIPLY input clamp only changes already-saturated final
     * mask values: x>1/2, weight>=64/255 implies 4*2*weight>1.
     * No extra stage or draw. Reconfigure on every parameter publication. */
    for(unsigned i=1;i<=3;i+=2){
        unsigned weight=e[i].color>>24;
        if(weight<=63){
            e[i].color=(e[i].color&0xffffffu)|(weight*4u<<24);
            C3D_TexEnvSrc(e+i,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);
            C3D_TexEnvFunc(e+i,C3D_Alpha,GPU_MODULATE);
            C3D_TexEnvScale(e+i,C3D_Alpha,GPU_TEVSCALE_1);
        }else{
            C3D_TexEnvSrc(e+i,C3D_Alpha,GPU_PREVIOUS,GPU_PREVIOUS,GPU_CONSTANT);
            C3D_TexEnvFunc(e+i,C3D_Alpha,GPU_ADD_MULTIPLY);
            C3D_TexEnvScale(e+i,C3D_Alpha,GPU_TEVSCALE_2);
        }
    }
}
#endif
