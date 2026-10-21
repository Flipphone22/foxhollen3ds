/* Included by platform_graphics.c. Melee gpu_efb_copy adaptation; see licenses.
 * Only the explicitly audited 320x240 persistent reflection image is deferred.
 * Ordinary GXCopyTex retains its immediate CPU-memory contract. */
static struct {
    unsigned enabled,dirty,copies,materializations,capture_owned;
    void *destination;
    C3D_Tex capture,scratch;
    Texture texture;
    C3D_RenderTarget *output,*scratch_target;
    u32 *linear;
} efb_color;
static void efb_color_finish(void){
    if(efb_color.enabled){char line[128];snprintf(line,sizeof(line),"GPU color reflection copies=%u CPU_materializations=%u\n",efb_color.copies,efb_color.materializations);fh_platform_log(line);}
    if(efb_color.output)C3D_RenderTargetDelete(efb_color.output);
    if(efb_color.scratch_target)C3D_RenderTargetDelete(efb_color.scratch_target);
    sampled_capture_storage=NULL;sampled_capture_bytes=0;
    if(efb_color.capture_owned && efb_color.capture.data)C3D_TexDelete(&efb_color.capture);
    if(efb_color.texture.gpu.data)C3D_TexDelete(&efb_color.texture.gpu);
    if(efb_color.scratch.data)C3D_TexDelete(&efb_color.scratch);
    linearFree(efb_color.linear);memset(&efb_color,0,sizeof(efb_color));
}
static void efb_color_init(void){
#ifndef FH_EMULATOR_TEST
    /* Physical validation candidate after exact fixture and matched title tests.
     * The depth path remains on safe CPU DisplayTransfer. */
    efb_color.enabled=1;
#endif
    FILE *f=fopen("sdmc:/3ds/foxhollow/efb_color.txt","rb");
    if(f){efb_color.enabled=fgetc(f)=='1';fclose(f);}
    if(!efb_color.enabled)return;
    /* Match pinned Melee gpu_efb_copy: the padded GPU capture is linear,
     * not VRAM. ARM never reads/writes it after GPU publication. Preserve the
     * shared linear reserve before this optional allocation and readback. */
    if(linearSpaceFree()<(efb_direct_view?0:512*512*4)+512*256*4+LINEAR_RESERVE)goto fail;
    if(efb_direct_view)efb_color.capture=efb_target_texture;
    else {
    efb_color.capture_owned=1;
#ifdef FH_VRAM_CAPTURE_NEGATIVE
#ifndef FH_EMULATOR_TEST
#error Physical VRAM capture regression is a test-only mutation
#endif
    if(!C3D_TexInitVRAM(&efb_color.capture,512,512,GPU_RGBA8))goto fail;
#else
    if(!C3D_TexInit(&efb_color.capture,512,512,GPU_RGBA8))goto fail;
#endif
    }
    if(!C3D_TexInitVRAM(&efb_color.texture.gpu,512,256,GPU_RGB565) ||
       !C3D_TexInitVRAM(&efb_color.scratch,512,256,GPU_RGBA8))goto fail;
    efb_color.output=C3D_RenderTargetCreateFromTex(&efb_color.texture.gpu,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    efb_color.scratch_target=C3D_RenderTargetCreateFromTex(&efb_color.scratch,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    if(linearSpaceFree()<512*256*4+LINEAR_RESERVE)goto fail;
    efb_color.linear=linearMemAlign(512*256*4,4096);
    if(!efb_color.output || !efb_color.scratch_target || !efb_color.linear)goto fail;
    /* Initialize/flush padding once, before any GPU copy can own these bytes.
     * Subsequent GPU-to-GPU use must not flush stale ARM cache lines over it. */
    if(efb_color.capture_owned){memset(efb_color.capture.data,0,efb_color.capture.size);C3D_TexFlush(&efb_color.capture);}
    sampled_capture_storage=efb_color.capture_owned?efb_color.capture.data:NULL;
    sampled_capture_bytes=efb_color.capture_owned?efb_color.capture.size:0;
    C3D_TexSetFilter(&efb_color.capture,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&efb_color.capture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    C3D_TexSetFilter(&efb_color.texture.gpu,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&efb_color.texture.gpu,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    efb_color.texture.bytes=512*256*2;
    fh_platform_log(efb_direct_view?"GPU color reflection enabled; borrowed VRAM view; extra VRAM=786432 linear=524288\n":
        "GPU color reflection enabled; linear sampled capture; VRAM=786432 linear=1572864\n");return;
fail:
    efb_color_finish();fh_platform_log("GPU color reflection budget unavailable; retaining CPU copy\n");
}
static void efb_color_begin(C3D_RenderTarget *out,C3D_Tex *source){
    flush_vertices();if(vertex_count+24>VERTICES)fh_renderer_sync();
    configure_transform(NULL);configure_affine(NULL);
    C3D_FrameDrawOn(out);C3D_SetViewport(0,0,512,256);
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_FogGasMode(GPU_NO_FOG,(GPU_GASMODE)0,false);fog_valid=0;
    C3D_TexBind(0,source);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,0,0);C3D_TexEnvFunc(e,C3D_Both,GPU_REPLACE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);C3D_TexEnvColor(e,0xffffffff);
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
}
static void efb_view_boundary(const char *phase){
    /* Both the EFB and snapshot are persistent GPU-owned allocations. Keep
     * their producer, sampler and subsequent overwrite in the same queue.
     * Split retains framebuffer flush/invalidate; TexBind retains texture
     * cache invalidation. CPU readers and buffer retirement still join.
     * Bound the SDK's capacity drain before it can wait indefinitely. */
    gxCmdQueue_s *q=&C3Di_GetContext()->gxQueue;
    if(q->numEntries+4>=q->maxEntries)gpu_copy_wait("view capacity",++gpu_copy_fence_count);
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    if(efb_ordered_view)efb_view_waits_avoided++;
    else gpu_copy_wait(phase,++gpu_copy_fence_count);
}
static void efb_color_end(unsigned count){
    flush_stream(vertices,(vertex_count+count)*sizeof(*vertices),&flushed_vertex_bytes,vertex_count*sizeof(*vertices));
    C3D_DrawArrays(GPU_TRIANGLES,vertex_count,count);vertex_count+=count;draws++;total_vertices+=count;
    if(efb_view_active){
        efb_view_boundary("view consumer");efb_view_active=0;
    }
    C3D_FrameDrawOn(target);C3D_SetViewport(0,0,FH_EFB_NATIVE_HEIGHT,400);
    combiner_valid=raster_valid=0;memset(bound_used,0,sizeof(bound_used));
}
/* Read a GPU texture through the same ordinary RGBA8 render/DisplayTransfer
 * path used for color materialization. Also used by the depth-view admission
 * check; callers own both the scratch target and the returned linear bytes. */
static void efb_color_read_linear(C3D_Tex *texture){
    begin();reserve_commands();efb_color_begin(efb_color.scratch_target,texture);
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        GpuVertex *v=vertices+vertex_count+i;unsigned k=corners[i];memset(v,0,sizeof(*v));
        v->position[0]=2*xy[k][0]-1;v->position[1]=1-2*xy[k][1];v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;
        v->uv[0][0]=xy[k][0];v->uv[0][1]=1-xy[k][1];v->uv[0][2]=1;
    }
    efb_color_end(6);C3D_FrameSplit(GX_CMDLIST_FLUSH);
    const char *error=fh_gpu_readback_error((uintptr_t)efb_color.scratch.data,efb_color.scratch.size,(uintptr_t)efb_color.linear,512*256*4,512,256,0);
    if(error)fh_platform_panic(error);
    if(R_FAILED(GX_DisplayTransfer(efb_color.scratch.data,GX_BUFFER_DIM(512,256),efb_color.linear,GX_BUFFER_DIM(512,256),0)))fh_platform_panic("GPU copy materialization transfer");
    fh_renderer_sync();if(R_FAILED(GSPGPU_InvalidateDataCache(efb_color.linear,512*256*4)))fh_platform_panic("GPU copy materialization visibility");
}
void fh_renderer_materialize_texture(const void *pixels){
    efb_shadow_materialize(pixels);
    efb_depth_materialize(pixels);
    if(!efb_color.dirty || pixels!=efb_color.destination)return;
    efb_color_read_linear(&efb_color.texture.gpu);
    unsigned char *dst=efb_color.destination;
    for(unsigned y=0;y<240;y++)for(unsigned x=0;x<320;x++){
        u32 color=efb_color.linear[y*512+x];unsigned value=((color>>16)&0xf800)|((color>>13)&0x7e0)|((color>>11)&31);
        unsigned at=((y/4)*80+x/4)*32+2*((y&3)*4+(x&3));dst[at]=value>>8;dst[at+1]=value;
    }
    efb_color.dirty=0;efb_color.materializations++;
    /* Melee distinguishes CPU-written ranges from GPU copy ownership.
     * Only these destination bytes changed; unrelated textures stay valid. */
    texture_cache_invalidate(efb_color.destination,320*240*2);
}
static Texture *efb_color_lookup(const FhTexture *source,unsigned mode){
    if(!efb_color.destination || source->pixels!=efb_color.destination)return NULL;
    if(mode || source->width!=320 || source->height!=240 || source->format!=4){fh_renderer_materialize_texture(source->pixels);return NULL;}
    efb_color.texture.frame=frame_number;return &efb_color.texture;
}
static void efb_color_invalidate(const void *address,size_t bytes){
    if(!address || !efb_color.destination)return;
    uintptr_t p=(uintptr_t)address,q=(uintptr_t)efb_color.destination;
    if(p<q+320*240*2 && q<p+bytes){efb_color.destination=NULL;efb_color.dirty=0;}
}
static void efb_capture_rows(u32 *source,unsigned rows){
    if(!rows || rows>50)fh_platform_panic("EFB capture row bounds");
    if(efb_direct_view){
        if(efb_view_active || (source!=target->frameBuf.colorBuf && source!=depth_snapshot_storage))fh_platform_panic("Unowned EFB texture view");
        efb_view_boundary("view producer");
        efb_color.capture.data=source;efb_view_active=1;efb_sampled_views++;
    }else if(efb_storage_width==512){
        copy_owned_gpu(source,efb_storage_bytes,0,(u32*)efb_color.capture.data,
            efb_color.capture.size,0,rows*512*8*4,FH_GPU_COPY_CONTIGUOUS);
    }else{
        copy_owned_gpu(source,efb_storage_bytes,GX_BUFFER_DIM(264*8*4/16,0),(u32*)efb_color.capture.data,
            efb_color.capture.size,GX_BUFFER_DIM(264*8*4/16,(512-264)*8*4/16),rows*264*8*4,FH_GPU_COPY_GAPPED);
    }
}
int fh_renderer_copy_gpu_color(void *destination,const uint32_t source[4],unsigned w,unsigned h,unsigned format,int half,int clear){
    if(fh_render_floor)return 0;
    if(!efb_color.enabled || !destination || format!=4 || w!=320 || h!=240 || !half || clear ||
       source[0] || source[1] || source[2]!=640 || source[3]!=480)return 0;
    begin();reserve_commands();u64 started=svcGetSystemTick();
    if(efb_color.destination && destination!=efb_color.destination)fh_renderer_materialize_texture(efb_color.destination);
    flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
    /* GPU owns this linear texture; CPU materialization uses DisplayTransfer
     * from the separate output texture, never reads these tiled bytes. */
    efb_capture_rows(target->frameBuf.colorBuf,50);
    efb_color_begin(efb_color.output,&efb_color.capture);
    float regions[4][4]={{0,0,320,240},{320,0,512,240},{0,240,320,256},{320,240,512,256}};unsigned count=0,corners[]={0,1,2,0,2,3};
    for(unsigned r=0;r<4;r++){
        float *b=regions[r],xy[4][2]={{b[0],b[1]},{b[0],b[3]},{b[2],b[3]},{b[2],b[1]}};
        for(unsigned i=0;i<6;i++){
            GpuVertex *v=vertices+vertex_count+count++;memset(v,0,sizeof(*v));float x=xy[corners[i]][0],y=xy[corners[i]][1];
            v->position[0]=2*x/512-1;v->position[1]=1-2*y/256;v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;
            if(b[0]>=320)x=319.5f;
            if(b[1]>=240)y=239.5f;
            v->uv[0][0]=fh_screen_copy_v(y)/512;v->uv[0][1]=1-fh_screen_copy_u(x)/512;v->uv[0][2]=1;
        }
    }
    efb_color_end(count);fh_renderer_invalidate(destination,320*240*2);
    efb_color.destination=destination;efb_color.dirty=1;efb_color.copies++;
    efb_color.texture.source=(FhTexture){.pixels=destination,.width=320,.height=240,.format=4};
    copy_ticks+=svcGetSystemTick()-started;copy_pixels+=320*240;return 1;
}
