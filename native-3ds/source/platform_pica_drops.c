/* Original live droplet centres -> one native PICA point batch. No CPU quad
 * construction, simulation changes, per-frame vertex writes or new GS. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include "fh_pica_drops.h"
#include "fh_pica_scene.h"
#include "fh_pica_hud.h"
#include "fh_pica_fog.h"
#include "fh_pica_viewport.h"
#include "fh_graphics_options.h"
#include "fh_minimal_graphics.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
enum {INDEX_OFFSET=240};
static shaderProgram_s program;
static int live_uniform,textured_uniform;
static void *geometry;
static unsigned loads,draws,points,maximum;
static u64 submit_ticks;
void fh_pica_drops_init(void *entry){
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)entry);
    live_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"live_points");
    textured_uniform=shaderInstanceGetUniformLocation(program.vertexShader,"textured");
    if(live_uniform<0 || textured_uniform<0 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"point_centers")!=30 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"fog_distance_scale")!=94 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"corners")!=23)
        fh_platform_panic("PICA drop shader ABI");
}
void fh_pica_drops_release(void){
    if(!geometry)return;
    /* Same immutable owner/retirement rule as pinned Melee and native scene. */
    fh_minimal_sync();fh_pica_scene_invalidate();
    linearFree(geometry);geometry=NULL;
}
static unsigned load_declined;
void fh_pica_drops_load(void){
    if(geometry)return;
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<FH_PICA_DROP_STORAGE+__ctru_linear_heap_size*15/100)
        {load_declined++;return;}
    geometry=linearMemAlign(FH_PICA_DROP_STORAGE,128);
    if(!geometry){load_declined++;return;}
    fh_pica_drops_template(geometry,(uint16_t*)((char*)geometry+INDEX_OFFSET));
    fh_gpu_cache_flush(geometry,FH_PICA_DROP_STORAGE);loads++;
}
void fh_pica_drops_draw(const float (*centers)[3],unsigned count,
    const float view[3][4],const float projection[4][4],const unsigned scissor[4],const uint8_t ambient[3]){
    if(!(fh_graphics_options&FH_GRAPHICS_WATER_DROPS) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER) || !count)return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    if(!geometry)return;
    if(count>FH_PICA_DROP_CAPACITY)fh_platform_panic("PICA drop batch lifetime/capacity");
    u64 start=svcGetSystemTick();
    fh_pica_hud_flush();unsigned clip[4];if(!fh_pica_hud_target(scissor,clip))return;
    fh_pica_scene_reserve();fh_pica_scene_invalidate();
    C3D_BindProgram(&program);C3D_LightEnvBind(NULL);
    C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,live_uniform,true);
    C3D_BoolUnifSet(GPU_VERTEX_SHADER,textured_uniform,false);
    for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    C3D_TexEnvSrc(e,C3D_Both,GPU_CONSTANT,0,0);
    C3D_TexEnvFunc(e,C3D_Both,GPU_REPLACE);C3D_TexEnvColor(e,fh_pica_drops_color(ambient));
    /* The source point setup inherits scene fog. Reclaim stage 5 after the
     * material reset, including when the preceding draw also enabled fog. */
    fh_pica_fog_invalidate();fh_pica_fog_bind(1);
    float viewport[6],proj[4][4],depth[2];fh_pica_live_viewport(viewport);
    fh_pica_viewport_xy(proj,projection,viewport);fh_pica_viewport_depth(depth,viewport[4],viewport[5]);
    if(!(viewport[2]>0 && viewport[3]>0))fh_platform_panic("PICA drop viewport");
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthMap(true,depth[0],depth[1]);
    C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ZERO,GPU_ONE);
    for(unsigned r=0;r<3;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,r,view[r][0],view[r][1],view[r][2],view[r][3]);
    for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+r,proj[r][0],proj[r][1],proj[r][2],proj[r][3]);
    const float corners[4][2]={{-.5f,-.5f},{.5f,-.5f},{-.5f,.5f},{.5f,.5f}};
    for(unsigned c=0;c<4;c++)C3D_FVUnifSet(GPU_VERTEX_SHADER,23+c,corners[c][0],corners[c][1],0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,27,1,1,0,0);
    /* Source GX point size 0x12 means three source-viewport pixels, not
     * three native-LCD pixels. Shader handles the rotated target. */
    C3D_FVUnifSet(GPU_VERTEX_SHADER,28,1.f/(3*viewport[2]),1.f/(3*viewport[3]),18,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,29,0,1.f/32,1.f/16,1.f/8);
    for(unsigned i=0;i<count;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,30+i,centers[i][0],centers[i][1],centers[i][2],1);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddFixed(a,0);C3D_FixedAttribSet(0,0,0,0,1);
    AttrInfo_AddLoader(a,1,GPU_UNSIGNED_BYTE,2);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,geometry,2,1,1)<0)fh_platform_panic("PICA drop vertex layout");
    C3D_DrawElements(GPU_TRIANGLES,count*6,C3D_UNSIGNED_SHORT,(char*)geometry+INDEX_OFFSET);
    fh_pica_hud_account(count*6);draws++;points+=count;if(count>maximum)maximum=count;
    submit_ticks+=svcGetSystemTick()-start;
    fh_pica_scene_invalidate();fh_pica_fog_invalidate();
}
void fh_pica_drops_shutdown(void){
    /* Host already retired every GPU reader before scene shutdown. */
    linearFree(geometry);geometry=NULL;shaderProgramFree(&program);
    char line[240];snprintf(line,sizeof(line),"PICA water drops: loads=%u draws=%u points=%u max_batch=%u linear_bytes=%u submit_ticks=%llu CPU_vertices=0\n",
        loads,draws,points,maximum,loads?FH_PICA_DROP_STORAGE:0,(unsigned long long)submit_ticks);fh_platform_log(line);
}
