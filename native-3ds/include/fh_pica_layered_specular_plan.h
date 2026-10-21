#pragma once
/* Four-stage prototype. NOT connected to the gameplay owner yet.
 * T0: secondary-light map with RGB divided by two, alpha exactly one half.
 * T1: base image A; T2: second layer B. Both need independent live UVs.
 * Source family: both layers precede lighting, interpolate mode, C2 alpha=0,
 * C2 modulated by A. Constant opacity and no independent opacity planes.
 * C0.rgb: diffuse-map strength; C0.a: half object/material opacity.
 * C2.a: one half. Save BOTH stage0 RGB and alpha in the delayed TEV buffer.
 * Stages4/5 remain available for colour fade and procedural distance fog.
 * A half-map owner and a three-UV-plus-fog design are still required.
 */
static inline void fh_pica_layered_specular_plan(C3D_TexEnv e[4],unsigned environment,unsigned selector){
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
    C3D_TexEnvSrc(e,C3D_RGB,environment?GPU_FRAGMENT_PRIMARY_COLOR:GPU_PRIMARY_COLOR,GPU_CONSTANT,GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(e,C3D_RGB,environment?GPU_MULTIPLY_ADD:GPU_REPLACE);
    /* Alpha temporarily stores (1+a)/2; buffer RGB retains light L. */
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,GPU_TEXTURE0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(e+1,C3D_RGB,GPU_TEXTURE2,GPU_PREVIOUS,0);
    C3D_TexEnvOpRgb(e+1,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_ONE_MINUS_SRC_ALPHA,0);
    C3D_TexEnvFunc(e+1,C3D_RGB,GPU_MODULATE);
    /* RGB=B*(1-a)/2, alpha=(a+S)/2. Neither can overflow. */
    C3D_TexEnvSrc(e+1,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE0,0);
    C3D_TexEnvOpAlpha(e+1,GPU_TEVOP_A_SRC_ALPHA,(selector&1)?GPU_TEVOP_A_SRC_B:GPU_TEVOP_A_SRC_R,0);
    C3D_TexEnvFunc(e+1,C3D_Alpha,GPU_ADD_SIGNED);
    C3D_TexEnvSrc(e+2,C3D_RGB,GPU_TEXTURE1,GPU_PREVIOUS,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+2,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(e+2,C3D_RGB,GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(e+2,C3D_Alpha,GPU_PREVIOUS_BUFFER,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e+2,C3D_Alpha,GPU_SUBTRACT);C3D_TexEnvScale(e+2,C3D_Alpha,GPU_TEVSCALE_2);
    C3D_TexEnvColor(e+2,0x80000000u);
    C3D_TexEnvSrc(e+3,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
    C3D_TexEnvFunc(e+3,C3D_RGB,GPU_MODULATE);C3D_TexEnvScale(e+3,C3D_RGB,GPU_TEVSCALE_2);
    C3D_TexEnvSrc(e+3,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE2,0);
    C3D_TexEnvFunc(e+3,C3D_Alpha,GPU_MODULATE);
}
