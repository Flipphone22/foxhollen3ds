/* Native lightning: immutable quad IDs, source endpoint uniforms, GPU expansion.
 * Parent scene retires all readers before shutdown. No per-draw allocations. */
#include <3ds.h>
#include <citro3d.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_lightning.h"
#include "fh_pica_scene.h"
#include "fh_pica_hud.h"
#include "fh_pica_image.h"
#include "fh_pica_viewport.h"
#include "fh_pica_options.h"
#include "fh_minimal_graphics.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_replay_timing.h"
#include "pica_lightning_shader.h"
static DVLB_s *binary;
static shaderProgram_s program;
static unsigned char *geometry;
static float projection[4][4],depth[2],points[FH_LIGHTNING_BATCH*2][4],previous[3];
static unsigned active,have_previous,width,count,page,rgba,clip[4],bound;
static unsigned bolts,draws,segments,disabled,invalid_points;
static u64 submit_ticks;
void fh_pica_lightning_invalidate(void){bound=0;}
void fh_pica_lightning_init(void){
    binary=DVLB_ParseFile((u32*)pica_lightning_shader,sizeof(pica_lightning_shader));
    if(!binary || binary->numDVLE!=1 || binary->DVLP.codeSize>128)fh_platform_panic("PICA lightning shader");
    shaderProgramInit(&program);shaderProgramSetVsh(&program,binary->DVLE);
    if(shaderInstanceGetUniformLocation(program.vertexShader,"points")!=7)
        fh_platform_panic("PICA lightning uniform ABI");
}
void fh_pica_lightning_load(void){
    if(geometry)return;
    extern u32 __ctru_linear_heap_size;
    if(linearSpaceFree()<FH_LIGHTNING_STORAGE+__ctru_linear_heap_size*15/100)
        return;
    geometry=linearMemAlign(FH_LIGHTNING_STORAGE,128);
    if(!geometry)return;
    memset(geometry,0,FH_LIGHTNING_STORAGE);
    int8_t *v=(int8_t*)geometry;uint16_t *indices=(uint16_t*)(geometry+640);
    static const unsigned corners[6]={0,1,2,2,1,3};
    for(unsigned i=0;i<FH_LIGHTNING_BATCH;i++){
        for(unsigned k=0;k<4;k++){
            v[(i*4+k)*4]=i*2;v[(i*4+k)*4+1]=k/2;
            v[(i*4+k)*4+2]=(k&1)?1:-1;v[(i*4+k)*4+3]=k&1;
        }
        for(unsigned k=0;k<6;k++)indices[i*6+k]=i*4+corners[k];
    }
    fh_gpu_cache_flush(geometry,FH_LIGHTNING_STORAGE);
}
static void flush(void){
    if(!count)return;
    FH_REPLAY_SCOPE(NATIVE_SUBMIT);
    u64 start=svcGetSystemTick();fh_pica_scene_reserve();
    if(!bound){
        fh_pica_scene_invalidate();C3D_BindProgram(&program);
        C3D_LightEnvBind(NULL);C3D_FogGasMode(GPU_NO_FOG,GPU_PLAIN_DENSITY,false);
        C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
        float scale[2];unsigned intensity;
        const C3D_Tex *texture=fh_pica_hud_sampler(page,scale,&intensity);
        if(!texture)fh_platform_panic("PICA lightning image lifetime");
        C3D_TexBind(0,fh_pica_options_texture(texture,FH_GRAPHICS_NO_PARTICLE_TEX));
        C3D_TexEnv *e=C3D_GetTexEnv(0);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_REPLACE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,GPU_TEXTURE0,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        if(intensity)C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        C3D_DepthMap(true,depth[0],depth[1]);C3D_CullFace(GPU_CULL_NONE);
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE,GPU_ONE,GPU_ZERO);
        C3D_SetScissor(GPU_SCISSOR_NORMAL,clip[0],clip[1],clip[2],clip[3]);
        for(unsigned i=0;i<4;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,i,projection[i][0],projection[i][1],projection[i][2],projection[i][3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,4,400,240,1.f/400,1.f/240);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,5,scale[0],scale[1],0,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,6,(rgba>>24)/255.f,((rgba>>16)&255)/255.f,((rgba>>8)&255)/255.f,(rgba&255)/255.f);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_BYTE,4);
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
        if(BufInfo_Add(b,geometry,4,1,0)<0)fh_platform_panic("PICA lightning vertex ABI");
        bound=1;
    }
    for(unsigned i=0;i<count*2;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,7+i,points[i][0],points[i][1],points[i][2],points[i][3]);
    C3D_DrawElements(GPU_TRIANGLES,count*6,C3D_UNSIGNED_SHORT,geometry+640);
    fh_pica_hud_account(count*6);draws++;segments+=count;count=0;
    submit_ticks+=svcGetSystemTick()-start;
}
void fh_pica_lightning_begin(const float v[3][4],const float p[4][4],unsigned image,uint32_t color,const unsigned scissor[4]){
    if(active || count)fh_platform_panic("PICA lightning begin");
    active=1;have_previous=0;bound=0;
    if(!(fh_graphics_options&FH_GRAPHICS_PARTICLES) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER)){disabled++;return;}
    if(!geometry || !image){active=2;disabled++;return;}
    fh_pica_hud_flush();if(!fh_pica_hud_target(scissor,clip)){active=2;return;}
    float viewport[6],adjusted[4][4];fh_pica_live_viewport(viewport);
    fh_pica_viewport_xy(adjusted,p,viewport);fh_pica_viewport_depth(depth,viewport[4],viewport[5]);
    for(unsigned i=0;i<4;i++)for(unsigned j=0;j<4;j++){
        float value=j==3?adjusted[i][3]:0;
        for(unsigned k=0;k<3;k++)value+=adjusted[i][k]*v[k][j];
        projection[i][j]=value;
    }
    page=image;rgba=color;bolts++;
}
void fh_pica_lightning_strand(unsigned value){
    if(!active || value>255)fh_platform_panic("PICA lightning strand");
    width=value;have_previous=0;
}
void fh_pica_lightning_point(float x,float y,float z){
    if(!active)fh_platform_panic("PICA lightning point");
    if(!(fh_graphics_options&FH_GRAPHICS_PARTICLES) || (fh_graphics_options&FH_GRAPHICS_NO_RENDER) || active==2)return;
    if(!isfinite(x) || !isfinite(y) || !isfinite(z)){have_previous=0;invalid_points++;return;}
    if(have_previous){
        if(count==FH_LIGHTNING_BATCH)flush();
        float *a=points[count*2],*b=a+4;
        memcpy(a,previous,sizeof(previous));a[3]=width/12.f; /* GX sixth-pixels * min(400/640,240/480). */
        b[0]=x;b[1]=y;b[2]=z;b[3]=0;count++;
    }
    previous[0]=x;previous[1]=y;previous[2]=z;have_previous=1;
}
void fh_pica_lightning_end(void){
    if(!active)fh_platform_panic("PICA lightning end");
    flush();active=0;
    if(bound)fh_pica_scene_invalidate();
}
void fh_pica_lightning_shutdown(void){
    if(active || count)fh_platform_panic("PICA lightning pending shutdown");
    linearFree(geometry);geometry=NULL;shaderProgramFree(&program);DVLB_Free(binary);binary=NULL;
    char line[224];snprintf(line,sizeof(line),"PICA lightning: bolts=%u draws=%u segments=%u disabled=%u invalid_points=%u linear_bytes=%u submit_ticks=%llu CPU_corners=0\n",
        bolts,draws,segments,disabled,invalid_points,FH_LIGHTNING_STORAGE,(unsigned long long)submit_ticks);fh_platform_log(line);
}
