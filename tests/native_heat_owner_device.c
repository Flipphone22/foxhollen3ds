/* Actual native capture/filter owners and material. Synthetic scene inputs;
 * no game progression, whole-scene fidelity or physical timing claim. */
#define FH_TEST_BLUR_OWNER 1
#define FH_TEST_HEAT_OWNER 1
#define main motion_fixture_main
#include "native_motion_blur_device.c"
#undef main
#include "fh_pica_depth_view.h"
#include "../native-3ds/source/platform_pica_blur_material.c"
#include "../native-3ds/source/platform_pica_blur_filter.h"
#include "../native-3ds/source/pica_heat_plan.c"
#include "../native-3ds/source/platform_pica_heat_material.c"
#include "../native-3ds/source/platform_pica_heat.h"
int main(void){
 gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fh_platform_panic("Blur init");
 DVLB_s*ui=DVLB_ParseFile((u32*)minimal_ui_shader,sizeof(minimal_ui_shader));
 if(!ui)fh_platform_panic("Blur programs");
 shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,ui->DVLE+MOTION_SHADER_ENTRY);
 target=native_surface_create();
 enum {CAPTURE=8*24,DISPLAY=CAPTURE+6,BACKGROUND=DISPLAY+6,MARKERS=BACKGROUND+6,COUNT=MARKERS+8*6};
 patterns=linearMemAlign(COUNT*sizeof(*patterns),128);if(!patterns)fh_platform_panic("Blur vertices");
 const unsigned rgb[4][3]={{224,32,64},{32,208,96},{48,64,224},{224,208,32}};
 for(unsigned p=0;p<8;p++)for(unsigned q=0;q<4;q++){
  NativeSurfaceVertex*v=patterns+p*24+q*6;
  quad(v,(q%2)*.5f,(q/2)*.5f,(q%2)*.5f+.5f,(q/2)*.5f+.5f,rgb[q][0],rgb[q][1],rgb[q][2]);
  unsigned source_depth=127+q+(p==2?5:p>=3?2:0);
  for(unsigned k=0;k<6;k++)v[k].position[2]=-((255-source_depth)+.5f)/256;
 }
 quad(patterns+DISPLAY,0,0,1,1,255,255,255);
 const unsigned corner[6]={0,1,2,0,2,3};const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
 for(unsigned i=0;i<6;i++){
  float x=xy[corner[i]][0],y=xy[corner[i]][1];
  patterns[DISPLAY+i].uv[0]=x;patterns[DISPLAY+i].uv[1]=y;
  patterns[CAPTURE+i]=patterns[DISPLAY+i];
  patterns[CAPTURE+i].position[0]=2*x-1;patterns[CAPTURE+i].position[1]=1-2*y;
 }
 quad(patterns+BACKGROUND,0,0,1,1,32,64,96);
 for(unsigned p=0;p<8;p++)quad(patterns+MARKERS+p*6,0,0,8.f/400,8.f/240,20+p,1,2);
 fh_gpu_cache_flush(patterns,COUNT*sizeof(*patterns));
#ifdef FH_TEST_ADMISSION
 check_post_admission(BACKGROUND);
#endif
 unsigned initial_motion_captures=motion_captures;
 for(unsigned p=0;p<8;p++){
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);
  pattern(p*24,24);C3D_DepthTest(true,GPU_ALWAYS,GPU_WRITE_ALL);
  C3D_DrawArrays(GPU_TRIANGLES,p*24,24);
  fh_graphics_options=(p==0||p==4)?0:FH_GRAPHICS_HEAT;
  if(p==7)fh_graphics_options|=FH_GRAPHICS_MOTION_BLUR;
  unsigned before_draws=auxiliary_draws;
  fh_pica_motion_blur_step(0,0,p==0||p==1||p==3||p==4||p==6||p==7);
  if(auxiliary_draws-before_draws!=(unsigned)(p==1||p==3||p==6||p==7))
   fh_platform_panic("Combined capture must publish exactly one draw");
  pattern(BACKGROUND,6);
  const float projection[4][4]={{2.f/640,0,0,-1},{0,-2.f/480,0,1},{0,0,-1.f/99,-100.f/99},{0,0,0,1}};
  if(p==7){
   /* Source LESS must preserve geometry closer than the fullscreen quad. */
   C3D_DepthMap(true,0,.99f);C3D_DepthTest(true,GPU_ALWAYS,GPU_WRITE_DEPTH);C3D_DrawArrays(GPU_TRIANGLES,BACKGROUND,6);
  }
  if(p==1){
   unsigned reserve=__ctru_linear_heap_size*15/100;
   unsigned held_bytes=linearSpaceFree()-(HEAT_BYTES+reserve-128);
   void *held=linearAlloc(held_bytes);if(!held)fh_platform_panic("Heat admission fixture allocation");
   fh_pica_heat(0,0,0,255,128,projection);
   if(heat_mesh || !(fh_graphics_unavailable_mask&FH_GRAPHICS_HEAT))fh_platform_panic("Heat reserve not enforced");
   linearFree(held);fh_graphics_options=0;fh_graphics_unavailable_mask=0;
   fh_graphics_options=FH_GRAPHICS_HEAT;
  }
  fh_pica_heat(.0137f,.0173f,0,255,p>=6?0:128+(p>=3?2:0),projection);
  if(blur_partial_target)fh_platform_panic("Heat must not allocate blur scratch");
  pattern(MARKERS+p*6,6);native_surface_present();C3D_FrameEnd(GX_CMDLIST_FLUSH);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);gspWaitForVBlank();gspWaitForVBlank();
  char name[128];snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ready-%u.txt",p);
  FILE*f=fopen(name,"w");if(!f)fh_platform_panic("Blur ready");fprintf(f,"owner case=%u\n",p);fclose(f);
  snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ack-%u.txt",p);u64 started=svcGetSystemTick();
  while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-started>10ULL*SYSCLOCK_ARM11)fh_platform_panic("Blur capture timeout");svcSleepThread(10000000);}fclose(f);
 }
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
 if(blur_captures!=4||heat_requests!=7||heat_unseeded!=1||heat_draws!=10||motion_captures-initial_motion_captures!=4)
  fh_platform_panic("Blur owner counters");
 heat_shutdown();blur_shutdown();motion_shutdown();
 linearFree(patterns);shaderProgramFree(&ui_program);DVLB_Free(ui);
 C3D_RenderTargetDelete(target);native_surface_destroy();C3D_Fini();gfxExit();
 fh_platform_log("PASS actual heat owner: stale depth, toggles, shared capture, foreground; CPU pixels=0\n");
 FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("PASS 8 cases\n",f);fclose(f);}return 0;
}
