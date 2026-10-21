/* Included after the shared motion/capture owner. A single draw captures RGB
 * and depth alpha into shared history at the original source boundary. */
#include "fh_pica_blur_filter.h"
#include "fh_pica_heat.h"
#include "fh_pica_blur_material.h"
#include "fh_pica_depth_view.h"
#include "post_sample_shader.h"
/* Independent RGBA8 scratch retains full 8-bit partial colour precision.
 * blur_depth borrows the history, not the scratch destination. The partial
 * pass never samples its destination; final sampling follows target switch. */
enum {BLUR_VRAM_BYTES=512*256*4,BLUR_LINEAR_BYTES=512};
static C3D_Tex blur_depth,blur_partial;
static void *blur_depth_copy;
static C3D_RenderTarget *blur_partial_target;
static NativeSurfaceVertex *blur_quads;
static C3D_AttrInfo blur_attributes;
static C3D_BufInfo blur_buffers[2];
static C3D_TexEnv blur_depth_stages[6];
static FhPicaBlurMaterial blur_material;
static shaderProgram_s blur_program;
static DVLB_s *blur_binary;
static unsigned blur_valid,blur_captures,blur_requests,blur_unseeded,blur_draws;
static u64 blur_capture_ticks,blur_draw_ticks;
unsigned fh_pica_blur_filter_enabled(void){
    return initialized && (fh_graphics_options&~fh_graphics_unavailable_mask&FH_GRAPHICS_BLUR_FILTER) &&
        !(fh_graphics_options&FH_GRAPHICS_NO_RENDER);
}
static int blur_capture_prepare(void){
    if(blur_quads)return 1;
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<BLUR_LINEAR_BYTES+__ctru_linear_heap_size*15/100)
        goto unavailable;
    blur_quads=linearMemAlign(BLUR_LINEAR_BYTES,128);
    if(!blur_quads)goto unavailable;
    blur_binary=DVLB_ParseFile((u32*)post_sample_shader,sizeof(post_sample_shader));
    if(!blur_binary)goto unavailable;
    shaderProgramInit(&blur_program);
    if(R_FAILED(shaderProgramSetVsh(&blur_program,blur_binary->DVLE)))goto unavailable;
    memset(blur_quads,0,BLUR_LINEAR_BYTES);
    const unsigned corner[6]={0,1,2,0,2,3};const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        float x=xy[corner[i]][0],y=xy[corner[i]][1];
        blur_quads[i]=(NativeSurfaceVertex){.position={2*x-1,1-2*y,-.5f,1},.color={1,1,1,1},.uv={x,y}};
        blur_quads[i+6]=(NativeSurfaceVertex){.position={1-2*y,1-2*x,-.5f,1},.color={1,1,1,1},.uv={x,y}};
    }
    fh_gpu_cache_flush(blur_quads,BLUR_LINEAR_BYTES);
    AttrInfo_Init(&blur_attributes);AttrInfo_AddLoader(&blur_attributes,0,GPU_FLOAT,4);
    AttrInfo_AddLoader(&blur_attributes,1,GPU_FLOAT,4);AttrInfo_AddLoader(&blur_attributes,2,GPU_FLOAT,2);
    for(unsigned i=0;i<2;i++){
        BufInfo_Init(blur_buffers+i);
        if(BufInfo_Add(blur_buffers+i,blur_quads+i*6,sizeof(*blur_quads),3,0x210)<0)fh_platform_panic("Blur filter buffers");
    }
    fh_pica_blur_material_init(&blur_material,1);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(blur_depth_stages+i);
    C3D_TexEnvSrc(blur_depth_stages,C3D_RGB,GPU_TEXTURE0,0,0);
    C3D_TexEnvSrc(blur_depth_stages,C3D_Alpha,GPU_TEXTURE1,0,0);
    C3D_TexEnvOpAlpha(blur_depth_stages,GPU_TEVOP_A_SRC_G,0,0);
    return 1;
unavailable:
    if(blur_binary){shaderProgramFree(&blur_program);DVLB_Free(blur_binary);blur_binary=NULL;}
    if(blur_partial.data)C3D_TexDelete(&blur_partial);
    memset(&blur_partial,0,sizeof(blur_partial));
    memset(&blur_depth,0,sizeof(blur_depth));
    linearFree(blur_quads);blur_quads=NULL;
    fh_graphics_options_reject(FH_GRAPHICS_BLUR_FILTER|FH_GRAPHICS_HEAT);
    fh_graphics_effects_reject(FH_EFFECT_DISTORTION);
    return 0;
}
static int blur_prepare(void){
    if(blur_partial_target)return 1;
    if(!blur_capture_prepare())return 0;
    if(vramSpaceFree()<BLUR_VRAM_BYTES+1024*1024)goto unavailable;
    if(!C3D_TexInitVRAM(&blur_partial,512,256,GPU_RGBA8))goto unavailable;
    C3D_TexSetFilter(&blur_partial,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&blur_partial,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    blur_partial_target=C3D_RenderTargetCreateFromTex(&blur_partial,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    if(blur_partial_target)return 1;
unavailable:
    if(blur_partial.data)C3D_TexDelete(&blur_partial);
    memset(&blur_partial,0,sizeof(blur_partial));
    fh_graphics_options_reject(FH_GRAPHICS_BLUR_FILTER);return 0;
}
static void blur_state(unsigned mode,unsigned depth_capture){
    C3D_BindProgram(&blur_program);C3D_LightEnvBind(NULL);
    C3D_SetAttrInfo(&blur_attributes);C3D_SetBufInfo(blur_buffers+(mode!=0));
    fh_native_fog_apply(0);C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);
    C3D_DepthTest(false,GPU_ALWAYS,depth_capture?GPU_WRITE_COLOR:
        (GPU_WRITE_RED|GPU_WRITE_GREEN|GPU_WRITE_BLUE));
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,mode?GPU_SRC_ALPHA:GPU_ONE,
        mode?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,depth_capture?GPU_ONE:GPU_ZERO,
        depth_capture?GPU_ZERO:GPU_ONE);
    C3D_TexEnvBufUpdate(C3D_Both,0);if(mode)C3D_TexEnvBufUpdate(C3D_Alpha,1);
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,depth_capture?blur_depth_stages+i:blur_material.stages[mode]+i);
    for(unsigned i=0;i<3;i++){
        float dx=mode||depth_capture?0:i==1?-1.f/320:i==2?1.f/320:0;
        float dy=mode||depth_capture?0:i==0?1.f/240:0;
        C3D_FVUnifSet(GPU_VERTEX_SHADER,2*i,1,0,dx,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,2*i+1,0,-1,1+dy,0);
    }
    if(depth_capture){
        C3D_FVUnifSet(GPU_VERTEX_SHADER,0,0,-240.f/256,240.f/256,0);
        /* The UI capture shader used 1-v; express that final texture-space
         * mapping directly in this affine program, for both source views. */
        C3D_FVUnifSet(GPU_VERTEX_SHADER,1,-400.f/512,0,400.f/512,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,2,0,-240.f/256,240.f/256,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,3,-400.f/512,0,400.f/512,0);
    }
    fh_pica_texture2_coord1(0);
}
/* Called within the shared owner's reservation. Returns one when the ordinary
 * colour capture was replaced. The parent accounts that single draw once. */
static unsigned blur_capture(unsigned capture){
    if(!fh_pica_blur_filter_enabled() && !fh_pica_heat_enabled() && !fh_pica_distortion_enabled()){blur_valid=0;return 0;}
    if(!capture)return 0;
    u64 started=svcGetSystemTick();
    if(!blur_capture_prepare())return 0;
    if(fh_pica_blur_filter_enabled() && !blur_prepare() && !fh_pica_heat_enabled() && !fh_pica_distortion_enabled())return 0;
    C3D_Tex view;
    if(!fh_pica_depth_view(&view,&target->frameBuf,&native_surface_color))fh_platform_panic("Blur filter depth layout");
    if(!blur_depth_copy)blur_depth_copy=vramAllocAt(view.size,VRAM_ALLOC_A);
    if(!blur_depth_copy)return 0;
    native_surface_split();
    C3D_SyncTextureCopy((u32*)view.data,0,(u32*)blur_depth_copy,0,view.size,FH_GPU_COPY_CONTIGUOUS);
    view.data=blur_depth_copy;
    C3D_FrameDrawOn(motion_target);blur_state(0,1);
    C3D_TexBind(0,&native_surface_color);C3D_TexBind(1,&view);C3D_TexBind(2,NULL);
    C3D_DrawArrays(GPU_TRIANGLES,0,6);
    native_surface_split();native_surface_draw_on(target);
    /* SDK retains the descriptor pointer; remove the stack view while live. */
    C3D_TexBind(1,NULL);
    blur_depth=motion_history; /* Borrow storage, own only nearest sampling. */
    C3D_TexSetFilter(&blur_depth,GPU_NEAREST,GPU_NEAREST);
    C3D_TexSetWrap(&blur_depth,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    blur_valid=1;blur_captures++;blur_capture_ticks+=svcGetSystemTick()-started;return 1;
}
static int blur_threshold(float projected_depth,unsigned *threshold){
    float encoded=16777216.f*(projected_depth+1.f);
    /* Match the source cast in its defined range; reject malformed projection
     * rather than invoking undefined float-to-integer conversion. */
    if(!(encoded>=0.f&&encoded<4294967296.f))return 0;
    *threshold=((u32)encoded>>16)&255u;return 1;
}
void fh_pica_blur_filter(float projected_depth,unsigned one_sided,unsigned strong){
    if(!fh_pica_blur_filter_enabled())return;
    blur_requests++;
    if(!blur_valid||!motion_valid){blur_unseeded++;return;}
    unsigned threshold;if(!blur_threshold(projected_depth,&threshold) || !blur_prepare())return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 started=svcGetSystemTick();fh_minimal_auxiliary_begin();
    fh_pica_blur_material_threshold(&blur_material,threshold);
    native_surface_split();C3D_FrameDrawOn(blur_partial_target);blur_state(0,0);
    C3D_TexBind(0,&motion_history);C3D_TexBind(1,&motion_history);C3D_TexBind(2,&motion_history);
    C3D_DrawArrays(GPU_TRIANGLES,0,6);
    native_surface_split();native_surface_draw_on(target);
    blur_state(fh_pica_blur_material_mode(one_sided,strong),0);
    C3D_TexBind(0,&blur_partial);C3D_TexBind(1,&motion_history);C3D_TexBind(2,&blur_depth);
    C3D_DrawArrays(GPU_TRIANGLES,0,6);
    fh_pica_fog_invalidate();fh_minimal_auxiliary_end(2,12);
    blur_draws+=2;blur_draw_ticks+=svcGetSystemTick()-started;
}
static void blur_shutdown(void){
    if(blur_depth_copy){vramFree(blur_depth_copy);blur_depth_copy=NULL;}
    /* Parent has retired every GPU reader before invoking this. */
    if(blur_partial_target)C3D_RenderTargetDelete(blur_partial_target);
    if(blur_partial.data)C3D_TexDelete(&blur_partial);
    memset(&blur_partial,0,sizeof(blur_partial));
    memset(&blur_depth,0,sizeof(blur_depth));
    if(blur_binary){shaderProgramFree(&blur_program);DVLB_Free(blur_binary);}
    linearFree(blur_quads);blur_quads=NULL;blur_binary=NULL;blur_valid=0;
    blur_partial_target=NULL;
    char line[256];snprintf(line,sizeof(line),"PICA blur filter: captures=%u requests=%u draws=%u unseeded=%u vram_peak=%u linear_peak=%u capture_ticks=%llu draw_ticks=%llu CPU_pixels=0\n",
        blur_captures,blur_requests,blur_draws,blur_unseeded,blur_draws?BLUR_VRAM_BYTES:0,
        blur_captures?BLUR_LINEAR_BYTES:0,blur_capture_ticks,blur_draw_ticks);
    fh_platform_log(line);
}
