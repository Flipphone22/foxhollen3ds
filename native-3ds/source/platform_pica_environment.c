/* Native directional environment illumination. Replaces the diffuse spherical
 * light-map contribution with PICA fragment lights; no GX draws, texture copy,
 * per-vertex CPU lighting, or transient GPU allocation. */
#include <citro3d.h>
#include <string.h>
#include <stdio.h>
#include "fh_pica_scene.h"
#include "fh_platform.h"
#include "fh_pica_layered.h"
static C3D_LightEnv environment;
static C3D_Light lights[8];
static FhPicaEnvironment snapshots[3];
static unsigned valid[3];
static unsigned revisions[3],resident_slot=~0u,resident_revision;
static unsigned updates,reuses,bindings,active_peak,publications;
static C3D_LightLutDA fog_lut;
static unsigned fog_lut_ready,resident_fog,fog_bindings,fog_publications;
static float fog_parameters[2];
static float fog_curve(float distance,float unused0,float unused1){
    (void)unused0;(void)unused1;return exp2f(-8*distance);
}
void fh_pica_environment_init(void){
    C3D_Material material={.ambient={1,1,1},.diffuse={1,1,1},.specular0={1,1,1}};
    memset(valid,0,sizeof(valid));resident_slot=~0u;
    resident_fog=fog_lut_ready=fog_bindings=fog_publications=0;
    C3D_LightEnvInit(&environment);
    C3D_LightEnvMaterial(&environment,&material);
    C3D_LightEnvClampHighlights(&environment,false);
    /* Immutable curve prepared at initialization, never on a draw. */
    LightLutDA_Create(&fog_lut,fog_curve,0,1,0,0);
    for(unsigned i=0;i<8;i++){
        C3D_Light *l=&lights[i];
        if(C3D_LightInit(l,&environment)<0)fh_platform_panic("PICA environment light capacity");
        C3D_LightColor(l,0,0,0);C3D_LightEnable(l,false);
    }
}
void fh_pica_environment_update(unsigned slot,const FhPicaEnvironment *p){
    if(slot>=3 || !p || p->count>8)fh_platform_panic("PICA environment snapshot");
    if(valid[slot] && !memcmp(p,snapshots+slot,sizeof(*p))){reuses++;return;}
    updates++;if(p->count>active_peak)active_peak=p->count;
    snapshots[slot]=*p;valid[slot]=1;
    revisions[slot]++;
    if(resident_slot==slot)resident_slot=~0u; /* Also safe at revision wrap. */
}
static void publish(unsigned slot){
    static const FhPicaEnvironment dark;
    const FhPicaEnvironment *p=slot<3?snapshots+slot:&dark;
    if(resident_fog){
        C3D_LightSpecular0(lights+7,0,0,0);
        C3D_LightDistAttnEnable(lights+7,false);resident_fog=0;
    }
    /* Rcp mode 0 uses a white register material. With no selected lights,
     * modelLightChannels_applyGXControls disables lighting: that material
     * passes through, ignoring ambient. Slot 3 is our fog-only owner and
     * must remain dark; it is not an original environment-map slot. */
    unsigned unlit_material=slot<3 && !p->count;
    C3D_LightEnvAmbient(&environment,unlit_material?1:p->ambient[0],
        unlit_material?1:p->ambient[1],unlit_material?1:p->ambient[2]);
    for(unsigned i=0;i<8;i++){
        C3D_Light *l=&lights[i];unsigned enabled=i<p->count;
        /* The hardware encodes light count minus one. Supply a dark dummy
         * directional light when this slot is ambient-only. */
        C3D_LightEnable(l,enabled || (!p->count && !i));
        C3D_LightDiffuse(l,enabled?p->color[i][0]:0,enabled?p->color[i][1]:0,enabled?p->color[i][2]:0);
        C3D_FVec position=enabled?FVec4_New(p->direction[i][0],p->direction[i][1],p->direction[i][2],0):FVec4_New(0,0,1,0);
        C3D_LightPosition(l,&position);
    }
    resident_slot=slot;resident_revision=slot<3?revisions[slot]:0;publications++;
}
void fh_pica_environment_bind(unsigned slot,unsigned enabled){
    if(enabled && (slot>=3 || !valid[slot]))fh_platform_panic("PICA environment not published");
    /* Citro3D LightEnvBind does not dirty a different environment's light
     * registers. One SDK environment owns those registers; selecting a game
     * slot republishes its parameters explicitly, including disabled lights. */
    if(enabled && (resident_slot!=slot || resident_revision!=revisions[slot] || resident_fog))publish(slot);
    C3D_LightEnvBind(enabled?&environment:NULL);
    bindings+=enabled!=0;
    fh_pica_scene_environment(enabled);
}
unsigned fh_pica_environment_layered_available(unsigned slot,unsigned base,unsigned fog){
    if(!base)return 1;
    return slot<3 && valid[slot] && (!fog || snapshots[slot].count<8);
}
int fh_pica_environment_layered_bind(unsigned slot,unsigned base,unsigned fog,const float parameters[2]){
    if(!fh_pica_environment_layered_available(slot,base,fog))return 0;
    if(!fog){fh_pica_environment_bind(slot,base);return 1;}
    if(!parameters || !isfinite(parameters[0]) || !isfinite(parameters[1]) || parameters[0]<=0)
        fh_platform_panic("PICA fragment fog parameters");
    unsigned selected=base?slot:3;
    unsigned revision=base?revisions[slot]:0;
    if(resident_slot!=selected || resident_revision!=revision)publish(selected);
    if(!resident_fog){
        C3D_Light *light=lights+7;
        C3D_FVec position=FVec4_New(0,0,0,1);
        C3D_LightEnable(light,true);C3D_LightDiffuse(light,0,0,0);
        C3D_LightSpecular0(light,1,1,1);C3D_LightPosition(light,&position);
        if(!base || !snapshots[slot].count)C3D_LightEnable(lights,false);
        resident_fog=1;
        /* LUT data and distance uniforms survive ordinary directional binds.
         * Re-enable only the selector when its parameters are unchanged. */
        if(fog_lut_ready)C3D_LightDistAttnEnable(light,true);
    }
    if(!fog_lut_ready || memcmp(parameters,fog_parameters,sizeof(fog_parameters))){
        fog_lut.scale=parameters[0];fog_lut.bias=parameters[1];
        C3D_LightDistAttn(lights+7,&fog_lut);
        memcpy(fog_parameters,parameters,sizeof(fog_parameters));
        fog_lut_ready=1;fog_publications++;
    }
    C3D_LightEnvBind(&environment);fh_pica_scene_environment(base);
    bindings+=base!=0;fog_bindings++;return 1;
}
void fh_pica_environment_shutdown(void){
    char line[192];snprintf(line,sizeof(line),"PICA environment: updates=%u reuses=%u bindings=%u max_lights=%u state_bytes=%u publications=%u\n",
        updates,reuses,bindings,active_peak,(unsigned)(sizeof(environment)+sizeof(lights)+sizeof(snapshots)+sizeof(valid)+sizeof(revisions)+sizeof(resident_slot)+sizeof(resident_revision)),publications);
    fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA fragment fog lighting: draws=%u publications=%u cpu_bytes=%u reserved_light=7\n",fog_bindings,fog_publications,(unsigned)(sizeof(fog_lut)+sizeof(fog_parameters)+4*sizeof(unsigned)));fh_platform_log(line);
}
