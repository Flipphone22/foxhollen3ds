/* GPU-only depth sampling feasibility. No depth DMA, CPU readback or game hook.
 * Reuse the already checked framebuffer/UI/device harness; the depth texture
 * descriptor is a borrowed view, never a second owner of its storage. */
#define main motion_fixture_main
#include "native_motion_blur_device.c"
#undef main
#include "fh_pica_depth_view.h"

int main(void){
 gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fh_platform_panic("Depth GPU init");
 DVLB_s*binary=DVLB_ParseFile((u32*)minimal_ui_shader,sizeof(minimal_ui_shader));
 if(!binary)fh_platform_panic("Depth shader parse");
 shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,binary->DVLE+MOTION_SHADER_ENTRY);
 target=native_surface_create();
 if(target->frameBuf.depthFmt!=GPU_RB_DEPTH24_STENCIL8 ||
    C3D_CalcDepthBufSize(target->frameBuf.width,target->frameBuf.height,target->frameBuf.depthFmt)!=native_surface_color.size ||
    !addrIsVRAM(target->frameBuf.depthBuf))fh_platform_panic("Depth view layout");
 C3D_Tex depth_view;
 if(!fh_pica_depth_view(&depth_view,&target->frameBuf,&native_surface_color))fh_platform_panic("Borrowed depth view contract");
 enum {SOURCE_VERTICES=8*24,SAMPLE_FIRST=SOURCE_VERTICES,MARKER_FIRST=SAMPLE_FIRST+6,VERTICES=MARKER_FIRST+8*6};
 patterns=linearMemAlign(VERTICES*sizeof(*patterns),128);if(!patterns)fh_platform_panic("Depth patterns");
 const unsigned base[4]={0,1,254,255};
 for(unsigned pass=0;pass<8;pass++)for(unsigned q=0;q<4;q++){
  NativeSurfaceVertex*v=patterns+pass*24+q*6;
  quad(v,(q%2)*.5f,(q/2)*.5f,(q%2)*.5f+.5f,(q/2)*.5f+.5f,0,0,0);
  unsigned value=(base[q]+pass*7)%256;
  float z=pass<6?-(value+.5f)/256.f:(((q+pass)&1)?-1.f:0.f);
  for(unsigned k=0;k<6;k++)v[k].position[2]=z;
 }
 const unsigned corner[6]={0,1,2,0,2,3};const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
 for(unsigned i=0;i<6;i++){
  float x=xy[corner[i]][0],y=xy[corner[i]][1];
  patterns[SAMPLE_FIRST+i]=(NativeSurfaceVertex){.position={1-2*y,1-2*x,-.5f,1},.color={1,1,1,1},
   .uv={(239.5f-239*y)/256.f,(112.5f+399*x)/512.f}};
 }
 for(unsigned pass=0;pass<8;pass++)quad(patterns+MARKER_FIRST+pass*6,0,0,8.f/400,8.f/240,20+pass,1,2);
 fh_gpu_cache_flush(patterns,VERTICES*sizeof(*patterns));
 for(unsigned pass=0;pass<8;pass++){
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);
  pattern(pass*24,24); /* Configure ordinary source state; then write known depth. */
  C3D_DepthTest(true,GPU_ALWAYS,GPU_WRITE_ALL);
  C3D_StencilTest(true,GPU_ALWAYS,17+pass,255,255);
  C3D_StencilOp(GPU_STENCIL_KEEP,GPU_STENCIL_KEEP,GPU_STENCIL_REPLACE);
  C3D_DrawArrays(GPU_TRIANGLES,pass*24,24);
  /* Publish depth writes before switching to the independent display colour
   * target. It has no depth attachment, so there is no read/write feedback. */
  native_surface_split();C3D_FrameDrawOn(native_surface_display);
  pattern(SAMPLE_FIRST,6);
  C3D_TexBind(0,&depth_view);
  C3D_TexEnv*e=C3D_GetTexEnv(0);
  C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,0,0);
  /* D24S8 as RGBA8: R=stencil, G=depth high, B=middle, A=low. */
  C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_ONE_MINUS_SRC_G,0,0);
  C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);C3D_TexEnvColor(e,0xffffffff);
  C3D_DrawArrays(GPU_TRIANGLES,SAMPLE_FIRST,6);
  pattern(MARKER_FIRST+pass*6,6);C3D_FrameEnd(GX_CMDLIST_FLUSH);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
  gspWaitForVBlank();gspWaitForVBlank();
  char name[128];snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ready-%u.txt",pass);
  FILE*f=fopen(name,"w");if(!f)fh_platform_panic("Depth ready");fprintf(f,"depth pass=%u\n",pass);fclose(f);
  snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ack-%u.txt",pass);u64 start=svcGetSystemTick();
  while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-start>10ULL*SYSCLOCK_ARM11)fh_platform_panic("Depth capture timeout");svcSleepThread(10000000);}fclose(f);
 }
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
 /* Borrowed depth_view is NOT deleted: target owns the original allocation. */
 linearFree(patterns);shaderProgramFree(&ui_program);DVLB_Free(binary);
 C3D_RenderTargetDelete(target);native_surface_destroy();C3D_Fini();gfxExit();
 fh_platform_log("PASS 8 D24S8 borrowed-view GPU cases; CPU depth reads=0; depth copies=0\n");
 FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("PASS 8 cases\n",f);fclose(f);}return 0;
}
