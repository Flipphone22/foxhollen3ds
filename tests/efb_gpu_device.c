/* Prototype of Melee's gpu_efb_copy, adapted to the 264x400 framebuffer.
 * Kept outside the production renderer until exact-copy/lifetime tests pass.
 * Melee provenance/license: native-3ds/licenses/ADAPTATIONS.md. */
#include "../native-3ds/source/platform_graphics.c"
#include <sys/stat.h>
u32 __ctru_heap_size=8*1024*1024,__ctru_linear_heap_size=12*1024*1024,__stacksize__=512*1024;
void fh_audio_pump(void) {}
void fh_platform_log(const char *s) {(void)s;}
void fh_platform_panic(const char *s) {FILE*f=fopen("sdmc:/3ds/foxhollow/panic.txt","wb");if(f){fputs(s,f);fclose(f);}exit(1);}
Result __real_GX_TextureCopy(u32*,u32,u32*,u32,u32,u32);
Result __wrap_GX_TextureCopy(u32 *src,u32 in,u32 *dst,u32 out,u32 bytes,u32 flags){
    uintptr_t p=(uintptr_t)dst;
    if(p<0x1f000000 || p>=0x1f600000)fh_platform_panic("Regression: raw GPU readback is outside the supported transfer contract");
    return __real_GX_TextureCopy(src,in,dst,out,bytes,flags);
}
static void gpu_copy(C3D_Tex *capture,C3D_RenderTarget *out,const u32 rect[4],unsigned w,unsigned h,int depth){
    flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
    /* Only VRAM -> VRAM. The old raw depth -> linear/CPU path stays forbidden. */
    C3D_SyncTextureCopy(depth?target->frameBuf.depthBuf:target->frameBuf.colorBuf,
        GX_BUFFER_DIM(264*8*4/16,0),(u32*)capture->data,
        GX_BUFFER_DIM(264*8*4/16,(512-264)*8*4/16),264*400*4,8);
    configure_transform(NULL);configure_affine(NULL);
    C3D_FrameDrawOn(out);C3D_SetViewport(0,0,512,256);
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_FogGasMode(GPU_NO_FOG,(GPU_GASMODE)0,false);fog_valid=0;
    C3D_TexBind(0,capture);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnv *e=C3D_GetTexEnv(0);C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,0,0);C3D_TexEnvFunc(e,C3D_Both,GPU_REPLACE);
    if(depth)C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_ONE_MINUS_SRC_G,0,0);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);C3D_TexEnvColor(e,0xffffffff);
#ifdef FH_EFB_R4_PROTOTYPE
    /* PICA RGBA4 storage rounds; GX R4 truncates. Map byte r to
     * max(r-8,0)+floor(r*16/255), whose rounded nibble is r>>4. */
    C3D_TexEnvSrc(e,C3D_Both,GPU_TEXTURE0,GPU_CONSTANT,0);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_R,0,0);
    C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,0,0);
    C3D_TexEnvFunc(e,C3D_Both,GPU_SUBTRACT);C3D_TexEnvColor(e,0x08080808);
    e=C3D_GetTexEnv(1);C3D_TexEnvSrc(e,C3D_Both,GPU_TEXTURE0,GPU_CONSTANT,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_R,0,0);C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,0,0);
    C3D_TexEnvFunc(e,C3D_Both,GPU_MULTIPLY_ADD);C3D_TexEnvColor(e,0x10101010);
#endif
    C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    unsigned corner[]={0,1,2,0,2,3};float regions[4][4]={{0,0,w,h},{w,0,512,h},{0,h,w,256},{w,h,512,256}};
    unsigned count=0;
    for(unsigned r=0;r<4;r++){
        const float *b=regions[r];float xy[4][2]={{b[0],b[1]},{b[0],b[3]},{b[2],b[3]},{b[2],b[1]}};
        if(b[0]>=b[2] || b[1]>=b[3])continue;
        for(unsigned i=0;i<6;i++){
            GpuVertex *v=vertices+vertex_count+count++;memset(v,0,sizeof(*v));float x=xy[corner[i]][0],y=xy[corner[i]][1];
            v->position[0]=2*x/512-1;v->position[1]=1-2*y/256;v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;
            for(unsigned c=0;c<4;c++)v->color[c]=1;
            if(b[0]>=w)x=w-.5f;if(b[1]>=h)y=h-.5f;
            v->uv[0][0]=(264-(rect[1]+(y-.5f)*rect[3]/h)*.5f-.0001f)/512;
            v->uv[0][1]=1-(40+(rect[0]+(x-.5f)*rect[2]/w)*.5f+.0001f)/512;
            v->uv[0][2]=1;
        }
    }
    GSPGPU_FlushDataCache(vertices+vertex_count,count*sizeof(*vertices));
    C3D_DrawArrays(GPU_TRIANGLES,vertex_count,count);vertex_count+=count;
    combiner_valid=raster_valid=0;memset(bound_used,0,sizeof(bound_used));
    C3D_FrameDrawOn(target);C3D_SetViewport(0,0,264,400);
}
static void expand_copy(C3D_Tex *source,C3D_RenderTarget *out){
    flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);C3D_FrameDrawOn(out);C3D_SetViewport(0,0,512,256);C3D_TexBind(0,source);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_TEXTURE0,0,0);C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_Both,GPU_REPLACE);
    C3D_TexEnvOpRgb(C3D_GetTexEnv(0),GPU_TEVOP_RGB_SRC_COLOR,0,0);
    unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){GpuVertex*v=vertices+vertex_count+i;memset(v,0,sizeof(*v));unsigned k=corners[i];v->position[0]=2*xy[k][0]-1;v->position[1]=1-2*xy[k][1];v->position[2]=-.5f;v->position[3]=1;v->matrix_index=-1;v->uv[0][0]=xy[k][0];v->uv[0][1]=1-xy[k][1];v->uv[0][2]=1;}
    GSPGPU_FlushDataCache(vertices+vertex_count,6*sizeof(*vertices));C3D_DrawArrays(GPU_TRIANGLES,vertex_count,6);vertex_count+=6;C3D_FrameDrawOn(target);C3D_SetViewport(0,0,264,400);
}
int main(void){
    gfxInitDefault();mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow",0777);fh_renderer_init();begin();
    C3D_Tex capture,depth_capture,result,expanded;
    GPU_TEXCOLOR result_format=GPU_RGB565;
#ifdef FH_EFB_R4_PROTOTYPE
    result_format=GPU_RGBA4;
#endif
    if(!C3D_TexInitVRAM(&capture,512,512,GPU_RGBA8) || !C3D_TexInitVRAM(&depth_capture,512,512,GPU_RGBA8) || !C3D_TexInitVRAM(&result,512,256,result_format) || !C3D_TexInitVRAM(&expanded,512,256,GPU_RGBA8))fh_platform_panic("Prototype VRAM budget");
    C3D_TexSetFilter(&capture,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&capture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    C3D_TexSetFilter(&depth_capture,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&depth_capture,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    C3D_TexSetFilter(&result,GPU_NEAREST,GPU_NEAREST);C3D_TexSetWrap(&result,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    C3D_RenderTarget *output=C3D_RenderTargetCreateFromTex(&result,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    C3D_RenderTarget *expanded_target=C3D_RenderTargetCreateFromTex(&expanded,GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    u32 *linear=linearMemAlign(512*256*4,4096),*reference=malloc(264*400*4);
    if(!output || !linear || !reference)fh_platform_panic("Prototype allocation");
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=528;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=528;
    s.color_write=s.alpha_write=s.depth_write=s.depth_test=1;s.depth_compare=7;s.alpha_compare[0]=s.alpha_compare[3]=7;s.stages=1;
    s.tev[0]=(FhTevStage){.color={15,15,15,10},.alpha={7,7,7,5},.color_clamp=1,.alpha_clamp=1,.channel=4};
    for(unsigned t=0;t<4;t++)for(unsigned c=0;c<4;c++)s.swaps[t][c]=c;
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{-1,-1},{-1,1},{1,1},{1,-1}};
    for(unsigned i=0;i<6;i++){unsigned k=corners[i];v[i].position[0]=xy[k][0];v[i].position[1]=xy[k][1];v[i].position[2]=-.9f+.2f*k;v[i].position[3]=1;v[i].color[0][k%3]=1;v[i].color[0][3]=1;}
    fh_renderer_draw(v,6,&s);flush_vertices();
    FILE *report=fopen("sdmc:/3ds/foxhollow/efb-prototype.jsonl","wb");
    for(unsigned depth=0;depth<1;depth++){
        C3D_FrameSplit(GX_CMDLIST_FLUSH);queue_readback(depth?target->frameBuf.depthBuf:target->frameBuf.colorBuf,264,400);fh_renderer_sync();
        GSPGPU_InvalidateDataCache(readback,264*400*4);memcpy(reference,readback,264*400*4);
        for(unsigned trial=0;trial<3;trial++){
            u32 rects[3][4]={{0,0,640,480},{24,36,160,120},{0,0,80,60}};unsigned widths[]={320,80,80},heights[]={240,60,60};
#ifdef FH_EFB_R4_PROTOTYPE
            rects[1][0]=rects[1][1]=0;rects[1][2]=rects[1][3]=256;widths[1]=heights[1]=128;
            rects[2][0]=24;rects[2][1]=36;rects[2][2]=160;rects[2][3]=120;widths[2]=80;heights[2]=60;
#endif
            u32 *rect=rects[trial];unsigned w=widths[trial],h=heights[trial];gpu_copy(depth?&depth_capture:&capture,output,rect,w,h,depth);
            expand_copy(&result,expanded_target);
            C3D_FrameSplit(GX_CMDLIST_FLUSH);
            const char *error=fh_gpu_readback_error((uintptr_t)expanded.data,expanded.size,(uintptr_t)linear,512*256*4,512,256,0);if(error)fh_platform_panic(error);
            GX_DisplayTransfer(expanded.data,GX_BUFFER_DIM(512,256),linear,GX_BUFFER_DIM(512,256),0);fh_renderer_sync();GSPGPU_InvalidateDataCache(linear,512*256*4);
            if(!trial){char path[128];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/actual-%u.bin",depth);FILE*f=fopen(path,"wb");fwrite(linear,4,512*256,f);fclose(f);snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/reference-%u.bin",depth);f=fopen(path,"wb");fwrite(reference,4,264*400,f);fclose(f);}
            unsigned maximum=0,mismatches=0;unsigned long long total=0;
            for(unsigned y=0;y<256;y++)for(unsigned x=0;x<512;x++){
                unsigned sx=rect[0]+(x<w?x:w-1)*rect[2]/w,sy=rect[1]+(y<h?y:h-1)*rect[3]/h;
                u32 expected=reference[(40+sx/2)*264+263-sy/2],actual=linear[y*512+x];
                if(depth){unsigned d=255-((expected>>16)&255);expected=d<<24|d<<16|d<<8|255;}else expected|=255;
                unsigned r=expected>>27,g=(expected>>18)&63,b=(expected>>11)&31;
                expected=((r<<3)|(r>>2))<<24|((g<<2)|(g>>4))<<16|((b<<3)|(b>>2))<<8|255;
#ifdef FH_EFB_R4_PROTOTYPE
                expected=((reference[(40+sx/2)*264+263-sy/2]>>28)*17)*0x01010101u;
#endif
                for(unsigned c=0;c<4;c++){unsigned d=abs((int)((expected>>(8*c))&255)-(int)((actual>>(8*c))&255));if(d>maximum)maximum=d;total+=d;mismatches+=d!=0;}
            }
            fprintf(report,"{\"depth\":%u,\"trial\":%u,\"maximum\":%u,\"mismatches\":%u,\"absolute_error\":%llu}\n",depth,trial,maximum,mismatches,total);fflush(report);
        }
    }
    fclose(report);fh_renderer_present();C3D_FrameSync();C3D_RenderTargetDelete(output);C3D_RenderTargetDelete(expanded_target);C3D_TexDelete(&capture);C3D_TexDelete(&depth_capture);C3D_TexDelete(&result);C3D_TexDelete(&expanded);linearFree(linear);free(reference);
    /* Retaining opaque tiled depth is separate from the rejected experiment
     * that sampled a D24 copy as a color texture. All CPU visibility still
     * uses the guarded DisplayTransfer contract. */
    enum { DEPTH_BYTES=264*400*4 };
    u32 *saved_depth=vramMemAlign(DEPTH_BYTES,4096);
    unsigned char *expected_depth=malloc(DEPTH_BYTES);
    if(!saved_depth || !expected_depth)fh_platform_panic("Depth retention allocation");
    unsigned depth_checks=0;
    for(unsigned trial=0;trial<3;trial++){
        fh_renderer_clear(255,0xabcdef+trial);begin();
        for(unsigned i=0;i<6;i++)v[i].position[2]=-.85f+.1f*corners[i]+trial*.05f;
        fh_renderer_draw(v,6,&s);flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
        queue_readback(target->frameBuf.depthBuf,264,400);fh_renderer_sync();
        GSPGPU_InvalidateDataCache(readback,DEPTH_BYTES);memcpy(expected_depth,readback,DEPTH_BYTES);
        C3D_FrameSplit(GX_CMDLIST_FLUSH);
        C3D_SyncTextureCopy(target->frameBuf.depthBuf,0,saved_depth,0,DEPTH_BYTES,8);
        fh_renderer_present();fh_renderer_clear(255,0x123456);begin();
        /* Overwrite the original target before asking for the old copy. */
        for(unsigned i=0;i<6;i++)v[i].position[2]=-.1f;
        fh_renderer_draw(v,6,&s);flush_vertices();C3D_FrameSplit(GX_CMDLIST_FLUSH);
        queue_readback(saved_depth,264,400);fh_renderer_sync();
        GSPGPU_InvalidateDataCache(readback,DEPTH_BYTES);
        for(unsigned i=0;i<DEPTH_BYTES;i++){
            if(((unsigned char*)readback)[i]!=expected_depth[i])fh_platform_panic("Retained opaque depth changed after target overwrite");
            depth_checks++;
        }
        fh_renderer_present();
    }
    C3D_FrameSync();vramFree(saved_depth);free(expected_depth);
    FILE *depth_report=fopen("sdmc:/3ds/foxhollow/depth-preserve.txt","wb");
    if(!depth_report)fh_platform_panic("Depth retention result file");
    fprintf(depth_report,"%u\n",depth_checks);fclose(depth_report);
    fh_renderer_shutdown();FILE*f=fopen("sdmc:/3ds/foxhollow/completed.txt","wb");if(f){fputs("complete",f);fclose(f);}gfxExit();return 0;
}
