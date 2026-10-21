/* Exercise real allocator pressure without submitting pressure buffers to GPU.
 * Existing temporal cases then prove successful retry and unchanged pixels. */
static void check_post_admission(unsigned background){
 for(unsigned scenario=0;scenario<3;scenario++){
  unsigned old_vram=vramSpaceFree(),old_linear=linearSpaceFree();
  void*pressure[32]={0};unsigned count=0;
  while((scenario==1?linearSpaceFree():vramSpaceFree())>1024*1024){
   if(count==32)fh_platform_panic("Admission fixture pressure cap");
   void*p=scenario==1?linearAlloc(256*1024):vramAlloc(256*1024);
   if(!p)break;
   pressure[count++]=p;
  }
  fh_graphics_options=FH_GRAPHICS_MOTION_BLUR|FH_GRAPHICS_BLUR_FILTER;
  fh_graphics_unavailable_mask=0;
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);pattern(background,6);
  unsigned prior_motion=motion_captures;
  fh_pica_motion_blur_step(0,0,1);
  unsigned expected=scenario==2?FH_GRAPHICS_BLUR_FILTER:fh_graphics_options;
  if(fh_graphics_unavailable_mask!=expected||blur_partial_target||blur_partial.data||
     (!!blur_quads!=(scenario==2))|| (!!blur_binary!=(scenario==2)))
   fh_platform_panic("Admission fixture expected denial");
  for(unsigned repeat=0;repeat<10;repeat++){
   fh_pica_motion_blur_step(0,0,0);fh_pica_blur_filter(-.5f,1,0);
  }
  if(motion_captures!=prior_motion+(scenario==2))fh_platform_panic("Admission fixture capture retry");
  native_surface_present();C3D_FrameEnd(GX_CMDLIST_FLUSH);
  C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
  for(unsigned i=0;i<count;i++)if(scenario==1)linearFree(pressure[i]);else vramFree(pressure[i]);
  if(vramSpaceFree()!=old_vram||linearSpaceFree()!=old_linear-(scenario==2?BLUR_LINEAR_BYTES:0))fh_platform_panic("Admission pressure leaked storage");
  fh_graphics_unavailable_mask=0;
  /* Seed motion history before scenario2 so the distinct blur owner, rather
   * than the shared colour owner, encounters the real VRAM reserve failure. */
  if(scenario==1){
   fh_graphics_options=FH_GRAPHICS_MOTION_BLUR;
   C3D_FrameBegin(C3D_FRAME_SYNCDRAW);native_surface_draw_on(target);pattern(background,6);
   fh_pica_motion_blur_step(0,0,1);C3D_FrameEnd(GX_CMDLIST_FLUSH);
   C3D_FrameBegin(C3D_FRAME_SYNCDRAW);C3D_FrameEnd(GX_CMDLIST_FLUSH);
  }
 }
 fh_graphics_unavailable_mask=fh_graphics_unavailable_seen=0;
 fh_platform_log("PASS actual VRAM/linear/shared-owner denial; rendering continues, storage preserved\n");
}
