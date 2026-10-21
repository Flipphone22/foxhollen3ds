#pragma once
/* Native four-stage plan for base -> lighting/specular -> overlay.
 * Prototype pending owner integration and visual acceptance.
 * T0=half-intensity specular map, T1=base, T2=independent overlay.
 * C0.rgb=diffuse strength, C0.a=object/material opacity; C1.rgb=1/2.
 * No TEV buffer is needed. Stages4/5 remain owned by colour fade and fog.
 * Only constant-opacity, base-modulated C2 with zero C2 alpha is admitted.
 */
static inline void fh_pica_postlight_specular_plan(C3D_TexEnv e[4],unsigned environment,unsigned selector){
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
    C3D_TexEnvSrc(e,C3D_RGB,environment?GPU_FRAGMENT_PRIMARY_COLOR:GPU_PRIMARY_COLOR,GPU_CONSTANT,GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(e,C3D_RGB,environment?GPU_MULTIPLY_ADD:GPU_REPLACE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    /* PICA clamps the ADD before multiplying. S/2+1/2 is bounded by one;
     * retain the factor of two until AFTER multiplying by base colour. */
    C3D_TexEnvSrc(e+1,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+1,(selector&1)?GPU_TEVOP_RGB_SRC_B:GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
    C3D_TexEnvFunc(e+1,C3D_RGB,GPU_ADD_MULTIPLY);
    C3D_TexEnvColor(e+1,0x00808080u);
    C3D_TexEnvSrc(e+2,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE1,0);
    C3D_TexEnvFunc(e+2,C3D_RGB,GPU_MODULATE);
    C3D_TexEnvScale(e+2,C3D_RGB,GPU_TEVSCALE_2);
    /* Saturated lit base is interpolated using ORIGINAL accumulated alpha. */
    C3D_TexEnvSrc(e+3,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE2,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e+3,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e+3,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvSrc(e+3,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE2,0);
    C3D_TexEnvFunc(e+3,C3D_Alpha,GPU_MODULATE);
}
