#include "fh_pica_heat_material.h"
void fh_pica_heat_material_init(C3D_TexEnv e[6]){
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(e+i);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,0,0);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_ONE_MINUS_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_SUBTRACT);
    C3D_TexEnvScale(e,C3D_Alpha,GPU_TEVSCALE_4);
    C3D_TexEnvScale(e+1,C3D_Alpha,GPU_TEVSCALE_4);
    C3D_TexEnvSrc(e+2,C3D_Alpha,GPU_PREVIOUS,GPU_PRIMARY_COLOR,0);
    C3D_TexEnvFunc(e+2,C3D_Alpha,GPU_MODULATE);
    C3D_TexEnvScale(e+2,C3D_Alpha,GPU_TEVSCALE_4);
}
void fh_pica_heat_material_threshold(C3D_TexEnv e[6],unsigned threshold){
    e[0].color=(threshold&255u)<<24;
}
