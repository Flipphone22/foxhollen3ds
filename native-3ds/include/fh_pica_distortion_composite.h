#ifndef FH_PICA_DISTORTION_COMPOSITE_H
#define FH_PICA_DISTORTION_COMPOSITE_H
#include <3ds.h>
#include <citro3d.h>
/* Final distortion material after a separate GPU depth-mask pass.
 * Texture0: displaced colour; texture1: undisplaced completed depth mask;
 * texture2: original radial alpha. No combiner-buffer updates are required.
 * Preserve the source INVSRCALPHA/SRCALPHA framebuffer blend. The raw depth
 * history must remain separate from the mask, including across hidden-HUD
 * frames that do not refresh reflection textures. Not integrated yet. */
static inline void fh_pica_distortion_composite_prepare(C3D_TexEnv e[6]){
    for(unsigned s=0;s<6;s++)C3D_TexEnvInit(e+s);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,0,0);
    for(unsigned s=1;s<4;s++){
        C3D_TexEnvSrc(e+s,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(e+s,s==1?GPU_TEVOP_RGB_SRC_R:s==2?GPU_TEVOP_RGB_SRC_G:GPU_TEVOP_RGB_SRC_B,0,0);
        C3D_TexEnvFunc(e+s,C3D_RGB,GPU_MULTIPLY_ADD);
    }
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE2,GPU_TEXTURE1,0);
    C3D_TexEnvOpAlpha(e,0,GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
}
static inline void fh_pica_distortion_composite_parameters(C3D_TexEnv e[6],const uint32_t c[4]){
    e[0].color=c[3];e[1].color=c[0];e[2].color=c[1];e[3].color=c[2];
}
#endif
