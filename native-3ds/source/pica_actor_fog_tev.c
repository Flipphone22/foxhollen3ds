#include "fh_pica_actor_fog_tev.h"
#include <string.h>
int fh_pica_actor_fog_tev(C3D_TexEnv out[4],const C3D_TexEnv base[4]){
    if(!out || !base)return 0;
    const C3D_TexEnv *e=base+3;
    if(e->funcRgb!=GPU_REPLACE || e->funcAlpha!=GPU_REPLACE ||
       (e->srcRgb&15)!=GPU_PREVIOUS || (e->srcAlpha&15)!=GPU_PREVIOUS ||
       (e->opRgb&15) || (e->opAlpha&7) ||
       e->scaleRgb!=GPU_TEVSCALE_1 || e->scaleAlpha!=GPU_TEVSCALE_1)return 0;
    C3D_TexEnv stages[4];memcpy(stages,base,sizeof(stages));
    C3D_TexEnvInit(stages+3);
    C3D_TexEnvSrc(stages+3,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_TEXTURE2);
    C3D_TexEnvOpRgb(stages+3,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(stages+3,C3D_RGB,GPU_INTERPOLATE);
    memcpy(out,stages,sizeof(stages));return 1;
}
