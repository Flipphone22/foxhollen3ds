#ifndef FH_PICA_SCREEN_COLOR_H
#define FH_PICA_SCREEN_COLOR_H
#include <3ds.h>
#include <citro3d.h>
/* Source doColorFilter: bias + K0*R + K1*G + K2*B, clamped after
 * each source addition. Fullscreen owner must publish all six stages and
 * invalidate other material owners (pinned Melee raster ownership pattern).
 * This only prepares a native description; it does not capture or draw.
 * Source mod changes only three constants. No allocation/vertex work. */
static inline void fh_pica_screen_color_prepare(C3D_TexEnv out[6]) {
    static const GPU_TEVOP_RGB component[3]={GPU_TEVOP_RGB_SRC_R,GPU_TEVOP_RGB_SRC_G,GPU_TEVOP_RGB_SRC_B};
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(out+i);
    C3D_TexEnvSrc(out,C3D_Both,GPU_CONSTANT,0,0);
    C3D_TexEnvColor(out,0xff0a0a0a);
    for(unsigned i=0;i<3;i++) {
        C3D_TexEnv *e=out+1+i;
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(e,component[i],GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
    }
}
static inline void fh_pica_screen_color_parameters(C3D_TexEnv out[6],const u8 mod[3]) {
    static const u8 base[3]={0x42,0x6e,0x14};
    for(unsigned i=0;i<3;i++) {
        u32 r=(u8)(base[i]+(mod[0]>>3));
        u32 g=(u8)(base[i]+(mod[1]>>3));
        u32 b=(u8)(base[i]+(mod[2]>>3));
        C3D_TexEnvColor(out+1+i,r|(g<<8)|(b<<16));
    }
}
#endif
