/* Included by the framebuffer owner. Each authored call captures its current
 * input, including any earlier colour filter in the same frame. No temporal
 * cache, particle-image alias, CPU pixel processing or geometry rebuilding.
 * Same ordered split and lifetime rules as the native motion capture owner. */
#include "fh_pica_screen_filter.h"
#include "fh_pica_screen_color.h"
enum {SCREEN_FILTER_VRAM=512*256*4,SCREEN_FILTER_LINEAR=512};
static C3D_Tex screen_filter_image;
static C3D_RenderTarget *screen_filter_target;
static NativeSurfaceVertex *screen_filter_quads;
static C3D_AttrInfo screen_filter_attributes;
static C3D_BufInfo screen_filter_buffers[2];
static C3D_TexEnv screen_filter_stages[2][6];
static unsigned screen_filter_calls,screen_filter_failures;
static int screen_filter_prepare(unsigned requested){
    if(screen_filter_target)return 1;
    extern u32 __ctru_linear_heap_size;
    if(vramSpaceFree()<SCREEN_FILTER_VRAM+1024*1024 ||
       linearSpaceFree()<SCREEN_FILTER_LINEAR+__ctru_linear_heap_size*15/100)goto unavailable;
    if(!C3D_TexInitVRAM(&screen_filter_image,512,256,GPU_RGBA8))goto unavailable;
    screen_filter_quads=linearMemAlign(SCREEN_FILTER_LINEAR,128);
    if(!screen_filter_quads)goto unavailable;
    /* Last fallible allocation. Never delete a target during an open frame. */
    screen_filter_target=C3D_RenderTargetCreateFromTex(&screen_filter_image,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    if(!screen_filter_target)goto unavailable;
    C3D_TexSetFilter(&screen_filter_image,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&screen_filter_image,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    memset(screen_filter_quads,0,SCREEN_FILTER_LINEAR);
    const unsigned corner[6]={0,1,2,0,2,3};const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        float x=xy[corner[i]][0],y=xy[corner[i]][1];
        screen_filter_quads[i]=(NativeSurfaceVertex){.position={2*x-1,1-2*y,-.5f,1},.color={1,1,1,1},
            .uv={(239.5f-239*y)/256.f,(112.5f+399*x)/512.f}};
        screen_filter_quads[i+6]=(NativeSurfaceVertex){.position={1-2*y,1-2*x,-.5f,1},.color={1,1,1,1},.uv={x,y}};
    }
    fh_gpu_cache_flush(screen_filter_quads,SCREEN_FILTER_LINEAR);
    AttrInfo_Init(&screen_filter_attributes);AttrInfo_AddLoader(&screen_filter_attributes,0,GPU_FLOAT,4);
    AttrInfo_AddLoader(&screen_filter_attributes,1,GPU_FLOAT,4);AttrInfo_AddLoader(&screen_filter_attributes,2,GPU_FLOAT,2);
    for(unsigned i=0;i<2;i++){
        BufInfo_Init(screen_filter_buffers+i);
        if(BufInfo_Add(screen_filter_buffers+i,screen_filter_quads+i*6,sizeof(*screen_filter_quads),3,0x210)<0)
            fh_platform_panic("Screen filter buffer");
    }
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(screen_filter_stages[0]+i);
    C3D_TexEnvSrc(screen_filter_stages[0],C3D_Both,GPU_TEXTURE0,0,0);
    fh_pica_screen_color_prepare(screen_filter_stages[1]);return 1;
unavailable:
    if(screen_filter_image.data)C3D_TexDelete(&screen_filter_image);
    memset(&screen_filter_image,0,sizeof(screen_filter_image));
    linearFree(screen_filter_quads);screen_filter_quads=NULL;
    screen_filter_failures++;fh_graphics_effects_reject(requested);return 0;
}
static void screen_filter_state(unsigned mode,C3D_TexEnv stages[6]){
    C3D_BindProgram(&ui_program);C3D_LightEnvBind(NULL);
    C3D_SetAttrInfo(&screen_filter_attributes);C3D_SetBufInfo(screen_filter_buffers+mode);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,1,1,0,0);C3D_FVUnifSet(GPU_VERTEX_SHADER,1,1,1,0,0);
    fh_native_fog_apply(0);C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,stages+i);
    C3D_TexBind(0,mode?&screen_filter_image:&native_surface_color);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
    fh_pica_texture2_coord1(0);
}
static void screen_filter_publish(unsigned mode){
    screen_filter_state(mode,screen_filter_stages[mode]);C3D_DrawArrays(GPU_TRIANGLES,0,6);
}
void fh_pica_screen_filter(const unsigned char modulation[3]){
    if(!initialized || !(fh_graphics_effects&FH_EFFECT_SCREEN_COLOR) ||
       (fh_effects_unavailable_mask&FH_EFFECT_SCREEN_COLOR) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER))return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    fh_minimal_auxiliary_begin();
    if(!screen_filter_prepare(FH_EFFECT_SCREEN_COLOR)){fh_minimal_auxiliary_end(0,0);return;}
    fh_pica_screen_color_parameters(screen_filter_stages[1],modulation);
    native_surface_split();C3D_FrameDrawOn(screen_filter_target);screen_filter_publish(0);
    native_surface_split();native_surface_draw_on(target);screen_filter_publish(1);
    screen_filter_calls++;fh_pica_fog_invalidate();fh_minimal_auxiliary_end(2,12);
}
static void screen_filter_destroy(void){
    /* Called only at renderer shutdown, after all GPU readers have retired. */
    if(screen_filter_target)C3D_RenderTargetDelete(screen_filter_target);
    if(screen_filter_image.data)C3D_TexDelete(&screen_filter_image);
    linearFree(screen_filter_quads);screen_filter_quads=NULL;screen_filter_target=NULL;
    memset(&screen_filter_image,0,sizeof(screen_filter_image));
    char line[128];snprintf(line,sizeof(line),"PICA screen colour: calls=%u failures=%u\n",screen_filter_calls,screen_filter_failures);
    fh_platform_log(line);
}
