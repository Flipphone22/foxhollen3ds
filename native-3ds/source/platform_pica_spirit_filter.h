/* Shared scratch, freshly captured at each original source call. Included after
 * screen colour owner. Only fragment work differs; geometry stays immutable. */
#include "fh_pica_spirit_filter.h"
#include "fh_pica_spirit_material.h"
static C3D_TexEnv spirit_stages[6],spirit_final[3];
static unsigned spirit_ready,spirit_calls;
unsigned fh_pica_spirit_filter_enabled(void){
    return initialized && (fh_graphics_effects&FH_EFFECT_SPIRIT_VISION) &&
        !(fh_effects_unavailable_mask&FH_EFFECT_SPIRIT_VISION) && !(fh_graphics_options&FH_GRAPHICS_NO_RENDER);
}
void fh_pica_spirit_filter(const unsigned coefficient[3],unsigned bias){
    if(!fh_pica_spirit_filter_enabled())return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    fh_minimal_auxiliary_begin();
    if(!screen_filter_prepare(FH_EFFECT_SPIRIT_VISION)){fh_minimal_auxiliary_end(0,0);return;}
    if(!spirit_ready){
        for(unsigned c=0;c<3;c++){fh_pica_spirit_material_prepare(spirit_stages,c);spirit_final[c]=spirit_stages[5];}
        spirit_stages[5]=spirit_final[0];spirit_ready=1;
    }
    fh_pica_spirit_material_parameters(spirit_stages,coefficient,bias);
    native_surface_split();C3D_FrameDrawOn(screen_filter_target);screen_filter_publish(0);
    native_surface_split();native_surface_draw_on(target);
    screen_filter_state(1,spirit_stages);C3D_TexEnvBufUpdate(C3D_RGB,1);
    static const unsigned masks[3]={GPU_WRITE_RED|GPU_WRITE_ALPHA,GPU_WRITE_GREEN,GPU_WRITE_BLUE};
    for(unsigned c=0;c<3;c++){
        if(c)C3D_SetTexEnv(5,spirit_final+c);
        C3D_DepthTest(false,GPU_ALWAYS,masks[c]);C3D_DrawArrays(GPU_TRIANGLES,0,6);
    }
    /* Subsequent consumers must never inherit this delayed combiner buffer. */
    C3D_TexEnvBufUpdate(C3D_Both,0);
    spirit_calls++;fh_pica_fog_invalidate();fh_minimal_auxiliary_end(4,24);
}
static void spirit_filter_report(void){
    char line[96];snprintf(line,sizeof(line),"PICA spirit vision: calls=%u\n",spirit_calls);fh_platform_log(line);
}
