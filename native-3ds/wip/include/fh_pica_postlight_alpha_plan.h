#pragma once
/* Four-stage native post-light overlay with secondary-light alpha.
 * T0 is the existing half-intensity RGB specular map, T1 base, T2 overlay.
 * C0.rgb=diffuse strength, C0.a=secondary-light alpha; C1.a=base opacity.
 * Publish stage0 RGB into the delayed TEV buffer (mask 1). Keep stages4/5
 * for colour fade/fog. No extra pass or map is needed. Owner integration
 * must admit only verified source layer/opacity modes.
 */
static inline void fh_pica_postlight_alpha_plan(C3D_TexEnv e[4],unsigned environment,unsigned selector){
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
    C3D_TexEnvSrc(e,C3D_RGB,environment?GPU_FRAGMENT_PRIMARY_COLOR:GPU_PRIMARY_COLOR,GPU_CONSTANT,GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(e,C3D_RGB,environment?GPU_MULTIPLY_ADD:GPU_REPLACE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_G,GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    /* (S/2 + G*k/2)*L cannot overflow before the multiply. Restore scale
     * afterward, preserving source C2 saturation independently of base. */
    C3D_TexEnvSrc(e+1,C3D_RGB,GPU_TEXTURE0,GPU_PREVIOUS,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+1,(selector&1)?GPU_TEVOP_RGB_SRC_B:GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(e+1,C3D_RGB,GPU_ADD_MULTIPLY);
    C3D_TexEnvScale(e+1,C3D_RGB,GPU_TEVSCALE_2);
    C3D_TexEnvSrc(e+1,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e+1,C3D_Alpha,GPU_MODULATE);
    C3D_TexEnvSrc(e+2,C3D_RGB,GPU_TEXTURE1,GPU_PREVIOUS_BUFFER,GPU_PREVIOUS);
    C3D_TexEnvFunc(e+2,C3D_RGB,GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(e+3,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE2,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+3,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e+3,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvSrc(e+3,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE2,0);
    C3D_TexEnvFunc(e+3,C3D_Alpha,GPU_MODULATE);
}
