/* Actual fixed native two-pass TEV material, borrowed depth and affine shader.
 * Synthetic spatial/depth cases; not production resource ownership or timing. */
#define main motion_fixture_main
#include "native_motion_blur_device.c"
#undef main
#include "fh_pica_depth_view.h"
#include "../native-3ds/source/platform_pica_blur_material.c"
#include "post_sample_shader.h"
static shaderProgram_s post_program;
static FhPicaBlurMaterial material;

static void post_state(unsigned mode,unsigned first){
 C3D_BindProgram(&post_program);C3D_LightEnvBind(NULL);fh_native_fog_apply(0);
 C3D_AttrInfo*a=C3D_GetAttrInfo();AttrInfo_Init(a);
 AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);AttrInfo_AddLoader(a,2,GPU_FLOAT,2);
 C3D_BufInfo*b=C3D_GetBufInfo();BufInfo_Init(b);
 if(BufInfo_Add(b,patterns+first,40,3,0x210)<0)fh_platform_panic("Post quad");
 C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
 C3D_StencilTest(false,GPU_ALWAYS,0,255,255);C3D_AlphaTest(false,GPU_ALWAYS,0);
 C3D_CullFace(GPU_CULL_NONE);C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
 C3D_TexEnvBufUpdate(C3D_Both,0);if(mode)C3D_TexEnvBufUpdate(C3D_Alpha,1);
 for(unsigned i=0;i<6;i++)C3D_SetTexEnv(i,material.stages[mode]+i);
 C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,mode?GPU_SRC_ALPHA:GPU_ONE,
  mode?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ZERO,GPU_ONE);
 for(unsigned i=0;i<3;i++){
  float dx=mode?0:i==1?-1.f/320:i==2?1.f/320:0;
  float dy=mode?0:i==0?1.f/240:0;
  C3D_FVUnifSet(GPU_VERTEX_SHADER,i*2,1,0,dx,0);
  C3D_FVUnifSet(GPU_VERTEX_SHADER,i*2+1,0,-1,1+dy,0);
 }
 if(mode){
  C3D_FVUnifSet(GPU_VERTEX_SHADER,4,0,-239.f/256,239.5f/256,0);
  C3D_FVUnifSet(GPU_VERTEX_SHADER,5,-399.f/512,0,399.5f/512,0);
 }
}
int main(void){
 gfxInitDefault();gfxSetScreenFormat(GFX_TOP,GSP_RGBA8_OES);
 if(!C3D_Init(C3D_DEFAULT_CMDBUF_SIZE))fh_platform_panic("Blur init");
 DVLB_s*ui=DVLB_ParseFile((u32*)minimal_ui_shader,sizeof(minimal_ui_shader));
 DVLB_s*post=DVLB_ParseFile((u32*)post_sample_shader,sizeof(post_sample_shader));
 if(!ui||!post)fh_platform_panic("Blur programs");
 shaderProgramInit(&ui_program);shaderProgramSetVsh(&ui_program,ui->DVLE+MOTION_SHADER_ENTRY);
 shaderProgramInit(&post_program);shaderProgramSetVsh(&post_program,post->DVLE);
 target=native_surface_create();C3D_Tex depth,partial;
 if(!fh_pica_depth_view(&depth,&target->frameBuf,&native_surface_color))fh_platform_panic("Blur depth view");
 if(!C3D_TexInitVRAM(&partial,512,256,GPU_RGBA8))fh_platform_panic("Blur partial");
 C3D_TexSetFilter(&partial,GPU_LINEAR,GPU_LINEAR);C3D_TexSetWrap(&partial,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
 C3D_RenderTarget*partial_target=C3D_RenderTargetCreateFromTex(&partial,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
 if(!partial_target)fh_platform_panic("Blur partial target");
 enum {CAPTURE=8*24,DISPLAY=CAPTURE+6,BACKGROUND=DISPLAY+6,MARKERS=BACKGROUND+6,COUNT=MARKERS+8*6};
 patterns=linearMemAlign(COUNT*sizeof(*patterns),128);if(!patterns)fh_platform_panic("Blur vertices");
 const unsigned rgb[4][3]={{224,32,64},{32,208,96},{48,64,224},{224,208,32}};
 for(unsigned p=0;p<8;p++)for(unsigned q=0;q<4;q++){
  NativeSurfaceVertex*v=patterns+p*24+q*6;
  quad(v,(q%2)*.5f,(q/2)*.5f,(q%2)*.5f+.5f,(q/2)*.5f+.5f,rgb[q][0],rgb[q][1],rgb[q][2]);
  unsigned source_depth=127+q+(p>=4?2:0);
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
 fh_gpu_cache_flush(patterns,COUNT*sizeof(*patterns));fh_pica_blur_material_init(&material,0);
 fh_graphics_options=FH_GRAPHICS_MOTION_BLUR;
 for(unsigned p=0;p<8;p++){
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);
  pattern(p*24,24);C3D_DepthTest(true,GPU_ALWAYS,GPU_WRITE_ALL);
  C3D_DrawArrays(GPU_TRIANGLES,p*24,24);
  fh_pica_motion_blur_step(0,0,1);
  native_surface_split();C3D_FrameDrawOn(partial_target);post_state(0,CAPTURE);
  C3D_TexBind(0,&motion_history);C3D_TexBind(1,&motion_history);C3D_TexBind(2,&motion_history);
  C3D_DrawArrays(GPU_TRIANGLES,0,6);
  native_surface_split();C3D_FrameDrawOn(native_surface_display);pattern(BACKGROUND,6);
  fh_pica_blur_material_threshold(&material,128+(p>=4?2:0));
  unsigned mode=fh_pica_blur_material_mode(p&1,(p>>1)&1);post_state(mode,DISPLAY);
  C3D_TexBind(0,&partial);C3D_TexBind(1,&motion_history);C3D_TexBind(2,&depth);
  C3D_DrawArrays(GPU_TRIANGLES,0,6);
  pattern(MARKERS+p*6,6);C3D_FrameEnd(GX_CMDLIST_FLUSH);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);gspWaitForVBlank();gspWaitForVBlank();
  char name[128];snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ready-%u.txt",p);
  FILE*f=fopen(name,"w");if(!f)fh_platform_panic("Blur ready");fprintf(f,"mode=%u\n",mode);fclose(f);
  snprintf(name,sizeof name,"sdmc:/3ds/foxhollow/ack-%u.txt",p);u64 started=svcGetSystemTick();
  while(!(f=fopen(name,"r"))){if(svcGetSystemTick()-started>10ULL*SYSCLOCK_ARM11)fh_platform_panic("Blur capture timeout");svcSleepThread(10000000);}fclose(f);
 }
 C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
 motion_shutdown();C3D_RenderTargetDelete(partial_target);C3D_TexDelete(&partial);
 linearFree(patterns);shaderProgramFree(&post_program);shaderProgramFree(&ui_program);DVLB_Free(post);DVLB_Free(ui);
 C3D_RenderTargetDelete(target);native_surface_destroy();C3D_Fini();gfxExit();
 fh_platform_log("PASS native two-pass blur material; CPU pixels=0; borrowed depth\n");
 FILE*f=fopen("sdmc:/3ds/foxhollow/done.txt","w");if(f){fputs("PASS 8 cases\n",f);fclose(f);}return 0;
}
