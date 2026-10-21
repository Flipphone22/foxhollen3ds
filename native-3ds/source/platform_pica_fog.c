/* Native PICA procedural alpha LUT: linear eye distance, not non-linear depth.
 * Resident CPU LUTs are copied into command words by Citro3D. No vertex rebuild,
 * texture allocation, extra geometry pass, readback or legacy GX state reader. */
#include <citro3d.h>
#include <string.h>
#include <stdio.h>
#include "fh_pica_fog.h"
#include "fh_pica_post_stages.h"
#include "fh_graphics_options.h"
#include "fh_platform.h"
#include "fh_pica_layered.h"
static C3D_ProcTex procedural;
static C3D_ProcTexColorLut colors;
static C3D_ProcTexLut alpha_lut,rgb_lut;
static float range[4],scale;
static float compiled_start,compiled_end,last_inactive_range[4];
static unsigned compiled_valid,curve_reuses;
static unsigned ready,dirty,color,color_dirty,mode=~0u,luts_bound,uniform_dirty;
static unsigned draws,rebuilds,changes,rejected;
static int uniform_register;
static float layered_scale=1,layered_parameters[2];
static unsigned layered_draws;
void fh_pica_fog_init(int uniform){
    uniform_register=uniform;mode=~0u;luts_bound=0;dirty=1;color_dirty=1;uniform_dirty=1;
    draws=rebuilds=changes=rejected=curve_reuses=compiled_valid=0;
    layered_draws=0;layered_scale=1;
    if(uniform<0 || uniform>=96)fh_platform_panic("PICA fog uniform budget");
    C3D_ProcTexInit(&procedural,0,2);
    C3D_ProcTexClamp(&procedural,GPU_PT_CLAMP_TO_EDGE,GPU_PT_CLAMP_TO_EDGE);
    C3D_ProcTexCombiner(&procedural,true,GPU_PT_U,GPU_PT_U);
    C3D_ProcTexFilter(&procedural,GPU_PT_NEAREST);
    const u32 white[2]={0xffffffff,0xffffffff};
    memset(&colors,0,sizeof(colors));ProcTexColorLut_Write(&colors,white,0,2);
    float zero[129]={0};ProcTexLut_FromArray(&rgb_lut,zero);
}
void fh_pica_fog_range(float start,float end,float near_z,float far_z){
    float next[4]={start,end,near_z,far_z};
    if(!memcmp(range,next,sizeof(range)))return;
    memcpy(range,next,sizeof(range));dirty=1;
    ready=isfinite(start)&&isfinite(end)&&isfinite(near_z)&&isfinite(far_z)&&
        start>=0 && end>start && near_z>0 && far_z>near_z;
    if(ready){
        float normalization=fmaxf(end,near_z),width=end-start;
        layered_scale=1.f/normalization;
        layered_parameters[0]=normalization/width;
        layered_parameters[1]=-(normalization+start)/width;
        if(mode==2 || mode==3)uniform_dirty=1;
    }
    if(!ready){rejected++;memcpy(last_inactive_range,next,sizeof(next));}
    else if(compiled_valid && !(fh_graphics_options&FH_GRAPHICS_NO_DISTANCE_FOG) &&
            fh_pica_fog_same_curve(start,end,compiled_start,compiled_end)){
        dirty=0;curve_reuses++;
    }
}
void fh_pica_fog_color(unsigned r,unsigned g,unsigned b){
    unsigned next=(r&255)|((g&255)<<8)|((b&255)<<16)|0xff000000u;
    if(next!=color){color=next;color_dirty=1;}
}
void fh_pica_fog_invalidate(void){mode=~0u;color_dirty=1;uniform_dirty=1;}
float fh_pica_fog_distance_scale(void){return scale;}
void fh_pica_fog_bind(unsigned enabled){
    enabled=enabled && ready && !(fh_graphics_options&FH_GRAPHICS_NO_DISTANCE_FOG);
    if(enabled && dirty){
        float values[129];scale=1.f/fmaxf(range[1],range[2]);
        for(unsigned i=0;i<=128;i++)values[i]=fh_pica_fog_transmission((i/128.f)/scale,range[0],range[1]);
        ProcTexLut_FromArray(&alpha_lut,values);
        C3D_ProcTexLutBind(GPU_LUT_ALPHAMAP,&alpha_lut);dirty=0;rebuilds++;
        compiled_start=range[0];compiled_end=range[1];compiled_valid=1;uniform_dirty=1;
    }
    if(enabled && !luts_bound){
        C3D_ProcTexLutBind(GPU_LUT_RGBMAP,&rgb_lut);
        C3D_ProcTexColorLutBind(&colors);luts_bound=1;
    }
    if(mode!=enabled){
        C3D_TexEnv *e=C3D_GetTexEnv(FH_PICA_FOG_STAGE);C3D_TexEnvInit(e);
        C3D_ProcTexBind(2,enabled?&procedural:NULL);
        if(enabled){
            C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_CONSTANT,GPU_TEXTURE3);
            C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
            /* Init preserves PREVIOUS alpha; fog never changes cutout/opacity. */
        }
        mode=enabled;changes++;color_dirty=1;uniform_dirty=1;
    }
    /* Vertex programs still emit the fog coordinate when the fragment stage
     * is OFF. Publish a finite neutral input after init/foreign-owner handoff;
     * disabled fragment sampling does not make an undefined VSH row safe. */
    if(uniform_dirty){
        C3D_FVUnifSet(GPU_VERTEX_SHADER,uniform_register,enabled?scale:0.f,0,0,0);
        uniform_dirty=0;
    }
    if(enabled){
        if(color_dirty){C3D_TexEnvColor(C3D_GetTexEnv(FH_PICA_FOG_STAGE),color);color_dirty=0;}
        draws++;
    }
}
unsigned fh_pica_fog_layered_enabled(unsigned enabled){
    return enabled && ready && !(fh_graphics_options&FH_GRAPHICS_NO_DISTANCE_FOG);
}
unsigned fh_pica_fog_layered_bind(unsigned enabled,float parameters[2]){
    enabled=fh_pica_fog_layered_enabled(enabled);
    unsigned selected=enabled?2:3;
    if(mode!=selected){
        C3D_TexEnv *e=C3D_GetTexEnv(FH_PICA_FOG_STAGE);C3D_TexEnvInit(e);
        C3D_ProcTexBind(2,NULL);
        if(enabled){
            C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_CONSTANT,GPU_FRAGMENT_SECONDARY_COLOR);
            C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
        }
        mode=selected;changes++;color_dirty=uniform_dirty=1;
    }
    if(uniform_dirty){
        C3D_FVUnifSet(GPU_VERTEX_SHADER,uniform_register,layered_scale,0,0,0);uniform_dirty=0;
    }
    if(enabled){
        if(color_dirty){C3D_TexEnvColor(C3D_GetTexEnv(FH_PICA_FOG_STAGE),color);color_dirty=0;}
        layered_draws++;
    }
    parameters[0]=layered_parameters[0];parameters[1]=layered_parameters[1];
    return enabled;
}
void fh_pica_fog_report(void){
    char line[256];snprintf(line,sizeof(line),"PICA distance fog: draws=%u rebuilds=%u changes=%u rejected=%u cpu_bytes=%u range=%.3f,%.3f nearfar=%.3f,%.3f\n",
        draws,rebuilds,changes,rejected,(unsigned)(sizeof(colors)+sizeof(alpha_lut)+sizeof(rgb_lut)+sizeof(procedural)),range[0],range[1],range[2],range[3]);
    fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA layered fog: draws=%u CPU_vertices=0 extra_geometry_passes=0\n",layered_draws);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA fog curve cache: reuses=%u max_transmission_delta=1/4095 last_inactive_range=%.3f,%.3f,%.3f,%.3f\n",
        curve_reuses,last_inactive_range[0],last_inactive_range[1],last_inactive_range[2],last_inactive_range[3]);fh_platform_log(line);
}
