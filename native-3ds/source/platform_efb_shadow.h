/* Audited newshadows.c R4 outputs. Adapted from pinned Melee renderer.c
 * gpu_efb_copy/texture_dirty; see licenses/ADAPTATIONS.md. CPU readers retain
 * exact GX bytes through materialization, rather than synthetic EFB tags. */
enum { SHADOW_SLOTS=8,SHADOW_VRAM_BUDGET=512*1024,SHADOW_VRAM_RESERVE=1024*1024 };
typedef struct {Texture texture;C3D_RenderTarget *target;void *destination;unsigned dirty,width,height,stamp;} ShadowCopy;
static ShadowCopy shadow_copies[SHADOW_SLOTS];
static unsigned shadow_enabled,shadow_clock;
unsigned fh_shadow_bytes;
unsigned fh_shadow_copies,fh_shadow_materializations,fh_shadow_evictions,fh_shadow_fallbacks;
unsigned fh_shadow_quantization; /* 1=direct, 2=rounding compensation, 3=disabled */
static void shadow_quantization_state(unsigned compensated) {
    C3D_TexEnv *e=C3D_GetTexEnv(0);
    C3D_TexEnvSrc(e,C3D_Both,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_R,0,0);C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,0,0);
    C3D_TexEnvFunc(e,C3D_Both,compensated?GPU_SUBTRACT:GPU_REPLACE);C3D_TexEnvColor(e,0x08080808);
    if(compensated) {
        e=C3D_GetTexEnv(1);C3D_TexEnvSrc(e,C3D_Both,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
        C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_R,0,0);C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,0,0);
        C3D_TexEnvFunc(e,C3D_Both,GPU_MULTIPLY_ADD);C3D_TexEnvColor(e,0x10101010);
    }
}
static void shadow_calibration_quad(unsigned viewport_width,unsigned viewport_height) {
    unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++) {
        GpuVertex *v=vertices+vertex_count+i;unsigned k=corners[i];memset(v,0,sizeof(*v));
        float x=xy[k][0],y=xy[k][1];v->position[0]=2*x*256/viewport_width-1;v->position[1]=1-2*y*8/viewport_height;
        v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;v->uv[0][0]=x;v->uv[0][1]=1-y;v->uv[0][2]=1;
    }
    efb_color_end(6);
}
static int shadow_calibrate(void) {
    if(fh_shadow_quantization)return fh_shadow_quantization!=3;
    C3D_Tex input={0},output={0};unsigned selected=3;
    reserve_texture_linear(8192);
    if(vramSpaceFree()<4096+SHADOW_VRAM_RESERVE || !C3D_TexInit(&input,256,8,GPU_RGBA8) || !C3D_TexInitVRAM(&output,256,8,GPU_RGBA4))goto done;
    for(unsigned y=0;y<8;y++)for(unsigned x=0;x<256;x++) {
        unsigned morton=(x&1)|((y&1)<<1)|((x&2)<<1)|((y&2)<<2)|((x&4)<<2)|((y&4)<<3);
        ((u32*)input.data)[(x/8)*64+morton]=x*0x01010100u|255;
    }
    C3D_TexFlush(&input);C3D_TexSetFilter(&input,GPU_NEAREST,GPU_NEAREST);C3D_TexSetFilter(&output,GPU_NEAREST,GPU_NEAREST);
    /* Verify every byte intensity on the actual rendering backend. Desktop
     * normalized framebuffer rounding must not be assumed for real PICA. */
    for(unsigned compensated=0;compensated<2;compensated++) {
        fh_renderer_sync();C3D_FrameBufTex(&efb_color.scratch_target->frameBuf,&output,GPU_TEXFACE_2D,0);
        efb_color_begin(efb_color.scratch_target,&input);C3D_SetViewport(0,0,256,8);shadow_quantization_state(compensated);
        shadow_calibration_quad(256,8);fh_renderer_sync();
        C3D_FrameBufTex(&efb_color.scratch_target->frameBuf,&efb_color.scratch,GPU_TEXFACE_2D,0);
        efb_color_begin(efb_color.scratch_target,&output);shadow_calibration_quad(512,256);C3D_FrameSplit(GX_CMDLIST_FLUSH);
        const char *error=fh_gpu_readback_error((uintptr_t)efb_color.scratch.data,efb_color.scratch.size,(uintptr_t)efb_color.linear,512*256*4,512,256,0);
        if(error)fh_platform_panic(error);
        if(R_FAILED(GX_DisplayTransfer(efb_color.scratch.data,GX_BUFFER_DIM(512,256),efb_color.linear,GX_BUFFER_DIM(512,256),0)))fh_platform_panic("Shadow calibration transfer");
        fh_renderer_sync();if(R_FAILED(GSPGPU_InvalidateDataCache(efb_color.linear,512*256*4)))fh_platform_panic("Shadow calibration visibility");
        unsigned mismatch=0;
        for(unsigned y=0;y<8;y++)for(unsigned x=0;x<256;x++)mismatch+=(efb_color.linear[y*512+x]>>24)!=(x>>4)*17;
#ifdef FH_EMULATOR_TEST
        if(mismatch){char path[96];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/shadow-calibration-%u.bin",compensated);
            FILE*f=fopen(path,"wb");if(f){for(unsigned y=0;y<8;y++)fwrite(efb_color.linear+y*512,4,256,f);fclose(f);}}
#endif
        if(!mismatch){selected=compensated+1;break;}
    }
done:
    if(input.data)C3D_TexDelete(&input);
    if(output.data)C3D_TexDelete(&output);
    fh_shadow_quantization=selected;
    char line[128];snprintf(line,sizeof(line),"GPU R4 quantization mode=%u (256 exact intensities; 3 retains CPU copy)\n",selected);fh_platform_log(line);
    return selected!=3;
}
static void efb_shadow_init(void) {
#ifndef FH_EMULATOR_TEST
    /* Exact device fixture + matched Palace integration. Actual backend
     * quantization is checked before admission; failure keeps CPU copies. */
    shadow_enabled=1;
#endif
    FILE *f=fopen("sdmc:/3ds/foxhollow/efb_shadow.txt","rb");
    if(f){shadow_enabled=fgetc(f)=='1';fclose(f);}
    if(shadow_enabled && !efb_color.capture.data)shadow_enabled=0;
    if(shadow_enabled)fh_platform_log("Experimental GPU R4 shadows enabled; VRAM cap=524288 reserve=1048576\n");
}
static void shadow_materialize(ShadowCopy *s) {
    if(!s->dirty || !s->destination)return;
    begin();reserve_commands();efb_color_begin(efb_color.scratch_target,&s->texture.gpu);
    /* Only the red nibble is stored by GX R4. Expand into the existing
     * RGBA8 transfer target so the established safe readback stays intact. */
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++) {
        GpuVertex *v=vertices+vertex_count+i;unsigned k=corners[i];memset(v,0,sizeof(*v));
        float x=xy[k][0],y=xy[k][1];v->position[0]=2*x*s->width/512-1;v->position[1]=1-2*y*s->height/256;
        v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;
        v->uv[0][0]=x;v->uv[0][1]=1-y;v->uv[0][2]=1;
    }
    efb_color_end(6);C3D_FrameSplit(GX_CMDLIST_FLUSH);
    const char *error=fh_gpu_readback_error((uintptr_t)efb_color.scratch.data,efb_color.scratch.size,(uintptr_t)efb_color.linear,512*256*4,512,256,0);
    if(error)fh_platform_panic(error);
    if(R_FAILED(GX_DisplayTransfer(efb_color.scratch.data,GX_BUFFER_DIM(512,256),efb_color.linear,GX_BUFFER_DIM(512,256),0)))fh_platform_panic("Shadow materialization transfer");
    fh_renderer_sync();if(R_FAILED(GSPGPU_InvalidateDataCache(efb_color.linear,512*256*4)))fh_platform_panic("Shadow materialization visibility");
    unsigned char *dst=s->destination;
    for(unsigned y=0;y<s->height;y++)for(unsigned x=0;x<s->width;x+=2) {
        unsigned a=efb_color.linear[y*512+x]>>28,b=efb_color.linear[y*512+x+1]>>28;
        unsigned at=((y/8)*(s->width/8)+x/8)*32+(y&7)*4+(x&7)/2;
        dst[at]=(a<<4)|b;
    }
    s->dirty=0;fh_shadow_materializations++;
    texture_cache_invalidate(s->destination,s->width*s->height/2);
}
static void efb_shadow_materialize(const void *pixels) {
    for(unsigned i=0;i<SHADOW_SLOTS;i++)if(shadow_copies[i].destination==pixels)shadow_materialize(shadow_copies+i);
}
static void shadow_retire(ShadowCopy *s) {
    shadow_materialize(s);fh_renderer_sync();
    /* Citro3D forbids deleting a target inside a frame. Melee keeps the
     * target object and reattaches its next texture after queue completion. */
    C3D_RenderTarget *target=s->target;
    if(s->texture.gpu.data){C3D_TexDelete(&s->texture.gpu);fh_shadow_bytes-=s->texture.bytes;}
    memset(s,0,sizeof(*s));s->target=target;
}
static void efb_shadow_finish(void) {
    for(unsigned i=0;i<SHADOW_SLOTS;i++)if(shadow_copies[i].target) {
        /* Shutdown has no remaining CPU readers of the private images. */
        ShadowCopy *s=shadow_copies+i;
        C3D_RenderTargetDelete(s->target);if(s->texture.gpu.data)C3D_TexDelete(&s->texture.gpu);memset(s,0,sizeof(*s));
    }
    if(shadow_enabled){char line[160];snprintf(line,sizeof(line),"GPU shadows copies=%u materializations=%u evictions=%u fallbacks=%u\n",fh_shadow_copies,fh_shadow_materializations,fh_shadow_evictions,fh_shadow_fallbacks);fh_platform_log(line);}
    fh_shadow_bytes=0;
}
static Texture *efb_shadow_lookup(const FhTexture *source,unsigned mode) {
    for(unsigned i=0;i<SHADOW_SLOTS;i++) {
        ShadowCopy *s=shadow_copies+i;if(!s->destination || s->destination!=source->pixels)continue;
        if(mode || source->format!=0 || source->width!=s->width || source->height!=s->height){shadow_materialize(s);return NULL;}
        s->stamp=++shadow_clock;s->texture.frame=frame_number;return &s->texture;
    }
    return NULL;
}
static void efb_shadow_invalidate(const void *address,size_t bytes) {
    if(!address || !bytes)return;
    uintptr_t p=(uintptr_t)address;
    for(unsigned i=0;i<SHADOW_SLOTS;i++) {
        ShadowCopy *s=shadow_copies+i;if(!s->destination)continue;
        uintptr_t q=(uintptr_t)s->destination;size_t n=s->width*s->height/2;
        if(p>=q?p-q<n:q-p<bytes){s->destination=NULL;s->dirty=0;}
    }
}
int fh_renderer_copy_gpu_shadow(void *destination,const uint32_t source[4],unsigned w,unsigned h,unsigned format,int half,int clear,uint32_t color,uint32_t depth,const FhDrawState *state) {
    if(fh_render_floor)return 0;
    if(!shadow_enabled || !destination || format!=0x20 || !half || w<8 || h<8 || w>256 || h>256 ||
        (w&(w-1)) || (h&(h-1)) || source[0] || source[1] || source[2]!=2*w || source[3]!=2*h)return 0;
    begin();reserve_commands();u64 started=svcGetSystemTick();
    if(!shadow_calibrate()){fh_shadow_fallbacks++;return 0;}
    ShadowCopy *slot=NULL;
    for(unsigned i=0;i<SHADOW_SLOTS;i++)if(shadow_copies[i].destination==destination){slot=shadow_copies+i;break;}
    if(!slot)for(unsigned i=0;i<SHADOW_SLOTS;i++)if(!shadow_copies[i].texture.bytes){slot=shadow_copies+i;break;}
    if(!slot)for(unsigned i=0;i<SHADOW_SLOTS;i++)if(!shadow_copies[i].texture.pinned && (!slot || shadow_copies[i].stamp<slot->stamp))slot=shadow_copies+i;
    if(!slot){fh_shadow_fallbacks++;return 0;}
    if(slot->texture.bytes && (slot->destination!=destination || slot->width!=w || slot->height!=h)){shadow_retire(slot);fh_shadow_evictions++;}
    if(!slot->texture.bytes) {
        unsigned required=w*h*2;
        while(fh_shadow_bytes+required>SHADOW_VRAM_BUDGET || vramSpaceFree()<required+SHADOW_VRAM_RESERVE) {
            ShadowCopy *oldest=NULL;
            for(unsigned i=0;i<SHADOW_SLOTS;i++) {
                ShadowCopy *s=shadow_copies+i;
                if(s!=slot && s->texture.bytes && !s->texture.pinned && (!oldest || s->stamp<oldest->stamp))oldest=s;
            }
            if(!oldest){fh_shadow_fallbacks++;return 0;}
            shadow_retire(oldest);fh_shadow_evictions++;
        }
        if(!C3D_TexInitVRAM(&slot->texture.gpu,w,h,GPU_RGBA4)){fh_shadow_fallbacks++;return 0;}
        if(slot->target)C3D_FrameBufTex(&slot->target->frameBuf,&slot->texture.gpu,GPU_TEXFACE_2D,0);
        else slot->target=C3D_RenderTargetCreateFromTex(&slot->texture.gpu,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
        if(!slot->target){C3D_TexDelete(&slot->texture.gpu);memset(slot,0,sizeof(*slot));fh_shadow_fallbacks++;return 0;}
        slot->texture.bytes=required;fh_shadow_bytes+=required;slot->width=w;slot->height=h;
        C3D_TexSetFilter(&slot->texture.gpu,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&slot->texture.gpu,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    }
    flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
    /* Melee copies only affected tile rows into its GPU-sampled linear capture. */
    unsigned last=(fh_screen_edge(2*w)+7)/8;
    efb_capture_rows(target->frameBuf.colorBuf,last);
    efb_color_begin(slot->target,&efb_color.capture);C3D_SetViewport(0,0,w,h);
    shadow_quantization_state(fh_shadow_quantization==2);
    unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++) {
        GpuVertex *v=vertices+vertex_count+i;unsigned k=corners[i];memset(v,0,sizeof(*v));float x=xy[k][0]*w,y=xy[k][1]*h;
        v->position[0]=2*x/w-1;v->position[1]=1-2*y/h;v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;
        v->uv[0][0]=(264-(y-.5f)-.0001f)/512;v->uv[0][1]=1-fh_screen_copy_u(x)/512;v->uv[0][2]=1;
    }
    efb_color_end(6);fh_renderer_invalidate(destination,w*h/2);
    slot->destination=destination;slot->dirty=1;slot->stamp=++shadow_clock;
    slot->texture.source=(FhTexture){.pixels=destination,.width=w,.height=h,.format=0};
    if(clear)efb_clear_rect(source,color,depth,state);
    fh_shadow_copies++;copy_ticks+=svcGetSystemTick()-started;copy_pixels+=w*h;return 1;
}
