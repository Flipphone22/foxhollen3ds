/* Persistent splash ring; immutable image ownership and native uniform updates.
 * Optional native scene reflection uses load-compiled materials. Indirect
 * distortion and exact filtered bump opacity remain incomplete. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_splash.h"
#include "fh_pica_image.h"
#include "fh_pica_hud.h"
#include "fh_pica_fog.h"
#include "fh_pica_viewport.h"
#include "fh_minimal_graphics.h"
#include "fh_graphics_options.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
_Static_assert(sizeof(FhPicaSplashVertex)==28,"Splash vertex layout");
enum {VERTEX_BYTES=128*28,INDEX_OFFSET=VERTEX_BYTES,ALLOCATION_BYTES=4864};
static shaderProgram_s program;
static void *geometry;
static unsigned images[2],intensity[2],ready,loads,objects,draws,disabled,indices,peak;
static C3D_Tex samplers[2];
static float scales[2][2];
static u64 submit_ticks;
#include "platform_pica_splash_material.h"
#include "platform_pica_splash_reflection.h"
void fh_pica_splash_init(void *entry){
    shaderProgramInit(&program);shaderProgramSetVsh(&program,(DVLE_s*)entry);
    if(shaderInstanceGetUniformLocation(program.vertexShader,"projection")!=0 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"view")!=4 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"bands")!=7 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"colors")!=31 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"fog_distance_scale")!=94 ||
       shaderInstanceGetUniformLocation(program.vertexShader,"uvscale")!=39)
        fh_platform_panic("PICA splash uniform ABI");
    ready=1;
}
static void release_retired(void){
    for(unsigned i=0;i<2;i++){fh_pica_image_release(images[i]);images[i]=0;}
    linearFree(geometry);geometry=NULL;
}
void fh_pica_splash_release(void){
    if(!geometry && !images[0] && !images[1])return;
    /* Melee's immutable-resource ownership rule: retire all queued readers
     * before freeing shared textures or the ring, including during map unload. */
    fh_minimal_sync();fh_pica_scene_invalidate();release_retired();
}
void fh_pica_splash_load(const float pos[16][3],const float uv[128][2],const FhTexture *mask,const FhTexture *foam){
    if(!ready)fh_platform_panic("PICA splash initialization order");
    fh_pica_splash_release();
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<ALLOCATION_BYTES+__ctru_linear_heap_size*15/100)
        return;
    geometry=linearMemAlign(ALLOCATION_BYTES,128);
    if(!geometry)return;
    memset(geometry,0,ALLOCATION_BYTES);
    fh_pica_splash_template(geometry,(uint16_t*)((char*)geometry+INDEX_OFFSET),pos,uv);
    fh_gpu_cache_flush(geometry,ALLOCATION_BYTES);
    const FhTexture *sources[2]={mask,foam};
    for(unsigned i=0;i<2;i++){
        images[i]=fh_pica_image_acquire(sources[i]);
        if(!images[i])fh_platform_panic("PICA splash image allocation");
        fh_pica_image_sampler(images[i],sources[i],samplers+i,scales[i]);
        intensity[i]=sources[i]->format<=1;
    }
    splash_material_prepare();
    loads++;peak=ALLOCATION_BYTES;
}
void fh_pica_splash_draw(const FhPicaSplashState *s,const float view[3][4],const float projection[4][4],
    const unsigned scissor[4],const uint8_t ambient[3]){
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    if(!(fh_graphics_options&FH_GRAPHICS_SPLASHES) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER)){disabled++;return;}
    if(!geometry || !images[0] || !images[1]){disabled++;return;}
    u64 start=svcGetSystemTick();
    C3D_Tex *reflection=fh_pica_splash_capture();
    float rows[8][3][4],alpha[8];fh_pica_splash_bands(s,rows,alpha);
    fh_pica_hud_flush();unsigned clip[4];if(!fh_pica_hud_target(scissor,clip))return;
    fh_pica_scene_reserve();fh_pica_scene_invalidate();
    C3D_BindProgram(reflection?splash_reflection_program():&program);C3D_LightEnvBind(NULL);
    C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    unsigned low=ambient[0]>>2 | ((ambient[1]>>2)<<8) | ((ambient[2]>>2)<<16) | 0xff000000u;
    fh_pica_fog_invalidate();
    if(reflection)splash_reflection_bind(reflection,low);
    else{
        unsigned textured=!(fh_graphics_options&FH_GRAPHICS_NO_PARTICLE_TEX);
        for(unsigned i=0;i<3;i++)C3D_TexBind(i,i<2&&textured?samplers+i:NULL);
        splash_material_publish(textured,low,NULL);
        fh_pica_fog_bind(1);
    }
    C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
    float viewport[6],native_projection[4][4],depth[2];fh_pica_live_viewport(viewport);
    fh_pica_viewport_xy(native_projection,projection,viewport);
    fh_pica_viewport_depth(depth,viewport[4],viewport[5]);
    C3D_DepthMap(true,depth[0],depth[1]);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
    C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ZERO,GPU_ONE);
    /* The shared target is rotated for the LCD, as in the native scene owner. */
    for(unsigned i=0;i<4;i++){
        unsigned row=i==0?1:i==1?0:i;float sign=i==1?-1.f:1.f;
        C3D_FVUnifSet(GPU_VERTEX_SHADER,i,sign*native_projection[row][0],sign*native_projection[row][1],sign*native_projection[row][2],sign*native_projection[row][3]);
    }
    for(unsigned i=0;i<3;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,4+i,view[i][0],view[i][1],view[i][2],view[i][3]);
    for(unsigned b=0;b<8;b++){
        for(unsigned i=0;i<3;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+b*3+i,rows[b][i][0],rows[b][i][1],rows[b][i][2],rows[b][i][3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,31+b,1,1,1,alpha[b]);
    }
    C3D_FVUnifSet(GPU_VERTEX_SHADER,39,scales[0][0],scales[0][1],0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,40,scales[1][0]*12,scales[1][1],0,0);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,2);AttrInfo_AddLoader(a,2,GPU_UNSIGNED_BYTE,4);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,geometry,28,3,0x210)<0)fh_platform_panic("PICA splash vertex layout");
    /* Preserve the source's back-surface then front-surface transparent order. */
    C3D_CullFace(GPU_CULL_BACK_CCW);
    C3D_DrawElements(GPU_TRIANGLES,FH_SPLASH_INDICES,C3D_UNSIGNED_SHORT,(char*)geometry+INDEX_OFFSET);
    fh_pica_hud_account(FH_SPLASH_INDICES);
    C3D_CullFace(GPU_CULL_FRONT_CCW);
    C3D_DrawElements(GPU_TRIANGLES,FH_SPLASH_INDICES,C3D_UNSIGNED_SHORT,(char*)geometry+INDEX_OFFSET);
    fh_pica_hud_account(FH_SPLASH_INDICES);
    objects++;draws+=2;indices+=2*FH_SPLASH_INDICES;submit_ticks+=svcGetSystemTick()-start;
    fh_pica_scene_invalidate();fh_pica_fog_invalidate();
}
void fh_pica_splash_shutdown(void){
    /* Host shutdown has already retired readers and freed its UI stream.
     * Do not reopen a command frame through the mid-game retirement helper. */
    release_retired();
    char line[240];snprintf(line,sizeof(line),"PICA splash: loads=%u objects=%u draws=%u indices=%u disabled=%u linear_peak=%u submit_ticks=%llu CPU_vertices=0 reflection_control=toggle\n",
        loads,objects,draws,indices,disabled,peak,(unsigned long long)submit_ticks);fh_platform_log(line);
    splash_reflection_shutdown();
    shaderProgramFree(&program);ready=0;
}
