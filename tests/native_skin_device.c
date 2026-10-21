/* Execute both real PICA programs with identical skinning/light inputs. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skin_before.h"
#include "skin_after.h"
#include "skin_cases.h"
#include "fh_native_normal_palette.h"
u32 __ctru_heap_size=8*1024*1024,__ctru_linear_heap_size=8*1024*1024;
void fh_early_queue_capacity_completed(u64 wait_ticks){(void)wait_ticks;}
typedef struct {float p[4],c[4],uv[2],n[3],skin[3];} Vertex;
static void fail(const char *s){FILE*f=fopen("sdmc:/3ds/foxhollow/panic.txt","w");if(f){fputs(s,f);fclose(f);}exit(1);}
int main(void){
    gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
    if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fail("C3D init");
    C3D_RenderTarget *target=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,GPU_RB_DEPTH24_STENCIL8);
    if(!target)fail("Target");
    C3D_RenderTargetSetOutput(target,GFX_TOP,GFX_LEFT,GX_TRANSFER_FLIP_VERT(0)|GX_TRANSFER_OUT_TILED(0)|
        GX_TRANSFER_RAW_COPY(0)|GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8)|GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));
    DVLB_s *shaders[2]={DVLB_ParseFile((u32*)skin_before,sizeof(skin_before)),DVLB_ParseFile((u32*)skin_after,sizeof(skin_after))};
    shaderProgram_s programs[2];
    for(unsigned i=0;i<2;i++){if(!shaders[i])fail("Shader");shaderProgramInit(programs+i);shaderProgramSetVsh(programs+i,shaders[i]->DVLE);}
    Vertex *vertices=linearAlloc(6*sizeof(Vertex));if(!vertices)fail("Vertices");
    const unsigned ix[6]={0,1,2,0,2,3};
    for(unsigned pass=0;pass<SKIN_CASES*3;pass++){
        const SkinCase *c=skin_cases+pass/3;
        for(unsigned i=0;i<6;i++){
            Vertex *v=vertices+i;memset(v,0,sizeof(*v));memcpy(v->p,c->positions[ix[i]],12);v->p[3]=1;
            for(unsigned j=0;j<4;j++)v->c[j]=1;
            memcpy(v->n,c->normal,12);v->skin[0]=0;v->skin[1]=c->same?0:3;v->skin[2]=c->weight;
        }
        GSPGPU_FlushDataCache(vertices,6*sizeof(Vertex));
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameDrawOn(target);
        C3D_RenderTargetClear(target,C3D_CLEAR_ALL,0x142030ff+(pass<<24),0);
        C3D_BindProgram(programs+(pass%3!=0));
        C3D_AttrInfo*a=C3D_GetAttrInfo();AttrInfo_Init(a);
        AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);AttrInfo_AddLoader(a,2,GPU_FLOAT,2);
        AttrInfo_AddLoader(a,3,GPU_FLOAT,3);AttrInfo_AddLoader(a,4,GPU_FLOAT,3);
        for(unsigned i=5;i<12;i++){AttrInfo_AddFixed(a,i);C3D_FixedAttribSet(i,0,0,0,0);}
        C3D_BufInfo*b=C3D_GetBufInfo();BufInfo_Init(b);
        if(BufInfo_Add(b,vertices,sizeof(Vertex),5,0x43210)<0)fail("Binding");
        for(unsigned i=0;i<2;i++)for(unsigned r=0;r<3;r++){
            const float *m=c->matrices[i][r];C3D_FVUnifSet(GPU_VERTEX_SHADER,i*3+r,m[0],m[1],m[2],0);
        }
        float palette[FH_MODEL_BONES][3][4]={0};
        for(unsigned i=0;i<2;i++)for(unsigned r=0;r<3;r++)memcpy(palette[i][r],c->matrices[i][r],12);
        unsigned normal_palette=pass%3==2 && fh_native_normal_palette(palette,2);
        if(normal_palette)for(unsigned i=0;i<2;i++)for(unsigned r=0;r<3;r++){
            const float *m=palette[11+i][r];C3D_FVUnifSet(GPU_VERTEX_SHADER,33+i*3+r,m[0],m[1],m[2],0);
        }
        /* Display rotation, matching the game's top-screen orientation. */
        const float projection[4][4]={{0,-1,0,0},{1,0,0,0},{0,0,1,0},{0,0,0,1}};
        for(unsigned r=0;r<4;r++)C3D_FVUnifSet(GPU_VERTEX_SHADER,66+r,projection[r][0],projection[r][1],projection[r][2],projection[r][3]);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,70,1,1,1,1);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,71,1,1,0,0);C3D_FVUnifSet(GPU_VERTEX_SHADER,72,1,1,0,0);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,73,1,1,1,1);
        for(unsigned i=0;i<16;i++)C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68+i,false);
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x77,normal_palette);
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x68,true);
        C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x76,c->sphere);
        for(unsigned i=0;i<8;i++){
            C3D_BoolUnifSet(GPU_VERTEX_SHADER,0x69+i,i<3);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,74+i,i==0?1e8f:0,i==1?1e8f:0,i==2?1e8f:0,0);
            C3D_FVUnifSet(GPU_VERTEX_SHADER,82+i,i==0?.25f:0,i==1?.25f:0,i==2?.25f:0,0);
        }
        C3D_FVUnifSet(GPU_VERTEX_SHADER,90,.5,.5,.5,1);C3D_FVUnifSet(GPU_VERTEX_SHADER,91,1,1,1,1);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,92,0,0,0,0);C3D_FVUnifSet(GPU_VERTEX_SHADER,93,0,0,0,1);C3D_FVUnifSet(GPU_VERTEX_SHADER,94,1,0,0,0);
        for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
        C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_Both,GPU_REPLACE);
        C3D_TexEnvBufUpdate(C3D_Both,0);C3D_TexBind(0,NULL);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
        C3D_FogGasMode(GPU_NO_FOG,0,false);C3D_CullFace(GPU_CULL_NONE);C3D_AlphaTest(false,GPU_ALWAYS,0);
        C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
        C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_DrawArrays(GPU_TRIANGLES,0,6);
        C3D_FrameEnd(0);C3D_FrameBegin(0);C3D_FrameEnd(0);gspWaitForVBlank();gspWaitForVBlank();
        char name[100];snprintf(name,sizeof(name),"sdmc:/3ds/foxhollow/ready-%u.txt",pass);FILE*f=fopen(name,"w");if(!f)fail("Ready");fprintf(f,"normal=%u\n",normal_palette);fclose(f);
        snprintf(name,sizeof(name),"sdmc:/3ds/foxhollow/ack-%u.txt",pass);u64 start=svcGetSystemTick();
        while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-start>10ULL*SYSCLOCK_ARM11)fail("Capture timeout");svcSleepThread(10000000);}fclose(f);
    }
    FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("done\n",f);fclose(f);}
    while(aptMainLoop())svcSleepThread(10000000);
    return 0;
}
