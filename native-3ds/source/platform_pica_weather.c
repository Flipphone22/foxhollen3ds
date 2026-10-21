/* Source precipitation on PICA: immutable corner IDs, bounded live uniforms.
 * No per-frame allocation, vertex expansion, render target, GS or worker. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_weather.h"
#include "fh_pica_scene.h"
#include "fh_pica_hud.h"
#include "fh_pica_image.h"
#include "fh_pica_fog.h"
#include "fh_pica_viewport.h"
#include "fh_pica_options.h"
#include "fh_minimal_graphics.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
#include "pica_weather_shader.h"
#include "pica_weather_shared_shader.h"
static DVLB_s *binary[2];
static shaderProgram_s program[2];
static unsigned char *geometry;
static float view[3][4],projection[4][4],shapes[60][4],particles[FH_WEATHER_SHARED_BATCH][4];
static int16_t uv[6];
static unsigned rgba,clip[4],page,count,active,bound;
static unsigned shared_shape,shared_pools;
static unsigned loads,pools,draws,triangles,disabled,publications;
static u64 submit_ticks;
void fh_pica_weather_invalidate(void){bound=0;}
void fh_pica_weather_init(void){
    binary[0]=DVLB_ParseFile((u32*)pica_weather_shader,sizeof(pica_weather_shader));
    binary[1]=DVLB_ParseFile((u32*)pica_weather_shared_shader,sizeof(pica_weather_shared_shader));
    for(unsigned i=0;i<2;i++){
    if(!binary[i] || binary[i]->numDVLE!=1 || binary[i]->DVLP.codeSize>512)fh_platform_panic("PICA weather shader");
    shaderProgramInit(program+i);shaderProgramSetVsh(program+i,binary[i]->DVLE);
    if(shaderInstanceGetUniformLocation(program[i].vertexShader,"shapes")!=7 ||
       shaderInstanceGetUniformLocation(program[i].vertexShader,"particles")!=(i?14:71) ||
       shaderInstanceGetUniformLocation(program[i].vertexShader,"fog_distance_scale")!=94)
        fh_platform_panic("PICA weather uniform ABI");
    }
}
void fh_pica_weather_load(void){
    if(geometry)return;
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<FH_WEATHER_STORAGE+__ctru_linear_heap_size*15/100)return;
    geometry=linearMemAlign(FH_WEATHER_STORAGE,128);
    if(!geometry)return;
    memset(geometry,0,FH_WEATHER_STORAGE);
    for(unsigned i=0;i<FH_WEATHER_SHARED_BATCH;i++)for(unsigned k=0;k<3;k++){
        geometry[(i*3+k)*2]=i;geometry[(i*3+k)*2+1]=k;
    }
    fh_gpu_cache_flush(geometry,FH_WEATHER_STORAGE);loads++;
}
static void flush(void){
    if(!count)return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();fh_pica_scene_reserve();
    if(!bound){
        fh_pica_scene_invalidate();C3D_BindProgram(program+shared_shape);
        C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
        C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
        float viewport[6],p[4][4],d[2];fh_pica_live_viewport(viewport);
        fh_pica_viewport_xy(p,projection,viewport);fh_pica_viewport_depth(d,viewport[4],viewport[5]);
        C3D_DepthMap(true,d[0],d[1]);C3D_CullFace(GPU_CULL_NONE);
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
        C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
        for(unsigned i=0;i<3;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,i,view[i][0],view[i][1],view[i][2],view[i][3]);
        for(unsigned i=0;i<4;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,3+i,p[i][0],p[i][1],p[i][2],p[i][3]);
        for(unsigned i=0;i<(shared_shape?3u:60u);i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+i,shapes[i][0],shapes[i][1],shapes[i][2],0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,shared_shape?13:70,(rgba>>24)/255.f,((rgba>>16)&255)/255.f,((rgba>>8)&255)/255.f,(rgba&255)/255.f);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_UNSIGNED_BYTE,2);
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
        if(BufInfo_Add(b,geometry,2,1,0)<0)fh_platform_panic("PICA weather vertex ABI");
        fh_pica_fog_invalidate();fh_pica_fog_bind(1);bound=1;publications++;
    }
    float scale[2];unsigned intensity=0;
    const C3D_Tex *texture=fh_pica_hud_sampler(page,scale,&intensity);
    if(!texture)fh_platform_panic("PICA weather missing resident image");
    C3D_TexBind(0,fh_pica_options_texture(texture,FH_GRAPHICS_NO_PARTICLE_TEX));
    C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvInit(e);
    C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,GPU_TEXTURE0,0);C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
    if(intensity)C3D_TexEnvOpAlpha(e,0,GPU_TEVOP_A_SRC_R,0);
    for(unsigned i=0;i<3;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,(shared_shape?10:67)+i,uv[i*2]*scale[0]/128.f,1-uv[i*2+1]*scale[1]/128.f,0,1);
    for(unsigned i=0;i<count*(shared_shape?1u:2u);i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,(shared_shape?14:71)+i,particles[i][0],particles[i][1],particles[i][2],particles[i][3]);
    C3D_DrawArrays(GPU_TRIANGLES,0,count*3);fh_pica_hud_account(count*3);
    draws++;triangles+=count;count=0;submit_ticks+=svcGetSystemTick()-start;
}
void fh_pica_weather_begin(const float v[3][4],const float p[4][4],const void *source,unsigned stride,
    const int16_t texcoords[6],uint32_t color,const unsigned scissor[4]){
    if(active || count || stride<9*sizeof(float))fh_platform_panic("PICA weather begin");
    active=1;
    if(!(fh_graphics_options&FH_GRAPHICS_WEATHER) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER)){disabled++;return;}
    if(!geometry){active=2;disabled++;return;}
    fh_pica_hud_flush();if(!fh_pica_hud_target(scissor,clip)){active=2;return;}
    memcpy(view,v,sizeof(view));memcpy(projection,p,sizeof(projection));memcpy(uv,texcoords,sizeof(uv));rgba=color;
    for(unsigned i=0;i<FH_WEATHER_SHAPES;i++){
        const float *rows=(const float*)((const char*)source+i*stride);
        for(unsigned corner=0;corner<3;corner++)for(unsigned axis=0;axis<3;axis++)shapes[i*3+corner][axis]=rows[axis*3+corner];
    }
    /* Source rain commonly repeats one shape. Exact live equality, not a
     * cloud-type guess, frees 57 uniform rows and preserves flake order. */
    shared_shape=1;
    for(unsigned i=1;i<FH_WEATHER_SHAPES;i++)if(memcmp(shapes,shapes+i*3,3*sizeof(*shapes))){shared_shape=0;break;}
    shared_pools+=shared_shape;
    bound=0;pools++;
}
void fh_pica_weather_particle(unsigned which,float x,float y,float z,float size,unsigned shape){
    if(!active)fh_platform_panic("PICA weather outside cloud");
    if(!(fh_graphics_options&FH_GRAPHICS_WEATHER) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER) || active==2)return;
    if(shape>=FH_WEATHER_SHAPES || !which)fh_platform_panic("PICA weather source shape/page");
    if(count && (page!=which || count==(shared_shape?FH_WEATHER_SHARED_BATCH:FH_WEATHER_BATCH)))flush();
    page=which;
    float *a=particles[count*(shared_shape?1:2)];
    a[0]=x;a[1]=y;a[2]=z;a[3]=size;
    if(!shared_shape){float*b=particles[count*2+1];b[0]=shape*3;b[1]=b[2]=b[3]=0;}
    count++;
}
void fh_pica_weather_end(void){
    if(!active)fh_platform_panic("PICA weather end");
    flush();active=0;
    /* OFF/empty/clipped clouds never changed the pipeline. */
    if(bound){fh_pica_scene_invalidate();fh_pica_fog_invalidate();}
}
void fh_pica_weather_shutdown(void){
    if(active || count)fh_platform_panic("PICA weather pending shutdown");
    /* Parent already retired every GPU reader; no shutdown frame is reopened. */
    linearFree(geometry);geometry=NULL;
    for(unsigned i=0;i<2;i++){shaderProgramFree(program+i);DVLB_Free(binary[i]);binary[i]=NULL;}
    char line[256];snprintf(line,sizeof(line),"PICA weather: pools=%u draws=%u triangles=%u disabled=%u publications=%u linear_bytes=%u submit_ticks=%llu CPU_vertices=0 shared_pools=%u\n",
        pools,draws,triangles,disabled,publications,loads?FH_WEATHER_STORAGE:0,(unsigned long long)submit_ticks,shared_pools);fh_platform_log(line);
}
