#include <citro3d.h>
#include <stdio.h>
#include "fh_pica_color_fade.h"
#include "fh_pica_post_stages.h"
#include "fh_pica_height_fog.h"
#include "fh_platform.h"
static unsigned color,resident,valid,objects,draws,publications;
void fh_pica_color_fade_set(unsigned rgba){
    color=(rgba>>24)?rgba:0;
    objects+=color!=0;
}
void fh_pica_color_fade_invalidate(void){valid=0;}
void fh_pica_color_fade_bind(unsigned actor){
    unsigned rgba=actor?color:0;
    if(!valid || resident!=rgba){
        fh_pica_height_fog_stage_invalidate();
        C3D_TexEnv *e=C3D_GetTexEnv(FH_PICA_COLOR_FADE_STAGE);C3D_TexEnvInit(e);
        if(rgba){
            /* GX: lerp(CPREV, KONST.rgb, A0). PICA interpolation arguments
             * are reversed. Init retains PREVIOUS alpha, including cutouts. */
            C3D_TexEnvSrc(e,C3D_RGB,GPU_CONSTANT,GPU_PREVIOUS,GPU_CONSTANT);
            C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
            C3D_TexEnvColor(e,rgba);
        }
        resident=rgba;valid=1;publications++;
    }
    draws+=rgba!=0;
}
void fh_pica_color_fade_report(void){
    char line[160];snprintf(line,sizeof(line),"PICA color fade: objects=%u draws=%u publications=%u extra_passes=0 storage_bytes=%u\n",
        objects,draws,publications,(unsigned)(6*sizeof(unsigned)));fh_platform_log(line);
}
