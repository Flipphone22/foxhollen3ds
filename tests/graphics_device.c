#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fh_graphics.h"
#include "fh_platform.h"
#include "fh_gpu_transfer.h"
#include "fur_fixture.h"
/* The hardware failure involved raw VRAM -> CPU readback. Azahar accepts
 * that command, so reject it before submission in this renderer fixture.
 * Tiled VRAM -> VRAM presentation copies remain allowed. */
Result __real_GX_TextureCopy(u32*,u32,u32*,u32,u32,u32);
extern const char *fh_renderer_texture_copy_submission_error(uintptr_t,size_t,unsigned,uintptr_t,size_t,unsigned,unsigned,unsigned);
Result __wrap_GX_TextureCopy(u32 *source,u32 input,u32 *destination,u32 output,u32 bytes,u32 flags) {
    uintptr_t address=(uintptr_t)destination;
#ifdef FH_LEGACY_GAPPED_COPY
    flags&=~4u; /* Restore the command captured in the physical crash. */
#endif
    const char *error=fh_renderer_texture_copy_submission_error((uintptr_t)source,SIZE_MAX,input,
        address,SIZE_MAX,output,bytes,flags);
    if(error)fh_platform_panic(error);
    return __real_GX_TextureCopy(source,input,destination,output,bytes,flags);
}
#ifdef FH_CAPTURE_DMA_VERIFY
Result __real_GX_RequestDma(u32*,u32*,u32);
extern const char *fh_renderer_capture_dma_error(uintptr_t,uintptr_t,unsigned);
Result __wrap_GX_RequestDma(u32 *source,u32 *destination,u32 bytes){
    const char *error=fh_renderer_capture_dma_error((uintptr_t)source,(uintptr_t)destination,bytes);
    if(error)fh_platform_panic(error);
    return __real_GX_RequestDma(source,destination,bytes);
}
#endif
u32 __ctru_heap_size=8*1024*1024;
#if defined(FH_TEXTURE_ONLY) || defined(FH_EFB_COLOR_ONLY)
/* Fragmentation and simultaneous color/depth copy fixtures use the production
 * game's linear heap budget, including its unchanged four-MiB reserve. */
u32 __ctru_linear_heap_size=24*1024*1024;
#else
u32 __ctru_linear_heap_size=12*1024*1024;
#endif
u32 __stacksize__=512*1024;
static FILE *logfile;
static unsigned fragment_test;
static unsigned gpu_transform_test;
static unsigned sphere_transform_test;
static unsigned movie_fixture_case;
#include "fh_compiled_geometry.h"
static void sphere_compiled_decode(FhVertex *v,const unsigned char *p){memcpy(v,p,sizeof(*v));}
static unsigned char pixels[5][8*8*4],lights[2][8*8*4];
static unsigned char fur_noise[128];
void fh_audio_pump(void) {}
unsigned fh_audio_synth_jobs_enabled; /* This renderer-only fixture has no NDSP worker. */
/* Direct renderer fixture: no GX producer/dispatcher executes or queues work. */
uint64_t fh_gx_phase_ticks[4],fh_gx_shading_ticks[3];
void fh_gx_dispatch_join_deferred(void) {}
void fh_gx_dispatch_barrier(void) {}
/* This fixture drives the renderer directly; no game scene or independent
 * reconstruction backend is loaded. Fail if a test accidentally selects one. */
unsigned fh_render_floor,fh_render_floor_copies,fh_minimal_enabled,fh_minimal_frames;
unsigned fh_native_scene_enabled,fh_native_scene_cpu_bytes,fh_native_scene_gpu_bytes;
unsigned fh_native_model_admission[16],fh_surface_detail;
int fh_minimal_ready(void){return 0;}
void fh_minimal_init(void){fh_platform_panic("Fixture selected reconstruction backend");}
void fh_minimal_sync(void){fh_platform_panic("Unexpected reconstruction sync");}
void fh_minimal_present(void){fh_platform_panic("Unexpected reconstruction present");}
void fh_minimal_shutdown(void){fh_platform_panic("Unexpected reconstruction shutdown");}
unsigned fh_minimal_prepare_write(unsigned frame){(void)frame;fh_platform_panic("Unexpected reconstruction write");return 0;}
void fh_minimal_texture_written(const void *p,size_t n){(void)p;(void)n;}
void fh_native_scene_texture_written(const void *p,size_t n){(void)p;(void)n;}
void fh_native_scene_shutdown(void){}
void fh_platform_log(const char *s) { if(logfile) {fputs(s,logfile);fflush(logfile);} }
void fh_platform_panic(const char *s) { FILE *f=fopen("sdmc:/3ds/foxhollow/panic.txt","wb");if(f){fputs(s,f);fclose(f);}exit(1); }
static void cell(unsigned index) {
    FhDrawState s={0};s.viewport[2]=640;s.viewport[3]=480;s.viewport[5]=1;s.scissor[2]=640;s.scissor[3]=480;
    s.color_write=s.alpha_write=1;s.depth_compare=7;s.alpha_compare[0]=s.alpha_compare[3]=7;s.blend_src=1;
    if(!index) s.depth_test=s.depth_write=1;
    if(!index && (fragment_test==4 || fragment_test==5 || (fragment_test>=7 && fragment_test<=10))) {
        s.fog_type=fragment_test>=7?fragment_test-3:fragment_test==4?2:10;s.fog_start=s.fog_near=10;s.fog_end=s.fog_far=100;s.fog_color=0x1464c8ff;
    }
    if(fragment_test==11){s.viewport[2]=s.viewport[3]=512;s.scissor[2]=s.scissor[3]=512;}
    s.stages=1;s.texgens=3;
    for(unsigned i=0;i<4;i++) for(unsigned c=0;c<4;c++) s.swaps[i][c]=c;
    for(unsigned i=0;i<5;i++) s.textures[i]=(FhTexture){.pixels=pixels[i],.width=8,.height=8,.format=0x46};
    s.tev[0]=(FhTevStage){.color={15,8,12,15},.alpha={7,4,6,7},.color_clamp=1,.alpha_clamp=1,.channel=4};
    if(!index && fragment_test==1) {s.tev[0].indirect[3]=1;s.indirect_count=1;}
    if(!index && fragment_test==2) {
        fur_fixture(&s);s.textures[3].pixels=fur_noise;s.textures[3].format=3;
        for(unsigned c=0;c<3;c++)s.tev_colors[3][c]=(c+1)*.2f;
        s.tev_colors[3][3]=-.1f;
        if(!fh_fur_material(&s))fh_platform_panic("Fur fixture specialization rejected");
    }
    if(index) {
        s.stages=2;s.tev[1]=s.tev[0];s.tev[1].color[2]=0;s.tev[1].alpha[2]=0;s.tev[1].map=s.tev[1].coord=1;
    }
    if(index==2) {
        s.stages=3;s.tev[2]=s.tev[1];s.tev[2].map=s.tev[2].coord=2;
    }
    if(index==1 && fragment_test==3) {
        s.textures[1]=s.textures[0];s.textures[1].wrap_s=1;
    }
    if(index==3) {
        const unsigned colors[7][4]={{15,8,14,10},{15,8,4,15},{15,6,8,15},{15,8,14,15},{8,0,1,15},{15,0,4,6},{0,8,14,15}};
        const unsigned alphas[7][4]={{7,7,7,7},{7,4,3,7},{7,7,7,7},{7,4,6,7},{7,4,0,7},{7,7,7,0},{7,7,7,0}};
        const unsigned maps[]={0,1,2,2,3,255,4},coords[]={0,0,1,1,2,255,3},dest[]={2,3,3,0,0,0,0};
        s.stages=7;s.alpha_write=0;
        for(unsigned i=0;i<7;i++) {
            s.tev[i]=(FhTevStage){.color_clamp=1,.alpha_clamp=1,.map=maps[i],.coord=coords[i],.color_dest=dest[i],.channel=i==0?4:255};
            memcpy(s.tev[i].color,colors[i],16);memcpy(s.tev[i].alpha,alphas[i],16);
        }
        s.tev[1].indirect[4]=s.tev[1].indirect[5]=6;s.tev[1].indirect[6]=1;
        s.tev[1].tex_swap=3;for(unsigned c=0;c<4;c++)s.swaps[3][c]=c==3?1:2;
        for(unsigned i=0;i<2;i++) {s.textures[i].format=6;s.textures[i].pixels=lights[i];}
        s.tev[6].kcolor=6;
    }
    if(sphere_transform_test){
        s.stages=2;s.alpha_write=0;
        for(unsigned i=0;i<2;i++)s.tev[i]=(FhTevStage){.color={15,10,11,i?0:8},.alpha={7,7,7,7},.color_clamp=1,.alpha_clamp=1,.channel=4+i,.map=0,.coord=0};
    }
    FhVertex v[6]={0};unsigned corners[]={0,1,2,0,2,3};float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    float left=-1.f+(index%2),top=1.f-(index/2);
    for(unsigned i=0;i<6;i++) {
        unsigned k=corners[i];v[i].position[0]=left+xy[k][0];v[i].position[1]=top-xy[k][1];v[i].position[2]=-.5f;v[i].position[3]=1;
        if(fragment_test==11){v[i].position[0]=-1+2*xy[k][0];v[i].position[1]=1-2*xy[k][1];}
        for(unsigned c=0;c<8;c++) ((float*)v[i].color)[c]=1;
        for(unsigned t=0;t<8;t++) {v[i].uv[t][0]=xy[k][0];v[i].uv[t][1]=xy[k][1];v[i].uv[t][2]=1;}
        if(index==3) for(unsigned c=0;c<3;c++)v[i].color[0][c]=(c+1)/10.f;
        if(sphere_transform_test)for(unsigned c=0;c<8;c++)((float*)v[i].color)[c]=.1f+.06f*((k*3+c)%11);
        if(index==1 && fragment_test==3)for(unsigned t=0;t<2;t++){v[i].uv[t][0]=1.25f;v[i].uv[t][1]=.25f;}
        if(!index && fragment_test==2)for(unsigned c=0;c<3;c++)v[i].color[0][c]=(c+1)*.1f;
        if(!index && fragment_test!=2) {
            float w=1+xy[k][0];for(unsigned c=0;c<4;c++) v[i].position[c]*=w;
            for(unsigned c=0;c<3;c++) v[i].uv[0][c]*=w;
        }
    }
    if(gpu_transform_test && index!=2){
        s.raw_position=1;s.position_matrix[0][0]=2;s.position_matrix[1][1]=.5f;s.position_matrix[2][2]=s.position_matrix[3][3]=1;
        s.position_matrix[0][3]=5+7*index;s.position_matrix[1][3]=-3;
        for(unsigned slot=0;slot<2;slot++)for(unsigned axis=0;axis<3;axis++)s.model_palette[slot*3+axis][axis]=1;
        s.model_palette[3][3]=4;
        for(unsigned i=0;i<6;i++){
            if(sphere_transform_test!=2 && i%3==2 && movie_fixture_case<=18){v[i].matrix_index=-1;continue;}
            v[i].position[0]=(v[i].position[0]-s.position_matrix[0][3]*v[i].position[3])*.5f;
            v[i].position[1]=(v[i].position[1]+3*v[i].position[3])*2;
            v[i].matrix_index=movie_fixture_case>18?0:sphere_transform_test==2?(i/3)*3:(i&1)?3:0;
            if(v[i].matrix_index==3)v[i].position[0]-=4*v[i].position[3];
        }
    }
    if(sphere_transform_test==3 && s.raw_position){
        uint32_t offsets[6];uint16_t indices[]={0,1,2,3,4,5};
        for(unsigned i=0;i<6;i++)offsets[i]=i*sizeof(FhVertex);
        FhCompiledMesh mesh={(const unsigned char*)v,offsets,indices,6,6};
        if(!fh_renderer_draw_compiled(&mesh,&s,sphere_compiled_decode,NULL))fh_platform_panic("Compiled sphere fixture declined");
    }else {
        if(index==3 && movie_fixture_case){
            /* Overlapping triangles with independently varying base colors and
             * movie UVs expose an invalid all-base-then-all-overlay reorder. */
            unsigned variant=(movie_fixture_case-1)%12+1;
            s.tev[6].kcolor=(variant-1)%8;
            s.depth_test=variant>4;s.depth_write=variant>8;
            s.depth_compare=variant%3==0?7:variant%3==1?3:6;
            for(unsigned i=3;i<6;i++){
                if(movie_fixture_case<=12)v[i]=v[i-3];
                v[i].color[0][0]=.85f;v[i].color[0][1]=.15f;
                v[i].uv[3][0]=.8f;v[i].uv[3][1]=.2f;
                if(movie_fixture_case<=12 && (movie_fixture_case&1))v[i].position[2]=-.6f;
            }
        }
#ifdef FH_ORDERED_PROGRAM_TEST
        if(index==3 && movie_fixture_case){
            FhVertex repeated[42];
            for(unsigned i=0;i<42;i++)repeated[i]=v[i%6];
            fh_renderer_draw(repeated,42,&s);
        }else
#endif
        fh_renderer_draw(v,6,&s);
    }
}
#include "movie_batch_fixture.h"
#include "blur_fixture.h"
#include "affine_fixture.h"
#include "indirect_st_fixture.h"
#include "tev_compare_fixture.h"
#include "tev_subtract_fixture.h"
#include "layered_five_fixture.h"
#include "vertex_reuse_fixture.h"
#include "raster_channel_fixture.h"
#include "gpu_lighting_fixture.h"
#include "sphere_lighting_fixture.h"
#include "fur_texgen_fixture.h"
#include "native_geometry_fixture.h"
#include "gpu_points_fixture.h"
#include "efb_readback_fixture.h"
#include "pica_texture_fixture.h"
#include "texture_fragmentation_fixture.h"
#include "efb_color_fixture.h"
#include "efb_shadow_fixture.h"
#include "reflective_fixture.h"
#include "fragment_batch_fixture.h"
int main(void) {
    extern unsigned fh_sphere_reference;
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow",0777);logfile=fopen("sdmc:/3ds/foxhollow/native.log","wb");
    gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false);osSetSpeedupEnable(true);
    /* CPU bounds snapshots must leave GPU-accessible linear RAM untouched. */
    {
        extern void *fh_renderer_cpu_cache_alloc(unsigned);
        unsigned before=linearSpaceFree();
        unsigned char *cache=fh_renderer_cpu_cache_alloc(1024*1024);
        if(!cache || ((uintptr_t)cache&31) || linearSpaceFree()!=before)
            fh_platform_panic("CPU bounds allocation consumed linear RAM or failed alignment");
        for(unsigned i=0;i<1024*1024;i++)cache[i]=(unsigned char)(i^(i>>8));
        for(unsigned i=0;i<1024*1024;i++)if(cache[i]!=(unsigned char)(i^(i>>8)))
            fh_platform_panic("CPU bounds allocation memory mismatch");
        free(cache);
    }
#ifdef FH_EFB_COLOR_ONLY
    efb_color_device_test();goto finish;
#endif
#ifdef FH_TEXTURE_ONLY
    pica_texture_device_test();texture_fragmentation_device_test();goto finish;
#endif
#ifdef FH_SPHERE_LIGHTING_ONLY
    sphere_lighting_device_test();goto finish;
#endif
#ifdef FH_SHADOW_COPY_ONLY
    efb_shadow_device_test();goto finish;
#endif
#ifdef FH_READBACK_ONLY
    efb_readback_device_test();goto finish;
#endif
#ifdef FH_FRAGMENT_BATCH_ONLY
    fragment_batch_device_test();goto finish;
#endif
    const unsigned char colors[4][4]={{200,80,40,240},{40,200,80,160},{80,40,200,80},{160,120,200,200}};
    for(unsigned i=0;i<64;i++){fur_noise[2*i]=192;fur_noise[2*i+1]=96;}
    for(unsigned y=0;y<8;y++) for(unsigned x=0;x<8;x++) {
        unsigned k=(y/4)*2+x/4;memcpy(pixels[0]+(y*8+x)*4,colors[k],4);
        const unsigned char b[]={128,64,192,128},c[]={192,128,64,128},d[]={64,160,128,192},movie[]={120,200,80,255};
        memcpy(pixels[1]+(y*8+x)*4,b,4);memcpy(pixels[2]+(y*8+x)*4,c,4);
        memcpy(pixels[3]+(y*8+x)*4,d,4);memcpy(pixels[4]+(y*8+x)*4,movie,4);
        for(unsigned i=0;i<2;i++) {
            unsigned char *dst=lights[i]+((y/4)*2+x/4)*64;unsigned q=(y%4)*4+x%4;const unsigned char *src=pixels[i]+(y*8+x)*4;
            dst[2*q]=src[3];dst[2*q+1]=src[0];dst[32+2*q]=src[1];dst[32+2*q+1]=src[2];
        }
    }
#ifdef FH_LIGHTING_ONLY
    gpu_lighting_device_test();fur_texgen_device_test();native_geometry_device_test();gpu_points_device_test();goto finish;
#endif
#ifdef FH_REFLECTION_LOD_ONLY
    reflective_device_test();goto finish;
#endif
#ifdef FH_LAYERED_ONLY
    layered_five_device_test();goto finish;
#endif
#ifdef FH_LIGHT_LOD_ONLY
    fh_platform_log("Points before reduced lighting\n");gpu_points_device_test();
    reduced_lighting_device_test();
    fh_platform_log("Points after reduced lighting\n");gpu_points_device_test();goto finish;
#endif
#ifdef FH_MOVIE_ONLY
    movie_batch_device_test();goto finish;
#endif
    unsigned char transform_reference[16][64];unsigned transform_checks=0;
#ifdef FH_NATIVE_FOG_ONLY
    const unsigned first_frame=13,last_frame=20;
#else
    const unsigned first_frame=0,last_frame=29;
#endif
    for(unsigned frame=first_frame;frame<last_frame && aptMainLoop();frame++) {
        fh_sphere_reference=frame==24;
        fragment_test=frame>=13?frame-9:frame==12?3:frame==11?2:frame==9?1:0;
        if(frame>=21)fragment_test=frame==23?1:0;
        gpu_transform_test=frame>=22 && frame!=24 && frame!=27;
        sphere_transform_test=frame>=24 && frame<=26?frame-23:0;
        if(frame>=27)fragment_test=2;
        if(frame==4 || frame==5) {
            for(unsigned y=0;y<4;y++)for(unsigned x=0;x<4;x++)pixels[0][(y*8+x)*4]=frame==4?33:200;
            fh_renderer_invalidate(pixels[0],sizeof(pixels[0]));
        }
        cell(0);
        if(frame>=13 && frame<20) {
            unsigned char copy[64];uint32_t rect[4]={80,60,8,8};FhDrawState state={.pixel_format=1};
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
            char path[96];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/fog-%u.bin",frame);
            FILE *f=fopen(path,"wb");fwrite(copy,1,64,f);fclose(f);
        }
        if(frame==20) {
            unsigned char copy[64];uint32_t rect[4]={80,500,8,8};FhDrawState state={.pixel_format=1};
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
            if(copy[1]!=80 || copy[32]!=40 || copy[33]!=200 || copy[0]!=80)fh_platform_panic("Offscreen shadow EFB color mismatch");
            if(abs((int)fh_renderer_depth(80,500)-8388607)>2)fh_platform_panic("Offscreen shadow EFB depth mismatch");
            FILE *f=fopen("sdmc:/3ds/foxhollow/shadow.bin","wb");fwrite(copy,1,64,f);fclose(f);
        }
        if(frame==11) {
            unsigned char copy[64];uint32_t rect[4]={80,60,8,8};FhDrawState state={.pixel_format=1};
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
            FILE *f=fopen("sdmc:/3ds/foxhollow/fur.bin","wb");fwrite(copy,1,64,f);fclose(f);
            for(unsigned i=0;i<64;i++)pixels[2][i*4+3]=248;
            fh_renderer_invalidate(pixels[2],sizeof(pixels[2]));cell(0);
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
            f=fopen("sdmc:/3ds/foxhollow/fur-mutated.bin","wb");fwrite(copy,1,64,f);fclose(f);
        }
        if(frame==4) {
            unsigned char copy[64];uint32_t rect[4]={80,60,8,8};FhDrawState state={0};
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
            if(copy[1]!=33 || copy[32]!=80 || copy[33]!=40)fh_platform_panic("Texture source mutation was not visible");
        }
        for(unsigned repeat=0;repeat<(frame>=24?1u:32u);repeat++) for(unsigned i=1;i<4;i++){cell(i);cell(i);}
        if(frame==12) {
            unsigned char copy[64];uint32_t rect[4]={400,60,8,8};FhDrawState state={0};
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
            FILE *f=fopen("sdmc:/3ds/foxhollow/sampler.bin","wb");fwrite(copy,1,64,f);fclose(f);
        }
        if(frame==9) {
            unsigned char copy[64];uint32_t rect[4]={80,60,8,8};FhDrawState state={0};
            FILE *f=fopen("sdmc:/3ds/foxhollow/efb.bin","wb");
            fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);fwrite(copy,1,64,f);
            fh_renderer_copy(copy,rect,4,4,0x11,1,0,0,0,&state);fwrite(copy,1,32,f);
            fh_renderer_copy(copy,rect,4,4,0x13,1,0,0,0,&state);fwrite(copy,1,32,f);
            uint32_t depths[]={fh_renderer_depth(80,60),fh_renderer_depth(400,300)};
            fwrite(depths,4,2,f);fclose(f);
        }
        if(frame>=21){
            FhDrawState state={.pixel_format=1,.color_write=1,.alpha_write=1,.depth_write=1};
            for(unsigned item=0;item<16;item++){
                unsigned cell=item/4,corner=item%4;
                uint32_t rect[]={80+(cell%2)*320+(corner%2)*160,(cell/2)*240+60+(corner/2)*120,8,8};
                unsigned char copy[64];fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
                if(frame==21 || frame==24 || frame==27)memcpy(transform_reference[item],copy,64);
                else for(unsigned pixel=0;pixel<16;pixel++){
                    unsigned indices[]={2*pixel+1,32+2*pixel,33+2*pixel};
                    for(unsigned c=0;c<3;c++){
                        unsigned index=indices[c];
                        if(abs((int)copy[index]-transform_reference[item][index])>2)fh_platform_panic("GPU transform differs from CPU reference");
                        transform_checks++;
                    }
                }
            }
            if(frame==22){
                unsigned char copy[64];uint32_t rect[]={80,60,8,8};
                fh_renderer_copy(copy,rect,4,4,6,1,1,0x112233ff,0x800000,&state);
                fh_renderer_copy(copy,rect,4,4,6,1,0,0,0,&state);
                if(copy[1]!=17 || copy[32]!=34 || copy[33]!=51 || copy[0]!=255)fh_platform_panic("GPU transform leaked into EFB clear");
                transform_checks++;
            }
        }
        fh_renderer_present();fh_renderer_invalidate(NULL,0);gspWaitForVBlank();
    }
#ifdef FH_NATIVE_FOG_ONLY
    goto finish;
#endif
    FILE *transform_report=fopen("sdmc:/3ds/foxhollow/transforms.txt","wb");
    if(!transform_report)fh_platform_panic("GPU transform report open failed");
    fprintf(transform_report,"%u\n",transform_checks);fclose(transform_report);
    blur_device_test();
    affine_device_test();
    indirect_st_device_test();
    tev_compare_device_test();
    tev_subtract_device_test();
    layered_five_device_test();
    reflective_device_test();
    vertex_reuse_device_test();
    raster_channel_device_test();
    gpu_lighting_device_test();
    fur_texgen_device_test();
    native_geometry_device_test();
    gpu_points_device_test();
    pica_texture_device_test();
finish:
    fh_renderer_shutdown();FILE *f=fopen("sdmc:/3ds/foxhollow/completed.txt","wb");if(f){fputs("complete",f);fclose(f);}fclose(logfile);gfxExit();return 0;
}
