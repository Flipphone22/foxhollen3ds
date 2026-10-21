#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_receiver_plane.h"
#include "fh_pica_shadow_image.h"
#include "fh_pica_scene.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "pica_shadow_pair_shader.h"
typedef struct {s8 xyzw[4][4];u16 indices[6];} ReceiverQuad;
static ReceiverQuad *quad;
static DVLB_s *binary;
static shaderProgram_s program,clear_program,project_program;
static C3D_AttrInfo attributes;
static C3D_BufInfo buffers;
static unsigned passes;
void fh_pica_receiver_init(void){
    if(quad)fh_platform_panic("PICA receiver duplicate init");
    quad=linearMemAlign(sizeof(*quad),128);
    if(!quad)fh_platform_panic("PICA receiver quad allocation");
    static const ReceiverQuad source={{{-1,-1,0,1},{1,-1,0,1},{1,1,0,1},{-1,1,0,1}},{0,1,2,0,2,3}};
    memcpy(quad,&source,sizeof(source));fh_gpu_cache_flush(quad,sizeof(*quad));
    binary=DVLB_ParseFile((u32*)pica_shadow_pair_shader,sizeof(pica_shadow_pair_shader));
    if(!binary || binary->numDVLE!=3 || binary->DVLP.codeSize>128)fh_platform_panic("PICA receiver shader");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,binary->DVLE);
    shaderProgramInit(&clear_program);shaderProgramSetVsh(&clear_program,binary->DVLE+1);
    shaderProgramInit(&project_program);shaderProgramSetVsh(&project_program,binary->DVLE+2);
    const char *names[]={"view","projection","bones","decode","fog_distance_scale"};
    const int slots[]={0,3,10,76,94};
    for(unsigned i=0;i<sizeof(slots)/sizeof(*slots);i++)if(shaderInstanceGetUniformLocation(program.vertexShader,names[i])!=slots[i])fh_platform_panic("PICA shadow pair ABI");
    AttrInfo_Init(&attributes);AttrInfo_AddLoader(&attributes,0,GPU_BYTE,4);
    BufInfo_Init(&buffers);
    if(BufInfo_Add(&buffers,quad->xyzw,4,1,0)<0)fh_platform_panic("PICA receiver buffer");
}
void *fh_pica_receiver_caster_program(void){
    if(!quad)fh_platform_panic("PICA silhouette program lifetime");
    return &program;
}
void fh_pica_receiver_draw(const FhPicaReceiverPlane *p,const unsigned clip[4]){
    if(!quad)fh_platform_panic("PICA receiver not initialized");
    fh_pica_scene_reserve();C3D_BindProgram(&clear_program);
    C3D_SetAttrInfo(&attributes);C3D_SetBufInfo(&buffers);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,p->equation[0],p->equation[1],0,p->equation[3]);
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_PRIMARY_COLOR,0,0);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_CullFace(GPU_CULL_NONE);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_DepthMap(true,p->depth[0],p->depth[1]);C3D_DepthTest(true,GPU_LESS,0);
    C3D_StencilTest(true,GPU_ALWAYS,2,3,2);
    C3D_StencilOp(GPU_STENCIL_KEEP,GPU_STENCIL_ZERO,GPU_STENCIL_REPLACE);
    C3D_DrawElements(GPU_TRIANGLES,6,C3D_UNSIGNED_SHORT,quad->indices);passes++;
    fh_pica_scene_invalidate();
}
void fh_pica_receiver_shutdown(void){
    /* Parent scene shutdown joins all submitted GPU work before this call. */
    if(!quad)return;
    char line[160];snprintf(line,sizeof(line),"PICA receiver quad: passes=%u indices=%u linear_bytes=%u CPU_vertices=0\n",passes,passes*6,(unsigned)sizeof(*quad));fh_platform_log(line);
    linearFree(quad);quad=NULL;shaderProgramFree(&program);shaderProgramFree(&clear_program);shaderProgramFree(&project_program);DVLB_Free(binary);binary=NULL;
}

void *fh_pica_receiver_project_program(void){
    if(!quad)fh_platform_panic("PICA projected receiver program lifetime");
    return &project_program;
}
void fh_pica_receiver_clear_image(void){
    if(!quad)fh_platform_panic("PICA shadow clear program lifetime");
    fh_pica_scene_reserve();fh_pica_shadow_image_rebind();
    C3D_BindProgram(&clear_program);
    C3D_SetAttrInfo(&attributes);C3D_SetBufInfo(&buffers);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,0,0,0,-.5f);
    C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_CONSTANT,0,0);
    C3D_TexEnvColor(C3D_GetTexEnv(0),0);C3D_TexEnvBufUpdate(C3D_Both,0);
    C3D_CullFace(GPU_CULL_NONE);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_DrawElements(GPU_TRIANGLES,6,C3D_UNSIGNED_SHORT,quad->indices);
    fh_pica_scene_invalidate();
}
