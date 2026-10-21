#ifndef FH_GRAPHICS_OPTIONS_H
#define FH_GRAPHICS_OPTIONS_H
#include <stdint.h>
/* Existing replay masks 0/1 retain their meaning. New disable bits default
 * clear; FPS is a presentation option and never changes a replay's mask. */
enum { FH_GRAPHICS_PARTICLES=1, FH_GRAPHICS_NO_LIGHTING=2,
       FH_GRAPHICS_NO_ENVIRONMENT=4, FH_GRAPHICS_NO_RENDER=8, FH_GRAPHICS_NO_WORLD=16,
       FH_GRAPHICS_NO_ACTORS=32, FH_GRAPHICS_NO_WORLD_TEX=64,
       FH_GRAPHICS_NO_ACTOR_TEX=128, FH_GRAPHICS_NO_HUD=256,
       FH_GRAPHICS_NO_HOLOGRAM=512, FH_GRAPHICS_NO_SKY=1024,
       FH_GRAPHICS_NO_HUD_TEX=2048, FH_GRAPHICS_NO_PARTICLE_TEX=4096,
       FH_GRAPHICS_NO_MOVIES=8192, FH_GRAPHICS_NO_GROUND_SHADOWS=16384,
       FH_GRAPHICS_NO_DISTANCE_FOG=32768,
       FH_GRAPHICS_SILHOUETTE_SHADOWS=65536,
       FH_GRAPHICS_SPLASHES=131072,
       FH_GRAPHICS_NO_MORPHS=262144,
       FH_GRAPHICS_WATER_DROPS=524288,
       FH_GRAPHICS_FUR=1048576,
       FH_GRAPHICS_SPECULAR=2097152,
       FH_GRAPHICS_SOURCE_FUR=4194304,
       FH_GRAPHICS_DIFFUSE_MAPS=8388608,
       FH_GRAPHICS_HEIGHT_FOG=16777216,
       FH_GRAPHICS_WEATHER=33554432,
       FH_GRAPHICS_FUR_GROOM=67108864,
       FH_GRAPHICS_SPLASH_REFLECTION=134217728,
       FH_GRAPHICS_MOTION_BLUR=268435456,
       FH_GRAPHICS_BLUR_FILTER=536870912,
       FH_GRAPHICS_HEAT=1073741824,
       FH_GRAPHICS_WAKES=0x80000000u,
       FH_GRAPHICS_ALL=1, FH_GRAPHICS_VALID=0xffffffffu,
       /* Contact receiver queries still exceed the cave CPU budget. Normal
        * gameplay exposes the effect but requires an explicit user opt-in. */
       FH_GRAPHICS_DEFAULT=FH_GRAPHICS_ALL|FH_GRAPHICS_NO_GROUND_SHADOWS };
extern unsigned fh_graphics_options;
/* Separate extensible word: do not renumber the existing replay/settings bits. */
enum {FH_EFFECT_RIPPLES=1,FH_EFFECT_RIPPLE_FAST=2,FH_EFFECT_PARTICLE_REFLECTION=4,FH_EFFECT_SCREEN_COLOR=8,FH_EFFECT_SPIRIT_VISION=16,FH_EFFECT_DISTORTION=32,FH_EFFECT_VALID=63};
extern unsigned fh_graphics_effects,fh_effects_unavailable_mask,fh_effects_unavailable_seen;
void fh_graphics_effects_set(unsigned mask);
void fh_graphics_effects_reject(unsigned mask);
void fh_graphics_gpu_quarantine(void);
void fh_graphics_quarantine_reload(void);
/* Requested settings remain intact. Unavailable is session-only, clears when
 * that option is turned OFF. Seen retains failures for honest replay results. */
extern unsigned fh_graphics_unavailable_mask,fh_graphics_unavailable_seen;
void fh_graphics_options_reject(unsigned mask);
extern unsigned fh_graphics_fps_visible;
extern unsigned fh_graphics_original_requested,fh_graphics_original_active,fh_graphics_original_available;
extern unsigned fh_graphics_textures_requested,fh_graphics_textures_active;
extern unsigned fh_graphics_perf_visible,fh_graphics_gpu_visible;
void fh_graphics_geometry_init(void);
/* Ordinary gameplay only; replay recipes remain authoritative and isolated. */
void fh_graphics_settings_load(const char *root);
int fh_graphics_settings_save(void);
void fh_graphics_options_frame(uint64_t ticks,uint64_t ticks_per_second);
void fh_graphics_options_set(unsigned mask);
void fh_graphics_options_touch(unsigned held,int x,int y,unsigned locked);
unsigned fh_graphics_options_draw(uint16_t *pixels,unsigned initial);
#endif
