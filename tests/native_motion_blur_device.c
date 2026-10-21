/* Actual production temporal owner with spatially distinctive synthetic scenes.
 * Observe the presented LCD; no raw GPU/depth readback. Not a game FPS test. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_graphics_options.h"
#include "minimal_ui_shader.h"
#ifndef MOTION_SHADER_ENTRY
#define MOTION_SHADER_ENTRY 0
#endif
u32 __ctru_heap_size=20*1024*1024,__ctru_linear_heap_size=8*1024*1024;
unsigned fh_graphics_options,fh_graphics_unavailable_mask,fh_graphics_unavailable_seen;
void fh_graphics_options_reject(unsigned mask){fh_graphics_unavailable_mask|=mask&fh_graphics_options;fh_graphics_unavailable_seen|=mask&fh_graphics_options;}
static unsigned initialized=1;
static shaderProgram_s ui_program;
static C3D_RenderTarget *target;
#define FH_REPLAY_SCOPE(x) ((void)0)
void fh_platform_panic(const char*s){FILE*f=fopen("sdmc:/3ds/foxhollow/panic.txt","w");if(f){fputs(s,f);fclose(f);}exit(1);}
static void fh_platform_log(const char*s){FILE*f=fopen("sdmc:/3ds/foxhollow/owner.txt","a");if(f){fputs(s,f);fclose(f);}}
static void fh_gpu_cache_flush(const void*p,size_t n){GSPGPU_FlushDataCache(p,n);}
static void fh_native_fog_apply(unsigned enabled){C3D_FogGasMode(GPU_NO_FOG,0,false);}
void fh_early_queue_capacity_completed(u64 t){(void)t;}
void fh_pica_fog_invalidate(void){}
static void fh_pica_texture2_coord1(unsigned enabled){if(enabled)fh_platform_panic("Unexpected coord owner");}
#include "../native-3ds/source/platform_native_surface.h"
void fh_minimal_auxiliary_begin(void){}
static unsigned auxiliary_draws;
void fh_minimal_auxiliary_end(unsigned draws,unsigned indices){if(indices!=draws*6 && indices!=107520)fh_platform_panic("Draw accounting");auxiliary_draws+=draws;native_surface_draw_on(target);}
#ifndef FH_TEST_HEAT_OWNER
unsigned fh_pica_heat_enabled(void){return 0;}
#endif
#ifndef FH_TEST_BLUR_OWNER
static unsigned blur_capture(unsigned capture){(void)capture;return 0;}
#endif
#include "../native-3ds/source/platform_pica_motion_blur.h"
static NativeSurfaceVertex *patterns;
static void quad(NativeSurfaceVertex *out,float x0,float y0,float x1,float y1,unsigned red,unsigned green,unsigned blue){
 const unsigned corner[6]={0,1,2,0,2,3};const float xy[4][2]={{x0,y0},{x0,y1},{x1,y1},{x1,y0}};
 for(unsigned i=0;i<6;i++){
  float x=xy[corner[i]][0],y=xy[corner[i]][1];
  out[i]=(NativeSurfaceVertex){.position={1-2*y,1-2*x,-.5f,1},.color={red/255.f,green/255.f,blue/255.f,1}};
 }
}
static void pattern(unsigned first,unsigned count){
 C3D_BindProgram(&ui_program);C3D_LightEnvBind(NULL);fh_native_fog_apply(0);
 C3D_AttrInfo*a=C3D_GetAttrInfo();AttrInfo_Init(a);AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);AttrInfo_AddLoader(a,2,GPU_FLOAT,2);
 C3D_BufInfo*b=C3D_GetBufInfo();BufInfo_Init(b);if(BufInfo_Add(b,patterns,40,3,0x210)<0)fh_platform_panic("Pattern layout");
 C3D_FVUnifSet(GPU_VERTEX_SHADER,0,1,1,0,0);C3D_FVUnifSet(GPU_VERTEX_SHADER,1,1,1,0,0);
 C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);C3D_DepthMap(true,-1,0);
 C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
 C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
 C3D_TexEnvBufUpdate(C3D_Both,0);for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
 C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_PRIMARY_COLOR,0,0);C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_Both,GPU_REPLACE);
 C3D_TexBind(0,NULL);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);C3D_DrawArrays(GPU_TRIANGLES,first,count);
}
int main(void){
 gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fh_platform_panic("GPU init");
 DVLB_s*binary=DVLB_ParseFile((u32*)minimal_ui_shader,sizeof(minimal_ui_shader));if(!binary)fh_platform_panic("Shader parse");
 shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,binary->DVLE+MOTION_SHADER_ENTRY);target=native_surface_create();
 patterns=linearMemAlign((3*24+8*6)*sizeof(*patterns),128);if(!patterns)fh_platform_panic("Patterns allocation");
 const unsigned rgb[4][3]={{224,32,64},{32,208,96},{48,64,224},{224,208,32}};
 for(unsigned p=0;p<3;p++)for(unsigned q=0;q<4;q++){
  unsigned red=p==0?rgb[q][0]:p==1?32:96,green=p==0?rgb[q][1]:p==1?64:96,blue=p==0?rgb[q][2]:p==1?96:96;
  quad(patterns+p*24+q*6,(q%2)*.5f,(q/2)*.5f,(q%2)*.5f+.5f,(q/2)*.5f+.5f,red,green,blue);
 }
 for(unsigned i=0;i<8;i++)quad(patterns+72+i*6,0,0,8.f/400,8.f/240,20+i,1,2);
 fh_gpu_cache_flush(patterns,(3*24+8*6)*sizeof(*patterns));
 for(unsigned pass=0;pass<8;pass++){
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);
  unsigned p=pass<2||pass==6?0:pass==2||pass==7?1:2;pattern(p*24,24);
  fh_graphics_options=(pass==0||pass==4)?0:FH_GRAPHICS_MOTION_BLUR;
  fh_pica_motion_blur_step(pass==2?.5f:1.f,1,pass==0||pass==1||pass==2||pass==6);
  pattern(72+pass*6,6);native_surface_present();C3D_FrameEnd(0);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(0);gspWaitForVBlank();gspWaitForVBlank();
  char name[128];snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ready-%u.txt",pass);
  FILE*f=fopen(name,"w");if(!f)fh_platform_panic("Ready file");fprintf(f,"case=%u captures=%u draws=%u\n",pass,motion_captures,motion_draws);fclose(f);
  snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ack-%u.txt",pass);u64 started=svcGetSystemTick();
  while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-started>10ULL*SYSCLOCK_ARM11)fh_platform_panic("Capture timeout");svcSleepThread(10000000);}fclose(f);
 }
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(0);
 if(motion_captures!=3||motion_draws!=3)fh_platform_panic("Unexpected owner counters");
 motion_shutdown();linearFree(patterns);shaderProgramFree(&ui_program);DVLB_Free(binary);
 C3D_RenderTargetDelete(target);native_surface_destroy();C3D_Fini();gfxExit();
 FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("PASS 8 cases\n",f);fclose(f);}return 0;
}
