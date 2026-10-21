/* Isolated real-PICA material fixture; no game, audio or desktop input. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "decal_shader.h"
#include "../native-3ds/wip/source/platform_native_decal.h"
u32 __ctru_heap_size=8*1024*1024,__ctru_linear_heap_size=8*1024*1024;
void fh_early_queue_capacity_completed(u64 wait_ticks){(void)wait_ticks;} /* No early queues. */
typedef struct {float p[4],c[4],uv[4];} Vertex;
static void fail(const char *s){FILE*f=fopen("sdmc:/3ds/foxhollow/panic.txt","w");if(f){fputs(s,f);fclose(f);}exit(1);}
int main(void){
    gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fail("C3D init");
    DVLB_s *bin=DVLB_ParseFile((u32*)decal_shader,sizeof(decal_shader));
    shaderProgram_s program;shaderProgramInit(&program);shaderProgramSetVsh(&program,&bin->DVLE[0]);
    C3D_RenderTarget *target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    Vertex *v=linearAlloc(4*sizeof(*v));u16 *ix=linearAlloc(12);C3D_Tex t[3];
    if(!target || !v || !ix)fail("allocation");
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,0);
    const u16 ids[6]={0,1,2,0,2,3};memcpy(ix,ids,12);
    for(unsigned i=0;i<4;i++){
        v[i]=(Vertex){{i==1||i==2?.75f:-.75f,i>=2?.75f:-.75f,-.5f,1},
            {.4f,.5f,.6f,.1f},{i==1||i==2?1:0,i>=2?1:0,0,1}};
    }
    GSPGPU_FlushDataCache(v,4*sizeof(*v));GSPGPU_FlushDataCache(ix,12);
    for(unsigned i=0;i<3;i++){
        if(!C3D_TexInit(t+i,8,8,GPU_RGBA8))fail("texture");
        C3D_TexSetFilter(t+i,GPU_NEAREST,GPU_NEAREST);
        C3D_TexSetWrap(t+i,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    }
    for(unsigned pass=0;pass<7;pass++){
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,((20+pass)<<24)|0x2030ff,0);
        C3D_FrameDrawOn(target);C3D_BindProgram(&program);
        C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
        for(unsigned i=0;i<3;i++)AttrInfo_AddLoader(a,i,GPU_FLOAT,4);
        C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);BufInfo_Add(b,v,sizeof(*v),3,0x210);
        const unsigned alpha=pass==0?255:pass==1||pass==2?0:128,mask=pass==1?0:pass==2?255:96;
        const u8 colors[3][4]={{180,120,60,alpha},{64,128,192,99},{230,mask,5,13}};
        for(unsigned j=0;j<3;j++){
            for(unsigned i=0;i<64;i++)for(unsigned c=0;c<4;c++)((u8*)t[j].data)[i*4+3-c]=colors[j][c];
            C3D_TexFlush(t+j);C3D_TexBind(j,t+j);
        }
        if(pass==4)decal_initialized=0;
        float phase[2]={pass==5?.75f:0,pass==5?.25f:0};
        model_decal_noise(pass,phase);
        if(pass<4){
            u32 color[128];for(unsigned i=0;i<128;i++)color[i]=0xffc86450u;
            ProcTexColorLut_Write(&decal_colors,color,0,128);C3D_ProcTexColorLutBind(&decal_colors);
        }
        model_decal_env(pass==3,128,pass==3?192:255);
        if(pass==6){
            C3D_ProcTexBind(0,NULL);C3D_TexEnvBufUpdate(C3D_Both,0);
            for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
            C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_TEXTURE0,0,0);
        }
        C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,pass==3?GPU_SRC_ALPHA:GPU_ONE,
            pass==3?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
        C3D_DrawElements(GPU_TRIANGLES,6,C3D_UNSIGNED_SHORT,ix);
        C3D_ProcTexBind(0,NULL);C3D_FrameEnd(GX_CMDLIST_FLUSH);
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
        char path[100];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/ready-%u.txt",pass);
        FILE*f=fopen(path,"w");if(!f)fail("ready");fputs("ready\n",f);fclose(f);
        snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/ack-%u.txt",pass);
        unsigned count=0;while(!(f=fopen(path,"r")) && aptMainLoop() && ++count<1800)gspWaitForVBlank();
        if(!f)fail("ack timeout");fclose(f);
    }
    FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("passes=7\n",f);fclose(f);}
    for(unsigned i=0;i<3;i++)C3D_TexDelete(t+i);
    linearFree(v);linearFree(ix);C3D_RenderTargetDelete(target);
    shaderProgramFree(&program);DVLB_Free(bin);C3D_Fini();gfxExit();return 0;
}
