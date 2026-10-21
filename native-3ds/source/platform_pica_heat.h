/* Native framebuffer owner. Immutable wave basis, GPU animation and sampling.
 * Shared history is captured at the original boundary, never the active target. */
#include "fh_pica_heat.h"
#include "fh_pica_heat_plan.h"
#include "fh_pica_heat_material.h"
#include "heat_shader.h"
enum {HEAT_VERTEX_BYTES=FH_HEAT_VERTICES*sizeof(FhHeatVertex),
      HEAT_INDEX_OFFSET=(HEAT_VERTEX_BYTES+127)&~127,
      HEAT_BYTES=HEAT_INDEX_OFFSET+FH_HEAT_INDICES*sizeof(uint16_t)};
static void *heat_mesh;
static DVLB_s *heat_binary;
static shaderProgram_s heat_program;
static C3D_AttrInfo heat_attributes;
static C3D_BufInfo heat_buffers;
static C3D_TexEnv heat_material[6];
static unsigned heat_requests,heat_draws,heat_unseeded,heat_peak;
static u64 heat_ticks,heat_prepare_ticks,heat_draw_ticks;
unsigned fh_pica_heat_enabled(void){
    return initialized && (fh_graphics_options&~fh_graphics_unavailable_mask&FH_GRAPHICS_HEAT) &&
        !(fh_graphics_options&FH_GRAPHICS_NO_RENDER);
}
static int heat_prepare(void){
    if(heat_mesh)return 1;
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<HEAT_BYTES+__ctru_linear_heap_size*15/100)goto unavailable;
    heat_binary=DVLB_ParseFile((u32*)heat_shader,sizeof(heat_shader));
    if(!heat_binary)goto unavailable;
    shaderProgramInit(&heat_program);
    if(R_FAILED(shaderProgramSetVsh(&heat_program,heat_binary->DVLE)))goto unavailable;
    heat_mesh=linearMemAlign(HEAT_BYTES,128);
    if(!heat_mesh)goto unavailable;
    fh_pica_heat_mesh(heat_mesh,(void*)((char*)heat_mesh+HEAT_INDEX_OFFSET));
    fh_gpu_cache_flush(heat_mesh,HEAT_BYTES);
    AttrInfo_Init(&heat_attributes);AttrInfo_AddLoader(&heat_attributes,0,GPU_UNSIGNED_BYTE,4);
    AttrInfo_AddLoader(&heat_attributes,1,GPU_SHORT,4);
    BufInfo_Init(&heat_buffers);
    if(BufInfo_Add(&heat_buffers,heat_mesh,sizeof(FhHeatVertex),2,0x10)<0)fh_platform_panic("Heat immutable buffers");
    fh_pica_heat_material_init(heat_material);heat_peak=HEAT_BYTES;return 1;
unavailable:
    if(heat_binary){shaderProgramFree(&heat_program);DVLB_Free(heat_binary);heat_binary=NULL;}
    fh_graphics_options_reject(FH_GRAPHICS_HEAT);return 0;
}
void fh_pica_heat(float sx,float sy,int pitch,unsigned alpha,unsigned threshold,const float projection[4][4]){
    if(!fh_pica_heat_enabled() || !alpha)return;
    heat_requests++;
    if(!blur_valid || !motion_valid){heat_unseeded++;return;}
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 started=svcGetSystemTick();if(!heat_prepare())return;
    u64 prepared=svcGetSystemTick();heat_prepare_ticks+=prepared-started;
    FhHeatParameters parameters;fh_pica_heat_parameters(&parameters,sx,sy,(int16_t)pitch,(uint8_t)alpha);
    fh_minimal_auxiliary_begin();native_surface_draw_on(target);
    C3D_BindProgram(&heat_program);C3D_LightEnvBind(NULL);
    C3D_SetAttrInfo(&heat_attributes);C3D_SetBufInfo(&heat_buffers);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,1.f/FH_HEAT_X,1.f/FH_HEAT_Y,1.f/32767,
        projection[2][3]-8*projection[2][2]);
    const float *rows[]={parameters.phase,parameters.offset_x,parameters.offset_y,parameters.alpha};
    for(unsigned i=0;i<4;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,i+1,rows[i][0],rows[i][1],rows[i][2],rows[i][3]);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,5,projection[1][1]*480,-projection[0][0]*640,projection[1][3],-projection[0][3]);
    fh_native_fog_apply(0);C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(true,GPU_GREATER,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ZERO,GPU_ONE);
    C3D_TexEnvBufUpdate(C3D_Both,0);fh_pica_heat_material_threshold(heat_material,threshold);
    for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,heat_material+i);
    C3D_TexBind(0,&motion_history);C3D_TexBind(1,&blur_depth);C3D_TexBind(2,NULL);fh_pica_texture2_coord1(0);
    /* Bounded aligned index runs; never depend on a >65535-index submission. */
    const uint16_t *indices=(const void*)((const char*)heat_mesh+HEAT_INDEX_OFFSET);
    unsigned batches=0;
    for(unsigned first=0;first<FH_HEAT_INDICES;){
        unsigned count=FH_HEAT_INDICES-first;if(count>65532)count=65532;
        C3D_DrawElements(GPU_TRIANGLES,count,C3D_UNSIGNED_SHORT,indices+first);first+=count;batches++;
    }
    fh_pica_fog_invalidate();fh_minimal_auxiliary_end(batches,FH_HEAT_INDICES);
    u64 ended=svcGetSystemTick();heat_draws+=batches;heat_ticks+=ended-started;heat_draw_ticks+=ended-prepared;
}
static void heat_shutdown(void){
    /* Enclosing shutdown has joined every GPU reader. */
    linearFree(heat_mesh);heat_mesh=NULL;
    if(heat_binary){shaderProgramFree(&heat_program);DVLB_Free(heat_binary);heat_binary=NULL;}
    char line[240];snprintf(line,sizeof(line),"PICA heat: requests=%u draws=%u unseeded=%u linear_peak=%u submit_ticks=%llu prepare_ticks=%llu draw_ticks=%llu CPU_vertices=0\n",
        heat_requests,heat_draws,heat_unseeded,heat_peak,heat_ticks,heat_prepare_ticks,heat_draw_ticks);fh_platform_log(line);
}
