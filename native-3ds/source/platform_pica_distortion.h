/* Included after motion/depth capture owners. Source images are immutable;
 * capture snapshots remain owned by motion/blur and are never freed here. */
#include "fh_pica_distortion.h"
#include "fh_pica_distortion_draw.h"
#include "fh_pica_distortion_composite.h"
#include "fh_pica_distortion_capture_mask.h"
#include "fh_pica_distortion_radial.h"
#include "distortion_shader.h"
static FhPicaDistortionGeometry distortion_geometry;
static C3D_Tex distortion_radial;
/* Preserve raw history across subsequent frames without a reflection refresh. */
enum {DISTORTION_MASK_VRAM_BYTES=512*256*4};
static C3D_Tex distortion_mask;
static C3D_RenderTarget *distortion_mask_target;
static unsigned distortion_mask_draws;
static DVLB_s *distortion_binary;
static shaderProgram_s distortion_program;
static C3D_TexEnv distortion_material[6];
static unsigned distortion_ready,distortion_requests,distortion_unseeded,distortion_draws;
static u64 distortion_prepare_ticks,distortion_submit_ticks;
unsigned fh_pica_distortion_enabled(void){
    return initialized && (fh_graphics_effects&~fh_effects_unavailable_mask&FH_EFFECT_DISTORTION) &&
        !(fh_graphics_options&FH_GRAPHICS_NO_RENDER);
}
static int distortion_prepare(const unsigned char *field,size_t field_bytes,
    const unsigned char *radial,size_t radial_bytes){
    if(distortion_ready)return 1;
    static int hardware=-1;
    if(hardware<0){FILE *f=fopen("sdmc:/3ds/foxhollow/distortion_hw.txt","rb");int c=f?fgetc(f):EOF;if(f)fclose(f);hardware=c=='1';}
    if(!hardware)goto unavailable;
    extern u32 __ctru_linear_heap_size;
    if(!field||!radial||field_bytes!=FH_DISTORTION_FIELD_BYTES||radial_bytes!=FH_DISTORTION_RADIAL_BYTES)
        goto unavailable;
    size_t reserve=__ctru_linear_heap_size*15/100;
    if(linearSpaceFree()<FH_DISTORTION_GEOMETRY_BYTES+FH_DISTORTION_RADIAL_BYTES+reserve ||
       vramSpaceFree()<DISTORTION_MASK_VRAM_BYTES+1024*1024)
        goto unavailable;
    distortion_binary=DVLB_ParseFile((u32*)distortion_shader,sizeof(distortion_shader));
    if(!distortion_binary)goto unavailable;
    shaderProgramInit(&distortion_program);
    if(R_FAILED(shaderProgramSetVsh(&distortion_program,distortion_binary->DVLE)))goto unavailable;
    if(!C3D_TexInit(&distortion_radial,128,128,GPU_A8))goto unavailable;
    if(!fh_pica_distortion_radial(distortion_radial.data,FH_DISTORTION_RADIAL_BYTES,radial,radial_bytes))goto unavailable;
    C3D_TexSetFilter(&distortion_radial,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&distortion_radial,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    fh_gpu_cache_flush(distortion_radial.data,FH_DISTORTION_RADIAL_BYTES);
    if(!fh_pica_distortion_geometry_prepare(&distortion_geometry,field,field_bytes,1,reserve))goto unavailable;
    if(!C3D_TexInitVRAM(&distortion_mask,512,256,GPU_RGBA8))goto unavailable;
    C3D_TexSetFilter(&distortion_mask,GPU_NEAREST,GPU_NEAREST);
    C3D_TexSetWrap(&distortion_mask,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    /* Final allocation: no live target is deleted in failed open-frame init. */
    distortion_mask_target=C3D_RenderTargetCreateFromTex(&distortion_mask,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    if(!distortion_mask_target)goto unavailable;
    fh_pica_distortion_composite_prepare(distortion_material);
    distortion_ready=1;return 1;
unavailable:
    /* No draw has referenced these candidates; no GPU wait or live free. */
    fh_pica_distortion_geometry_destroy_after_fence(&distortion_geometry);
    if(distortion_mask.data)C3D_TexDelete(&distortion_mask);
    memset(&distortion_mask,0,sizeof(distortion_mask));
    if(distortion_radial.data)C3D_TexDelete(&distortion_radial);
    memset(&distortion_radial,0,sizeof(distortion_radial));
    if(distortion_binary){shaderProgramFree(&distortion_program);DVLB_Free(distortion_binary);distortion_binary=NULL;}
    fh_graphics_effects_reject(FH_EFFECT_DISTORTION);return 0;
}
static void distortion_make_mask(const FhPicaDistortionPlan *plan){
    C3D_TexEnv stages[6];
    fh_pica_distortion_capture_mask(stages,plan->color[0]>>24,plan->color[3]>>24,plan->color[1]>>24);
    native_surface_split();C3D_FrameDrawOn(distortion_mask_target);blur_state(0,0);
    for(unsigned i=0;i<2;i++){
        C3D_FVUnifSet(GPU_VERTEX_SHADER,2*i,1,0,0,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,2*i+1,0,-1,1,0);
    }
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,stages+i);
    C3D_TexEnvBufUpdate(C3D_Both,0);C3D_TexEnvBufUpdate(C3D_Alpha,4);
    /* Ordinary blur mode excludes alpha writes; mask needs RGBA. */
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_TexBind(0,&motion_history);C3D_TexBind(1,&blur_depth);C3D_TexBind(2,NULL);
    C3D_DrawArrays(GPU_TRIANGLES,0,6);distortion_mask_draws++;
    native_surface_split();native_surface_draw_on(target);C3D_TexEnvBufUpdate(C3D_Both,0);
}
void fh_pica_distortion_render(const FhPicaDistortionPlan *plan,const float viewport[4],
    const unsigned char *field,size_t field_bytes,const unsigned char *radial,size_t radial_bytes){
    if(!fh_pica_distortion_enabled())return;
    distortion_requests++;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();
    if(!distortion_prepare(field,field_bytes,radial,radial_bytes))return;
    u64 prepared=svcGetSystemTick();distortion_prepare_ticks+=prepared-start;
    fh_minimal_auxiliary_begin();
    /* Original lightmap.c refreshes both reflection images immediately before
     * each distortion call, separately from the earlier motion-blur capture. */
    if(!motion_prepare()||!blur_capture(1)){
        distortion_unseeded++;fh_minimal_auxiliary_end(0,0);return;
    }
    motion_valid=1;motion_captures++;
    distortion_make_mask(plan);
    unsigned draws=fh_pica_distortion_draw(&distortion_geometry,&distortion_program,
        distortion_material,plan,viewport,&motion_history,&distortion_mask,&distortion_radial);
    fh_pica_fog_invalidate();fh_minimal_auxiliary_end(draws+2,(draws?FH_DISTORTION_INDICES:0)+12);
    distortion_draws+=draws;distortion_submit_ticks+=svcGetSystemTick()-prepared;
}
static void distortion_shutdown(void){
    /* Enclosing shutdown has joined every GPU reader. No subsequent draws. */
    if(distortion_mask_target)C3D_RenderTargetDelete(distortion_mask_target);
    distortion_mask_target=NULL;
    if(distortion_mask.data)C3D_TexDelete(&distortion_mask);
    memset(&distortion_mask,0,sizeof(distortion_mask));
    fh_pica_distortion_geometry_destroy_after_fence(&distortion_geometry);
    if(distortion_radial.data)C3D_TexDelete(&distortion_radial);
    memset(&distortion_radial,0,sizeof(distortion_radial));
    if(distortion_binary){shaderProgramFree(&distortion_program);DVLB_Free(distortion_binary);distortion_binary=NULL;}
    char line[240];snprintf(line,sizeof(line),"PICA distortion: requests=%u draws=%u unseeded=%u linear_bytes=%u prepare_ticks=%llu submit_ticks=%llu CPU_vertices=0\n",
        distortion_requests,distortion_draws,distortion_unseeded,
        distortion_ready?FH_DISTORTION_GEOMETRY_BYTES+FH_DISTORTION_RADIAL_BYTES:0,
        distortion_prepare_ticks,distortion_submit_ticks);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA distortion mask: draws=%u vram_bytes=%u CPU_pixels=0\n",
        distortion_mask_draws,distortion_ready?DISTORTION_MASK_VRAM_BYTES:0);
    fh_platform_log(line);distortion_ready=0;
}
