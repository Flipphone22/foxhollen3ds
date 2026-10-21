#include <stdio.h>
/* Dedicated persistent image: water's on-demand snapshot must not replace the
 * source reflection image between its publication and particle consumption.
 * Existing auxiliary begin/end drains producers and invalidates render state.
 * No CPU pixel readback, extra geometry generation or second scene traversal. */
static C3D_Tex glow_reflection_image;
static C3D_RenderTarget *glow_reflection_target;
static unsigned glow_reflection_valid;
static unsigned glow_capture_requests,glow_capture_disabled,glow_capture_completed,glow_capture_failed;
static u64 glow_capture_prepare_ticks,glow_capture_draw_ticks;

static int glow_reflection_enabled(void){
    return initialized && (fh_graphics_effects&FH_EFFECT_PARTICLE_REFLECTION) &&
        !(fh_effects_unavailable_mask&FH_EFFECT_PARTICLE_REFLECTION) &&
        (fh_graphics_options&FH_GRAPHICS_PARTICLES) &&
        !(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_PARTICLE_TEX));
}
void fh_pica_glow_reflection_capture(void){
    glow_capture_requests++;
    if(!glow_reflection_enabled()){glow_capture_disabled++;glow_reflection_valid=0;return;}
    u64 started=svcGetSystemTick();
    fh_minimal_auxiliary_begin();
    if(!glow_reflection_target){
        extern u32 __ctru_linear_heap_size;
        if(vramSpaceFree()<32768+1024*1024 ||
           linearSpaceFree()<6*sizeof(NativeSurfaceVertex)+__ctru_linear_heap_size*15/100 ||
           !native_surface_prepare_snapshot(&glow_reflection_image,&glow_reflection_target)){
            glow_reflection_valid=0;
            fh_minimal_auxiliary_end(0,0);
            glow_capture_failed++;glow_capture_prepare_ticks+=svcGetSystemTick()-started;
            fh_graphics_effects_reject(FH_EFFECT_PARTICLE_REFLECTION);return;
        }
    }
    glow_capture_prepare_ticks+=svcGetSystemTick()-started;
    started=svcGetSystemTick();
    native_surface_capture_into(target,&ui_program,&glow_reflection_image,glow_reflection_target);
    glow_reflection_valid=1;
    fh_minimal_auxiliary_end(1,6);
    glow_capture_completed++;glow_capture_draw_ticks+=svcGetSystemTick()-started;
}
const void *fh_pica_glow_reflection_image(void){
    return glow_reflection_enabled()&&glow_reflection_valid?&glow_reflection_image:NULL;
}
static void glow_reflection_capture_destroy(void){
    /* Same shutdown point as native_surface_destroy: all queued readers retired. */
    if(glow_reflection_target)C3D_RenderTargetDelete(glow_reflection_target);
    if(glow_reflection_image.data)C3D_TexDelete(&glow_reflection_image);
    memset(&glow_reflection_image,0,sizeof(glow_reflection_image));
    glow_reflection_target=NULL;glow_reflection_valid=0;
    /* Wall-clock submission/retirement time, possibly including waits. These
     * are NOT GPU execution cycles, and must not be added to overlapping frame
     * buckets. Disabled requests never read the system tick or issue commands. */
    char line[256];snprintf(line,sizeof(line),
        "PICA particle capture: requests=%u disabled=%u completed=%u failed=%u prepare_wall_ticks=%llu draw_wall_ticks=%llu gpu_execution_measured=0\n",
        glow_capture_requests,glow_capture_disabled,glow_capture_completed,glow_capture_failed,
        (unsigned long long)glow_capture_prepare_ticks,(unsigned long long)glow_capture_draw_ticks);
    fh_platform_log(line);
}
