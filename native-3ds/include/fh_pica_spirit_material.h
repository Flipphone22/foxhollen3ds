#ifndef FH_PICA_SPIRIT_MATERIAL_H
#define FH_PICA_SPIRIT_MATERIAL_H
#include <3ds.h>
#include <citro3d.h>
/* Experimental source Spirit Vision description, not a live game owner.
 * Caller publishes six stages, updates RGB combiner buffer from stage 0 only,
 * and writes just channel output_channel. Three channel draws share one capture.
 * Keep blue*K2 in the buffer while stages 1..3 reproduce clamped red/green.
 * Stage 4 averages the two terms before stage 5 performs source subtraction.
 * Constant alpha127 approximates 1/2 by127/255. Both nearest choices passed
 * single-pass tests; 127 reduced compounded rounding in the owner fixture.
 * This remains an approximation, with physical precision unvalidated.
 */
static inline void fh_pica_spirit_material_prepare(C3D_TexEnv e[6],unsigned output_channel){
    static const GPU_TEVOP_RGB remap[3]={GPU_TEVOP_RGB_SRC_G,GPU_TEVOP_RGB_SRC_B,GPU_TEVOP_RGB_SRC_R};
    for(unsigned s=0;s<6;s++)C3D_TexEnvInit(e+s);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_B,GPU_TEVOP_RGB_SRC_COLOR,0);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
    C3D_TexEnvSrc(e+1,C3D_Both,GPU_CONSTANT,0,0);
    for(unsigned s=2;s<4;s++){
        C3D_TexEnvSrc(e+s,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(e+s,s==2?GPU_TEVOP_RGB_SRC_R:GPU_TEVOP_RGB_SRC_G,0,0);
        C3D_TexEnvFunc(e+s,C3D_RGB,GPU_MULTIPLY_ADD);
    }
    C3D_TexEnvSrc(e+4,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,GPU_CONSTANT);
    C3D_TexEnvOpRgb(e+4,0,0,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e+4,C3D_RGB,GPU_INTERPOLATE);
    C3D_TexEnvColor(e+4,0x7f000000);
    C3D_TexEnvSrc(e+5,C3D_RGB,GPU_TEXTURE0,GPU_PREVIOUS,0);
    C3D_TexEnvOpRgb(e+5,remap[output_channel],0,0);
    C3D_TexEnvFunc(e+5,C3D_RGB,GPU_SUBTRACT);
    C3D_TexEnvScale(e+5,C3D_RGB,GPU_TEVSCALE_4);
}
static inline void fh_pica_spirit_material_parameters(C3D_TexEnv e[6],const unsigned coefficient[3],u32 bias){
    C3D_TexEnvColor(e,coefficient[2]);C3D_TexEnvColor(e+1,bias);
    C3D_TexEnvColor(e+2,coefficient[0]);C3D_TexEnvColor(e+3,coefficient[1]);
}
#endif
