#include <citro3d.h>
#include <string.h>
#include <stdio.h>
#include "fh_native_fog.h"
#include "fh_fog.h"
#include "fh_platform.h"

/* PICA applies transmission after TEV, preserving fragment alpha. One 512-byte
 * CPU LUT; no new shader instructions, linear allocation or per-vertex work.
 * The command encoder copies LUT words before a later draw can replace them. */
static C3D_FogLut native_fog_lut;
static uint32_t native_fog_key[5],native_fog_color;
static unsigned native_fog_valid,native_fog_active,native_fog_color_valid;
static unsigned native_fog_draws,native_fog_rebuilds,native_fog_rejected;
void fh_native_fog_reset(void){
    native_fog_valid=native_fog_active=native_fog_color_valid=0;
    native_fog_draws=native_fog_rebuilds=native_fog_rejected=0;
}

void fh_native_fog_apply(unsigned enable){
    fh_native_fog_apply_state(enable?fh_gx_native_fog_state():NULL);
}
void fh_native_fog_apply_state(const FhDrawState *s){
    unsigned type=s?s->fog_type&7:0;
    if(type && (s->fog_type>15 || (type!=2 && type<4) ||
        !isfinite(s->fog_start) || !isfinite(s->fog_end) ||
        !isfinite(s->fog_near) || !isfinite(s->fog_far) ||
        s->fog_near<0 || s->fog_far<s->fog_near)){
        native_fog_rejected++;type=0;
    }
    if(!type){
        /* Also initialize explicitly: never inherit another application's fog. */
        C3D_FogGasMode(GPU_NO_FOG,(GPU_GASMODE)0,false);native_fog_active=0;return;
    }
    uint32_t key[5]={s->fog_type};memcpy(key+1,&s->fog_start,16);
    if(!native_fog_valid || memcmp(key,native_fog_key,sizeof(key))){
        float samples[256],previous=0;
        for(unsigned i=0;i<=128;i++){
            float value=fh_fog_transmission(s,i/128.f);
            if(i<128)samples[i]=value;
            if(i)samples[i+127]=value-previous;
            previous=value;
        }
        FogLut_FromArray(&native_fog_lut,samples);
        memcpy(native_fog_key,key,sizeof(key));native_fog_valid=1;native_fog_rebuilds++;
        C3D_FogLutBind(&native_fog_lut);
    }
    if(!native_fog_color_valid || native_fog_color!=s->fog_color){
        unsigned c=s->fog_color;
        C3D_FogColor((c>>24)|((c>>8)&0xff00)|((c<<8)&0xff0000));
        native_fog_color=c;native_fog_color_valid=1;
    }
    if(!native_fog_active)C3D_FogGasMode(GPU_FOG,(GPU_GASMODE)0,false);
    native_fog_active=1;native_fog_draws++;
}
void fh_native_fog_report(void){
    char line[160];snprintf(line,sizeof(line),"Native fog: draws=%u rebuilds=%u rejected=%u cpu_bytes=%u\n",
        native_fog_draws,native_fog_rebuilds,native_fog_rejected,(unsigned)sizeof(native_fog_lut));
    fh_platform_log(line);
}
