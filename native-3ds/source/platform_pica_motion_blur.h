/* Included by the native framebuffer owner. Source temporal ordering is
 * blend(previous capture) then capture(current result). Never sample the
 * active render target; ordered splits publish both target transitions. */
#include "fh_pica_motion_blur.h"
#include "fh_pica_distortion.h"
#include "fh_pica_fog.h"
extern void fh_minimal_auxiliary_begin(void);
extern void fh_minimal_auxiliary_end(unsigned draws,unsigned indices);
/* RGBA history shares its alpha with source-timed depth when blur is enabled.
 * Fixed format avoids reallocating a live temporal image when toggles change. */
enum {MOTION_WIDTH=512,MOTION_HEIGHT=256,MOTION_VRAM_BYTES=512*256*4,MOTION_LINEAR_BYTES=512};
static C3D_Tex motion_history;
static C3D_RenderTarget *motion_target;
static NativeSurfaceVertex *motion_quads;
static C3D_AttrInfo motion_attributes;
static C3D_BufInfo motion_buffers[2];
static C3D_TexEnv motion_stages[2][6];
static unsigned motion_valid,motion_captures,motion_draws,motion_unseeded,motion_disabled;
static u64 motion_ticks;
static unsigned blur_capture(unsigned capture);
static unsigned motion_alpha(float value){
    /* All ordinary source values in [0,1] retain its truncating byte cast.
     * Reject nonpositive/NaN; saturate malformed values before conversion. */
    return !(value>0.f)?0:value>=1.f?255:(unsigned)(255.f*value);
}
static int motion_prepare(void){
    if(motion_target)return 1;
    extern u32 __ctru_linear_heap_size;
    if(vramSpaceFree()<MOTION_VRAM_BYTES+1024*1024 ||
       linearSpaceFree()<MOTION_LINEAR_BYTES+__ctru_linear_heap_size*15/100)
        goto unavailable;
    if(!C3D_TexInitVRAM(&motion_history,MOTION_WIDTH,MOTION_HEIGHT,GPU_RGBA8))
        goto unavailable;
    motion_quads=linearMemAlign(MOTION_LINEAR_BYTES,128);
    if(!motion_quads)goto unavailable;
    /* Target creation is the final allocation: SDK deletion is illegal in an
     * open frame. Failure before publication can release these new buffers. */
    motion_target=C3D_RenderTargetCreateFromTex(&motion_history,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    if(!motion_target)goto unavailable;
    memset(motion_quads,0,MOTION_LINEAR_BYTES);
    C3D_TexSetFilter(&motion_history,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&motion_history,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    const unsigned corner[6]={0,1,2,0,2,3};
    const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        float x=xy[corner[i]][0],y=xy[corner[i]][1];
        /* Capture unrotates the native padded framebuffer into the history.
         * Display rotates it back. Source viewport and texel centres match
         * the audited native surface capture; neither quad changes at draw. */
        motion_quads[i]=(NativeSurfaceVertex){.position={2*x-1,1-2*y,-.5f,1},
            .color={1,1,1,1},.uv={240.f*(1-y)/256.f,1.f-400.f*(1-x)/512.f}};
        motion_quads[i+6]=(NativeSurfaceVertex){.position={1-2*y,1-2*x,-.5f,1},
            .color={1,1,1,1},.uv={x,y}};
    }
    fh_gpu_cache_flush(motion_quads,MOTION_LINEAR_BYTES);
    AttrInfo_Init(&motion_attributes);
    AttrInfo_AddLoader(&motion_attributes,0,GPU_FLOAT,4);
    AttrInfo_AddLoader(&motion_attributes,1,GPU_FLOAT,4);
    AttrInfo_AddLoader(&motion_attributes,2,GPU_FLOAT,2);
    for(unsigned mode=0;mode<2;mode++){
        BufInfo_Init(motion_buffers+mode);
        if(BufInfo_Add(motion_buffers+mode,motion_quads+mode*6,sizeof(*motion_quads),3,0x210)<0)
            fh_platform_panic("Motion blur immutable buffers");
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(motion_stages[mode]+i);
        C3D_TexEnvSrc(motion_stages[mode],C3D_RGB,GPU_TEXTURE0,0,0);
        C3D_TexEnvFunc(motion_stages[mode],C3D_RGB,GPU_REPLACE);
        C3D_TexEnvSrc(motion_stages[mode],C3D_Alpha,mode?GPU_CONSTANT:GPU_TEXTURE0,0,0);
        C3D_TexEnvFunc(motion_stages[mode],C3D_Alpha,GPU_REPLACE);
    }
    return 1;
unavailable:
    if(motion_history.data)C3D_TexDelete(&motion_history);
    memset(&motion_history,0,sizeof(motion_history));
    linearFree(motion_quads);motion_quads=NULL;
    fh_graphics_options_reject(FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT);
    fh_graphics_effects_reject(FH_EFFECT_DISTORTION);
    return 0;
}
static void motion_publish(unsigned mode,unsigned alpha){
    C3D_BindProgram(&ui_program);C3D_LightEnvBind(NULL);
    C3D_SetAttrInfo(&motion_attributes);C3D_SetBufInfo(motion_buffers+mode);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,1,1,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,1,1,1,0,0);
    fh_native_fog_apply(0);
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,mode?GPU_SRC_ALPHA:GPU_ONE,
        mode?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,mode?GPU_ZERO:GPU_ONE,mode?GPU_ONE:GPU_ZERO);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,motion_stages[mode]+i);
    if(mode)C3D_TexEnvColor(C3D_GetTexEnv(0),(u32)alpha<<24);
    C3D_TexBind(0,mode?&motion_history:&native_surface_color);
    C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
    fh_pica_texture2_coord1(0);
    C3D_DrawArrays(GPU_TRIANGLES,0,6);
}
void fh_pica_motion_blur_step(float alpha,unsigned requested,unsigned capture){
    blur_capture(0); /* OFF invalidates depth even when no colour work is needed. */
    unsigned admitted=fh_graphics_options&~fh_graphics_unavailable_mask;
    if(!initialized || !(admitted&(FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT)) ||
       (fh_graphics_options&FH_GRAPHICS_NO_RENDER)){
        motion_valid=0;motion_disabled++;return;
    }
    unsigned opacity=(admitted&FH_GRAPHICS_MOTION_BLUR)?motion_alpha(alpha):0,blend=requested&&opacity&&motion_valid;
    if(requested&&opacity&&!motion_valid)motion_unseeded++;
    if(!blend&&!capture)return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 started=svcGetSystemTick();if(!motion_prepare())return;
    fh_minimal_auxiliary_begin();
    unsigned draws=0;
    if(blend){native_surface_draw_on(target);motion_publish(1,opacity);motion_draws++;draws++;}
    if(capture){
        /* A combined colour/depth capture replaces, rather than follows, the
         * ordinary colour draw. Both keep the original capture boundary. */
        if(!blur_capture(1)){
            native_surface_split();C3D_FrameDrawOn(motion_target);
            motion_publish(0,255);
            native_surface_split();native_surface_draw_on(target);
        }
        motion_valid=1;motion_captures++;draws++;
    }
    fh_pica_fog_invalidate();fh_minimal_auxiliary_end(draws,draws*6);
    motion_ticks+=svcGetSystemTick()-started;
}
static void motion_shutdown(void){
    /* The enclosing renderer has already joined every GPU reader. */
    if(motion_target)C3D_RenderTargetDelete(motion_target);
    if(motion_history.data)C3D_TexDelete(&motion_history);
    memset(&motion_history,0,sizeof(motion_history));
    linearFree(motion_quads);motion_quads=NULL;motion_target=NULL;motion_valid=0;
    char line[240];snprintf(line,sizeof(line),
        "PICA motion blur: captures=%u draws=%u unseeded=%u disabled=%u vram_peak=%u linear_peak=%u submit_ticks=%llu CPU_pixels=0\n",
        motion_captures,motion_draws,motion_unseeded,motion_disabled,
        motion_captures?MOTION_VRAM_BYTES:0,motion_captures?MOTION_LINEAR_BYTES:0,motion_ticks);
    fh_platform_log(line);
}
