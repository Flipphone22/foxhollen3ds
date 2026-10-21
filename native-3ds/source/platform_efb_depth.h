/* Audited reflection-depth destination, not a general GXCopyTex replacement.
 * Keep opaque D24S8 bytes in VRAM and preserve the exact copy-time image.
 * Melee omits its depth-texture payloads for coverage overlays. SFA consumes
 * these values, so this adaptation delays conversion rather than omitting it.
 * An optional GPU Z8 view extracts the inverted high depth byte from a
 * GPU-owned padded copy. CPU consumers retain the validated DisplayTransfer. */
#define EFB_DEPTH_BYTES efb_storage_bytes
static struct {
    u32 *snapshot;
    void *destination;
    unsigned dirty,copies,materializations;
    Texture texture;
    C3D_RenderTarget *output;
    unsigned gpu_valid,gpu_conversions;
    unsigned verify,verification,probe_count,probe_ready;
} efb_depth;
#ifdef FH_EMULATOR_TEST
unsigned fh_renderer_depth_gpu_admission(void){return efb_depth.verification;}
#endif
static void efb_depth_init(void){
    efb_depth.probe_ready=1;
    unsigned enabled=1;
#ifdef FH_EMULATOR_TEST
    enabled=0;
#endif
    FILE *f=fopen("sdmc:/3ds/foxhollow/efb_depth.txt","rb");
    if(f){enabled=fgetc(f)=='1';fclose(f);}if(!enabled)return;
    efb_depth.snapshot=vramMemAlign(EFB_DEPTH_BYTES,4096);
    depth_snapshot_storage=efb_depth.snapshot;
    depth_snapshot_bytes=efb_depth.snapshot?EFB_DEPTH_BYTES:0;
    if(efb_depth.snapshot){char line[96];snprintf(line,sizeof(line),"Deferred reflection depth enabled; VRAM=%u\n",EFB_DEPTH_BYTES);fh_platform_log(line);}
    else fh_platform_log("Deferred depth allocation unavailable; retaining CPU copy\n");
    unsigned gpu=0;
#ifndef FH_EMULATOR_TEST
    /* Device admission is mandatory in the console candidate. Unsupported
     * aliasing/quantization falls back to the established CPU conversion. */
    gpu=efb_depth.verify=1;
#endif
    f=fopen("sdmc:/3ds/foxhollow/efb_depth_gpu.txt","rb");
    if(f){int mode=fgetc(f);gpu=mode=='1' || mode=='2';efb_depth.verify=mode=='2';fclose(f);}
    if(gpu && efb_depth.snapshot && efb_color.capture.data && vramSpaceFree()>=512*256*4+1024*1024 &&
       C3D_TexInitVRAM(&efb_depth.texture.gpu,512,256,GPU_RGBA8)){
        efb_depth.output=C3D_RenderTargetCreateFromTex(&efb_depth.texture.gpu,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
        if(!efb_depth.output){C3D_TexDelete(&efb_depth.texture.gpu);memset(&efb_depth.texture,0,sizeof(efb_depth.texture));}
        else {efb_depth.texture.bytes=512*256*4;fh_platform_log("Experimental GPU Z8 view enabled; additional VRAM=524288\n");}
    }
}
static void efb_depth_finish(void){
    if(efb_depth.snapshot){
        char line[200];snprintf(line,sizeof(line),"Deferred depth copies=%u CPU_materializations=%u GPU_conversions=%u validation=%u probes=%u\n",efb_depth.copies,efb_depth.materializations,efb_depth.gpu_conversions,efb_depth.verification,efb_depth.probe_count);fh_platform_log(line);
        vramFree(efb_depth.snapshot);
        depth_snapshot_storage=NULL;depth_snapshot_bytes=0;
    }
    if(efb_depth.output)C3D_RenderTargetDelete(efb_depth.output);
    if(efb_depth.texture.gpu.data)C3D_TexDelete(&efb_depth.texture.gpu);
    memset(&efb_depth,0,sizeof(efb_depth));
}
static void efb_depth_invalidate(const void *address,size_t bytes){
    if(!address || !efb_depth.destination)return;
    uintptr_t p=(uintptr_t)address,q=(uintptr_t)efb_depth.destination;
    if(p<q+320*240 && q<p+bytes){efb_depth.destination=NULL;efb_depth.dirty=efb_depth.gpu_valid=0;}
}
static Texture *efb_depth_lookup(const FhTexture *source,unsigned mode){
    if(!efb_depth.output || !efb_depth.destination || source->pixels!=efb_depth.destination ||
       mode || source->format!=1 || source->width!=320 || source->height!=240 || efb_depth.verification==2)return NULL;
    /* A flat intro proves neither support nor failure. After an inconclusive
     * probe, let the required CPU fallback identify a varied scene before
     * paying for another full GPU comparison. Never turn flatness into a
     * permanent hardware rejection or accept it as validation. */
    if(efb_depth.verify && !efb_depth.verification && !efb_depth.probe_ready)return NULL;
    if(!efb_depth.gpu_valid){
        /* D24S8 and RGBA8 both occupy four bytes in the same tiled layout.
         * The high depth byte is RGBA8 green. 255-G equals the high byte of
         * (0xffffff-depth), including low-byte carries. Never read raw VRAM
         * from ARM. Reuse the color-copy scratch only after prior draws drain. */
        begin();reserve_commands();u64 started=svcGetSystemTick();
        flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
        efb_capture_rows(efb_depth.snapshot,50);
        efb_color_begin(efb_depth.output,&efb_color.capture);
        C3D_TexEnvOpRgb(C3D_GetTexEnv(0),GPU_TEVOP_RGB_ONE_MINUS_SRC_G,0,0);
        float regions[4][4]={{0,0,320,240},{320,0,512,240},{0,240,320,256},{320,240,512,256}};
        unsigned count=0,corners[]={0,1,2,0,2,3};
        for(unsigned r=0;r<4;r++){
            float *b=regions[r],xy[4][2]={{b[0],b[1]},{b[0],b[3]},{b[2],b[3]},{b[2],b[1]}};
            for(unsigned i=0;i<6;i++){
                GpuVertex *v=vertices+vertex_count+count++;memset(v,0,sizeof(*v));
                float x=xy[corners[i]][0],y=xy[corners[i]][1];
                v->position[0]=2*x/512-1;v->position[1]=1-2*y/256;v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;
                if(b[0]>=320)x=319.5f;
                if(b[1]>=240)y=239.5f;
                v->uv[0][0]=fh_screen_copy_v(y)/512;v->uv[0][1]=1-fh_screen_copy_u(x)/512;v->uv[0][2]=1;
            }
        }
        efb_color_end(count);efb_depth.gpu_valid=1;efb_depth.gpu_conversions++;
        copy_ticks+=svcGetSystemTick()-started;
    }
    if(efb_depth.verify && !efb_depth.verification){
        /* Admission compares actual sampled RGB/alpha, including replicated
         * borders, against the established opaque-depth DisplayTransfer. It
         * neither writes the game's destination nor changes its copy epoch.
         * Existing readback and color scratch buffers suffice: no allocation.
         * A blank frame is insufficient evidence; retry only after a varied
         * CPU fallback image. A real mismatch disables this path for the run. */
        flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
        queue_readback(efb_depth.snapshot,FH_EFB_NATIVE_HEIGHT,400);fh_renderer_sync();
        acquire_readback();
        efb_color_read_linear(&efb_depth.texture.gpu);
        unsigned seen[8]={0},distinct=0,mismatch=0,x_variation=0,y_variation=0;
        for(unsigned y=0;y<256;y++)for(unsigned x=0;x<512;x++){
            unsigned sx=x<320?x:319,sy=y<240?y:239;
            unsigned expected=fh_efb_depth_pixel_stride(readback,readback_stride,2*sx,2*sy)>>16;
            unsigned value=efb_color.linear[y*512+x];
            mismatch+=value!=(expected*0x01010100u|255u);
            unsigned mask=1u<<(expected&31);
            if(!(seen[expected>>5]&mask)){seen[expected>>5]|=mask;distinct++;}
            if(x && x<320)x_variation|=expected!=(fh_efb_depth_pixel_stride(readback,readback_stride,2*(x-1),2*sy)>>16);
            if(y && y<240)y_variation|=expected!=(fh_efb_depth_pixel_stride(readback,readback_stride,2*sx,2*(y-1))>>16);
        }
        efb_depth.probe_count++;
        if(mismatch)efb_depth.verification=2;
        else if(distinct>=4 && x_variation && y_variation)efb_depth.verification=1;
        else efb_depth.probe_ready=0;
        char line[192];snprintf(line,sizeof(line),"GPU Z8 admission=%u probe=%u mismatches=%u levels=%u varied=%u/%u (1=verified,2=CPU)\n",efb_depth.verification,efb_depth.probe_count,mismatch,distinct,x_variation,y_variation);fh_platform_log(line);
        if(efb_depth.verification!=1)return NULL;
    }
    efb_depth.texture.frame=frame_number;return &efb_depth.texture;
}
static void efb_depth_materialize(const void *pixels){
    if(!efb_depth.dirty || pixels!=efb_depth.destination)return;
    begin();reserve_commands();flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
    u64 started=svcGetSystemTick();
    queue_readback(efb_depth.snapshot,FH_EFB_NATIVE_HEIGHT,400);fh_renderer_sync();
    acquire_readback();
    const uint32_t rect[]={0,0,640,480};
    if(!fh_efb_depth_convert_stride(efb_depth.destination,320*240,readback,readback_stride,rect,320,240,0x11))fh_platform_panic("Deferred depth conversion");
    if(efb_depth.verify && !efb_depth.verification && !efb_depth.probe_ready){
        /* Existing ordinary-heap copy only: no extra GPU transfer or raw VRAM
         * access. Sparse variation is a retry hint, never admission evidence;
         * the next eligible sample still compares every RGBA byte exactly. */
        const unsigned char *z=efb_depth.destination;
        unsigned seen[8]={0},distinct=0,x_variation=0,y_variation=0;
        for(unsigned y=0;y<240;y+=16)for(unsigned x=0;x<320;x+=16){
            unsigned value=z[y*320+x],mask=1u<<(value&31);
            if(!(seen[value>>5]&mask)){seen[value>>5]|=mask;distinct++;}
            if(x)x_variation|=value!=z[y*320+x-16];
            if(y)y_variation|=value!=z[(y-16)*320+x];
        }
        efb_depth.probe_ready=distinct>=4 && x_variation && y_variation;
    }
    efb_depth.dirty=0;efb_depth.materializations++;
    texture_cache_invalidate(efb_depth.destination,320*240);
    copy_ticks+=svcGetSystemTick()-started;
}
int fh_renderer_copy_gpu_depth(void *destination,const uint32_t source[4],unsigned w,unsigned h,unsigned format,int half,int clear){
    if(fh_render_floor)return 0;
    if(!efb_depth.snapshot || !destination || format!=0x11 || w!=320 || h!=240 || !half || clear ||
       source[0] || source[1] || source[2]!=640 || source[3]!=480)return 0;
    begin();reserve_commands();u64 started=svcGetSystemTick();
    if(efb_depth.destination && destination!=efb_depth.destination)efb_depth_materialize(efb_depth.destination);
    flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
    /* Both ends are VRAM. CPU reads always use queue_readback's validated
     * flags-zero DisplayTransfer, with its aligned linear destination. */
    copy_owned_gpu(target->frameBuf.depthBuf,EFB_DEPTH_BYTES,0,efb_depth.snapshot,EFB_DEPTH_BYTES,0,EFB_DEPTH_BYTES,FH_GPU_COPY_CONTIGUOUS);
    fh_renderer_invalidate(destination,320*240);
    efb_depth.destination=destination;efb_depth.dirty=1;efb_depth.copies++;
    efb_depth.gpu_valid=0;efb_depth.texture.source=(FhTexture){.pixels=destination,.width=320,.height=240,.format=1};
    copy_ticks+=svcGetSystemTick()-started;copy_pixels+=320*240;return 1;
}
