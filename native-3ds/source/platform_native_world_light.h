/* Original per-list light choices, PICA fragment distance attenuation.
 * The allowed detail reduction replaces separable view-XZ/view-Y falloff with
 * isotropic linear distance falloff. Original post-texture combiners remain.
 * Specular0 with distribution/geometric/highlight factors disabled is a
 * normal-independent RGB accumulator; no surface-normal approximation.
 * Pinned Citro3D light.c/lightenv.c define the public conf/dirty-bit contract.
 */
#include <math.h>
static C3D_LightEnv world_light_env;
static C3D_Light world_lights[2];
static C3D_LightLutDA world_light_lut;
static unsigned world_light_ready,world_light_draws,world_light_selected;
static float world_light_falloff(float distance,float a,float b){
    (void)a;(void)b;return fmaxf(0,1-distance);
}
static void world_light_bind(const FhMinimalWorldLights *m){
    if(!m || !m->count){C3D_LightEnvBind(NULL);return;}
    if(!world_light_ready){
        C3D_LightEnvInit(&world_light_env);
        const C3D_Material material={.specular0={1,1,1}};
        C3D_LightEnvMaterial(&world_light_env,&material);
        C3D_LightEnvClampHighlights(&world_light_env,false);
        LightLutDA_Create(&world_light_lut,world_light_falloff,0,1,0,0);
        for(unsigned i=0;i<2;i++){
            if(C3D_LightInit(world_lights+i,&world_light_env)!=(int)i)
                fh_platform_panic("Native world light initialization");
            C3D_LightDistAttn(world_lights+i,&world_light_lut);
        }
        world_light_ready=1;
    }
    for(unsigned i=0;i<2;i++){
        C3D_Light *light=world_lights+i;
        C3D_LightEnable(light,i<m->count);
        if(i>=m->count)continue;
        const FhMinimalWorldLight *source=m->light+i;
        C3D_FVec position=FVec4_New(source->position[0],source->position[1],source->position[2],1);
        C3D_LightPosition(light,&position);
        C3D_LightSpecular0(light,source->color[0]/255.f,source->color[1]/255.f,source->color[2]/255.f);
        /* Only the coordinate scale changes. Reusing the immutable LUT avoids
         * uploading its 1KiB again for every radius/selected-light change.
         * APT restore uses Citro3D's normal LUT dirty/restore mechanism. */
        u32 scale=f32tof20(1/fmaxf(source->radius,.1f));
        if(light->conf.distAttnScale!=scale){
            light->conf.distAttnScale=scale;light->flags|=C3DF_Light_Dirty;
        }
    }
    C3D_LightEnvBind(&world_light_env);
    world_light_draws++;world_light_selected+=m->count;
}
static void world_light_env_stages(const FhMinimalWorldLights *m){
    if(!m->count && !m->mode)return;
    unsigned light=m->count?GPU_FRAGMENT_SECONDARY_COLOR:GPU_CONSTANT;
    C3D_TexEnv *e=C3D_GetTexEnv(3);
    C3D_TexEnvColor(e,0);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,light,0);
    C3D_TexEnvFunc(e,C3D_RGB,m->mode?GPU_MODULATE:GPU_ADD);
    if(m->mode!=2)return;
    /* Preserve B from stage2 for stage4 and B*L from stage3 for stage5.
     * PICA's combiner buffer has a one-stage delay. Alpha always passes through.
     * Result = mix(clamp(B+L), clamp(B*L), original raster alpha). */
    C3D_TexEnvBufUpdate(C3D_RGB,(1u<<2)|(1u<<3));
    e=C3D_GetTexEnv(4);C3D_TexEnvColor(e,0);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS_BUFFER,light,0);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_ADD);
    e=C3D_GetTexEnv(5);
    C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS_BUFFER,GPU_PREVIOUS,GPU_PRIMARY_COLOR);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
}
