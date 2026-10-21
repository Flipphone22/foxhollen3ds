/* Actual sheet shader/material/owner and native scene capture. Synthetic pixels
 * isolate projective routing, opacity and handoff; not original-game parity. */
#define main motion_fixture_main
#include "native_motion_blur_device.c"
#undef main
#include "fh_pica_water_sheet.h"
#include "pica_water_sheet_shader.h"
#ifdef FH_TEST_WATER_GRID
#include "water_grid_private.h"
#include "../native-3ds/source/pica_water_grid.c"
#endif
static C3D_Tex fixture_mask;
static unsigned mask_refs,reserves,accounted;
unsigned fh_pica_image_acquire(const FhTexture*t){(void)t;mask_refs++;return 1;}
void fh_pica_image_release(unsigned id){if(id){if(id!=1||!mask_refs)fh_platform_panic("Mask reference");mask_refs--;}}
void fh_pica_image_sampler(unsigned id,const FhTexture*t,C3D_Tex*out,float scale[2]){
 (void)t;if(id!=1||!mask_refs)fh_platform_panic("Mask sampler");*out=fixture_mask;scale[0]=scale[1]=1;
}
void fh_pica_hud_flush(void){}
int fh_pica_hud_target(const unsigned s[4],unsigned clip[4]){
 (void)s;clip[0]=clip[1]=0;clip[2]=240;clip[3]=400;return 1;
}
void fh_pica_hud_account(unsigned count){
#ifdef FH_TEST_WATER_GRID
 if(count!=7128)fh_platform_panic("Grid accounting");
#else
 if(count!=6)fh_platform_panic("Sheet accounting");
#endif
 accounted++;
}
void fh_pica_scene_reserve(void){
 /* Exercise an actual command split and a different program/material before
  * EVERY pass, including the second one. The production owner must restore. */
 C3D_FrameSplit(0);reserves++;C3D_BindProgram(&ui_program);
 for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
 for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
 C3D_DepthTest(false,GPU_NEVER,GPU_WRITE_ALL);
}
void fh_pica_scene_invalidate(void){}
void fh_minimal_sync(void){C3D_FrameSplit(0);}
void fh_pica_live_viewport(float v[6]){const float a[]={0,0,640,480,0,1};memcpy(v,a,sizeof a);}
unsigned fh_pica_fog_layered_bind(unsigned enabled,float parameters[2]){
 (void)enabled;parameters[0]=parameters[1]=0;C3D_FVUnifSet(GPU_VERTEX_SHADER,94,0,0,0,0);return 0;
}
int fh_pica_environment_layered_bind(unsigned slot,unsigned base,unsigned fog,const float parameters[2]){
 (void)parameters;if(slot||base||fog)fh_platform_panic("Unexpected fixture fog");return 1;
}
#include "../native-3ds/source/pica_water_material.c"
#include "../native-3ds/source/platform_pica_water_sheet.c"
int main(void){
 gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fh_platform_panic("Sheet GPU init");
 DVLB_s*ui=DVLB_ParseFile((u32*)minimal_ui_shader,sizeof(minimal_ui_shader));
 DVLB_s*sheet=DVLB_ParseFile((u32*)pica_water_sheet_shader,sizeof(pica_water_sheet_shader));
 if(!ui||!sheet)fh_platform_panic("Sheet programs");
 shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,ui->DVLE+MOTION_SHADER_ENTRY);
 target=native_surface_create();fh_pica_water_sheet_init(sheet->DVLE);
#ifdef FH_TEST_WATER_GRID
 if(!C3D_TexInit(&fixture_mask,32,64,GPU_RGBA8))fh_platform_panic("Grid mask");
 memcpy(fixture_mask.data,water_grid_rgba,sizeof water_grid_rgba);
#else
 if(!C3D_TexInit(&fixture_mask,8,8,GPU_RGBA8))fh_platform_panic("Fixture mask");
 for(unsigned i=0;i<64;i++)((u32*)fixture_mask.data)[i]=0xffffff80u;
#endif
 C3D_TexSetFilter(&fixture_mask,GPU_LINEAR,GPU_LINEAR);C3D_TexSetWrap(&fixture_mask,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
 fh_gpu_cache_flush(fixture_mask.data,fixture_mask.size);
 const int16_t positions[4][3]={{0,0,0},{0,480,0},{640,480,0},{640,0,0}};
 const int16_t uv[4][2]={{0,0},{0,128},{128,128},{128,0}};
 const uint16_t indices[]={0,1,2,0,2,3};FhTexture mask={.format=6};
#ifdef FH_TEST_WATER_GRID
 mask=(FhTexture){.pixels=water_grid_source,.width=32,.height=64,.format=5};
#endif
 if(!fh_pica_water_sheet_load(positions,uv,indices,&mask))fh_platform_panic("Sheet load");
 enum {BACKGROUND=24,MARKERS=30,COUNT=MARKERS+8*6};
 patterns=linearMemAlign(COUNT*sizeof(*patterns),128);if(!patterns)fh_platform_panic("Sheet patterns");
 const unsigned rgb[4][3]={{224,32,64},{32,208,96},{48,64,224},{224,208,32}};
 for(unsigned q=0;q<4;q++)quad(patterns+q*6,(q%2)*.5f,(q/2)*.5f,(q%2)*.5f+.5f,(q/2)*.5f+.5f,rgb[q][0],rgb[q][1],rgb[q][2]);
 quad(patterns+BACKGROUND,0,0,1,1,32,64,96);
 for(unsigned p=0;p<8;p++)quad(patterns+MARKERS+p*6,0,0,8.f/400,8.f/240,20+p,1,2);
 fh_gpu_cache_flush(patterns,COUNT*sizeof(*patterns));
 const float view[3][4]={{1,0,0,0},{0,1,0,0},{0,0,1,-10}};
 const float projection[4][4]={{2.f/640,0,0,-1},{0,-2.f/480,0,1},{0,0,0,-.5f},{0,0,0,1}};
 const unsigned scissor[]={0,0,640,480};
 for(unsigned p=0;p<8;p++){
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);
  pattern(0,24);C3D_Tex *capture=native_surface_capture(target,&ui_program);
  pattern(BACKGROUND,6);
  /* Reset depth explicitly so the foreground-occlusion case is isolated. */
  C3D_DepthMap(true,0,p==7?.99f:0.f);C3D_DepthTest(true,GPU_ALWAYS,GPU_WRITE_DEPTH);C3D_DrawArrays(GPU_TRIANGLES,BACKGROUND,6);
  float reflected[2][3][4]={{{1.f/640,0,0,0},{0,-1.f/480,0,1},{0,0,0,1}},{{-1.f/640,0,0,1},{0,-1.f/480,0,1},{0,0,0,1}}};
  if(p==6)for(unsigned k=0;k<2;k++)for(unsigned row=0;row<3;row++)for(unsigned c=0;c<4;c++)reflected[k][row][c]*=2;
  const u8 tint[]={16,24,32,p==3?64:p==5?0:255};
  unsigned mix=p==0?0:p==1?0xffffffu:0xc08040u;
  if(fh_pica_water_sheet_draw(capture,view,projection,reflected,scissor,tint,mix,p==4)!=2)fh_platform_panic("Sheet draws");
  pattern(MARKERS+p*6,6);native_surface_present();C3D_FrameEnd(GX_CMDLIST_FLUSH);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);gspWaitForVBlank();gspWaitForVBlank();
  char name[128];snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ready-%u.txt",p);
  FILE*f=fopen(name,"w");if(!f)fh_platform_panic("Sheet ready");fprintf(f,"sheet case=%u\n",p);fclose(f);
  snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ack-%u.txt",p);u64 started=svcGetSystemTick();
  while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-started>10ULL*SYSCLOCK_ARM11)fh_platform_panic("Sheet capture timeout");svcSleepThread(10000000);}fclose(f);
 }
 if(reserves!=16||accounted!=16)fh_platform_panic("Sheet pass count");
 fh_pica_water_sheet_shutdown();if(mask_refs)fh_platform_panic("Sheet image leak");
 C3D_TexDelete(&fixture_mask);linearFree(patterns);shaderProgramFree(&ui_program);DVLB_Free(ui);DVLB_Free(sheet);
 C3D_RenderTargetDelete(target);native_surface_destroy();C3D_Fini();gfxExit();
 fh_platform_log("PASS actual water sheet: 16 split/handoff draws; capture, masks, projective Q, source mix/opacity and foreground\n");
 FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("PASS 8 cases\n",f);fclose(f);}return 0;
}
