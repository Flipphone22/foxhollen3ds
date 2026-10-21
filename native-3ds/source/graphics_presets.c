#include "fh_graphics_presets.h"
static const FhGraphicsPreset presets[]={
    {"n3ds-fancy",FH_GRAPHICS_PARTICLES|FH_GRAPHICS_SPLASHES|FH_GRAPHICS_WATER_DROPS|
        FH_GRAPHICS_FUR|FH_GRAPHICS_SOURCE_FUR|FH_GRAPHICS_FUR_GROOM|FH_GRAPHICS_SPECULAR|
        FH_GRAPHICS_DIFFUSE_MAPS|FH_GRAPHICS_HEIGHT_FOG|FH_GRAPHICS_WEATHER|FH_GRAPHICS_SILHOUETTE_SHADOWS|
        FH_GRAPHICS_SPLASH_REFLECTION|FH_GRAPHICS_WAKES|FH_GRAPHICS_MOTION_BLUR,1,1,
        FH_EFFECT_PARTICLE_REFLECTION|FH_EFFECT_SCREEN_COLOR|FH_EFFECT_SPIRIT_VISION},
    {"n3ds-fast",FH_GRAPHICS_SPECULAR|FH_GRAPHICS_PARTICLES|FH_GRAPHICS_WEATHER|FH_GRAPHICS_DIFFUSE_MAPS|FH_GRAPHICS_NO_GROUND_SHADOWS,0,0},
    {"o3ds-fancy",FH_GRAPHICS_SPECULAR|FH_GRAPHICS_PARTICLES|FH_GRAPHICS_WEATHER|FH_GRAPHICS_DIFFUSE_MAPS|
        FH_GRAPHICS_HEIGHT_FOG|FH_GRAPHICS_SPLASHES|FH_GRAPHICS_NO_GROUND_SHADOWS,1,0},
    {"o3ds-fast",FH_GRAPHICS_SPECULAR|FH_GRAPHICS_PARTICLES|FH_GRAPHICS_NO_GROUND_SHADOWS,0,0},
    {"Custom",0,0,0}
};
const FhGraphicsPreset*fh_graphics_preset(unsigned i){return i<FH_PRESET_COUNT?presets+i:0;}
unsigned fh_graphics_preset_match(unsigned mask,unsigned geometry,unsigned textures){
    for(unsigned i=0;i<FH_PRESET_CUSTOM;i++)if(mask==presets[i].mask && geometry==presets[i].original_geometry && textures==presets[i].original_textures && fh_graphics_effects==presets[i].effects)return i;
    return FH_PRESET_CUSTOM;
}
int fh_graphics_preset_apply(unsigned i){
    if(i>=FH_PRESET_COUNT)return 0;
    if(i==FH_PRESET_CUSTOM)return 1;
    fh_graphics_options_set(presets[i].mask);fh_graphics_effects_set(presets[i].effects);
    fh_graphics_original_requested=presets[i].original_geometry;
    fh_graphics_textures_requested=presets[i].original_textures;
    return 1;
}
