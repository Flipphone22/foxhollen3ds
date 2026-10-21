#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fh_graphics.h"
#include "fh_material.h"
#include "fh_gpu_lighting.h"
#include "fh_gpu_texgen.h"
#include "fh_platform.h"
#include "fh_fragment.h"
#include "fh_indirect_lighting.h"
unsigned fh_audio_synth_jobs_enabled;
/* Captured draw replay has no GX producer or asynchronous source leases. */
uint64_t fh_gx_phase_ticks[4],fh_gx_shading_ticks[3];
void fh_gx_dispatch_join_deferred(void) {}
#include "fh_fragment_raster.h"
#include "fh_compiled_geometry.h"
u32 __ctru_heap_size=24*1024*1024,__ctru_linear_heap_size=24*1024*1024,__stacksize__=512*1024;
static FILE *logfile;
static void replay_decode(FhVertex *v,const unsigned char *packet){memcpy(v,packet,sizeof(*v));}
static uint64_t replay_decode_batch(FhVertex *out,const FhCompiledMesh *mesh){
    if(!out)return 1;
    for(unsigned i=0;i<mesh->unique;i++)replay_decode(out+i,mesh->packets+mesh->offsets[i]);
    return 1; /* This replay owns the immutable complete vertex payload. */
}
#ifdef FH_RASTER_COMPARE
static unsigned raster_reference;
void fh_fragment_raster_reference(FhFragmentRaster*,unsigned,unsigned);
void __real_fh_fragment_raster_rows(FhFragmentRaster*,unsigned,unsigned);
void __wrap_fh_fragment_raster_rows(FhFragmentRaster *j,unsigned first,unsigned end) {
    if(raster_reference)fh_fragment_raster_reference(j,first,end);
    else __real_fh_fragment_raster_rows(j,first,end);
}
#endif
void fh_audio_pump(void) {}
void fh_platform_log(const char *s) {if(logfile){fputs(s,logfile);fflush(logfile);}}
void fh_platform_panic(const char *s) {FILE *f=fopen("sdmc:/3ds/foxhollow/panic.txt","wb");if(f){fputs(s,f);fclose(f);}exit(1);}
static void read_bytes(FILE *f,void *data,size_t bytes) {
    if(bytes && fread(data,1,bytes,f)!=bytes)fh_platform_panic("Truncated captured draw");
}
int main(void) {
    logfile=fopen("sdmc:/3ds/foxhollow/native.log","wb");
    FILE *f=fopen("sdmc:/3ds/foxhollow/draw-capture.bin","rb");
    if(!f)fh_platform_panic("No captured draw");
    char magic[8];uint32_t h[4];read_bytes(f,magic,8);read_bytes(f,h,sizeof(h));
    unsigned ordinary=!memcmp(magic,"FHDRAW04",8),texgen=ordinary || !memcmp(magic,"FHDRAW03",8),lit=!memcmp(magic,"FHDRAW03",8) || !memcmp(magic,"FHDRAW02",8);
    if((!lit && !ordinary && memcmp(magic,"FHDRAW01",8)) || h[1]<3 || h[1]>384 || h[1]%3 || h[2]!=sizeof(FhDrawState) || h[3]!=sizeof(FhVertex))fh_platform_panic("Captured draw ABI mismatch");
    FhDrawState s;static FhVertex vertices[384];read_bytes(f,&s,sizeof(s));read_bytes(f,vertices,h[1]*sizeof(*vertices));
    FhGpuLighting lighting;
    if(lit){if(!(s.raw_position&2))fh_platform_panic("Lighting capture flag missing");read_bytes(f,&lighting,sizeof(lighting));}
    else if(s.raw_position&2)fh_platform_panic("Lighting capture descriptor missing");
    float texgen_post[3][4];
    if(texgen){if(!(s.raw_position&4))fh_platform_panic("Texgen capture flag missing");read_bytes(f,texgen_post,sizeof(texgen_post));}
    else if(s.raw_position&4)fh_platform_panic("Texgen capture descriptor missing");
    unsigned total=0;
    for(unsigned i=0;i<8;i++){
        uint32_t n[2];read_bytes(f,n,sizeof(n));
        if(n[0]>4*1024*1024 || n[1]>32768 || total+n[0]+n[1]>16*1024*1024)fh_platform_panic("Captured texture size bound");
        total+=n[0]+n[1];
        if(n[0] && n[0]!=fh_texture_source_bytes(s.textures[i].format,s.textures[i].width,s.textures[i].height))fh_platform_panic("Captured texture layout mismatch");
        s.textures[i].pixels=n[0]?malloc(n[0]):NULL;s.textures[i].palette=n[1]?malloc(n[1]):NULL;
        if((n[0] && !s.textures[i].pixels) || (n[1] && !s.textures[i].palette))fh_platform_panic("Captured texture allocation failed");
        read_bytes(f,(void*)s.textures[i].pixels,n[0]);read_bytes(f,(void*)s.textures[i].palette,n[1]);
    }
    if(fgetc(f)!=EOF)fh_platform_panic("Unexpected captured draw trailer");fclose(f);
    f=fopen("sdmc:/3ds/foxhollow/fit_view.txt","rb");if(f){
        fclose(f);
        /* Diagnostic orthographic close-up, identical for both shader routes.
         * Retain captured topology, colors, texture coordinates and textures.
         * This deliberately changes the camera and is never gameplay evidence. */
        if(lit)fh_platform_panic("Diagnostic close-up needs captured CPU colors");
        float low[3]={INFINITY,INFINITY,INFINITY},high[3]={-INFINITY,-INFINITY,-INFINITY};
        for(unsigned i=0;i<h[1];i++){
            float p[3];memcpy(p,vertices[i].position,sizeof(p));
            if(s.raw_position){
                unsigned row=vertices[i].matrix_index;
                if(row>27 || row%3)fh_platform_panic("Diagnostic close-up matrix index");
                for(unsigned axis=0;axis<3;axis++){
                    const float *m=s.model_palette[row+axis];
                    vertices[i].position[axis]=m[0]*p[0]+m[1]*p[1]+m[2]*p[2]+m[3];
                }
            }
            for(unsigned axis=0;axis<3;axis++){
                float value=vertices[i].position[axis];
                if(!isfinite(value))fh_platform_panic("Diagnostic close-up nonfinite position");
                low[axis]=fminf(low[axis],value);high[axis]=fmaxf(high[axis],value);
            }
        }
        float extent=fmaxf((high[0]-low[0])*0.75f,high[1]-low[1]);
        if(extent<1e-6f)fh_platform_panic("Diagnostic close-up empty bounds");
        for(unsigned i=0;i<h[1];i++){
            for(unsigned axis=0;axis<2;axis++)vertices[i].position[axis]=(vertices[i].position[axis]-(high[axis]+low[axis])*.5f)*1.7f/extent*(axis?1:.75f);
            vertices[i].position[2]=high[2]>low[2]?-.9f+.8f*(vertices[i].position[2]-low[2])/(high[2]-low[2]):-.5f;
            vertices[i].position[3]=1;vertices[i].matrix_index=-1;
        }
        s.raw_position=0;s.viewport[0]=s.viewport[1]=0;s.viewport[2]=640;s.viewport[3]=480;s.viewport[4]=0;s.viewport[5]=1;
        s.scissor[0]=s.scissor[1]=0;s.scissor[2]=640;s.scissor[3]=480;
    }
    f=fopen("sdmc:/3ds/foxhollow/uncull.txt","rb");if(f){s.cull=0;fclose(f);}
#ifdef FH_RASTER_COMPARE
    f=fopen("sdmc:/3ds/foxhollow/raster_reference.txt","rb");if(f){raster_reference=1;fclose(f);}
#endif
    gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false);osSetSpeedupEnable(true);
    fh_renderer_init();
    fh_renderer_clear(0x203040ff,0xffffff);
    if(lit && !fh_renderer_prepare_lighting(&s,&lighting))fh_platform_panic("Captured lighting material rejected");
    if(texgen && !(ordinary?fh_renderer_prepare_uvgen(&s,texgen_post):fh_renderer_prepare_texgen(&s,texgen_post)))fh_platform_panic("Captured texgen material rejected");
    unsigned repeats=1,uncached=0,compiled=0,gpu_shell=0,zero_second=0;
    f=fopen("sdmc:/3ds/foxhollow/zero_second_fur.txt","r");if(f){zero_second=1;fclose(f);}
    extern unsigned fh_gpu_fur_disabled,fh_gpu_fur_draws,fh_gpu_fur_replays;
    f=fopen("sdmc:/3ds/foxhollow/gpu_fur.txt","r");if(f){unsigned enable=0;if(fscanf(f,"%u",&enable)!=1)fh_platform_panic("GPU shell flag invalid");fh_gpu_fur_disabled=!enable;gpu_shell=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/reflective_candidate.txt","r");if(f){fh_reflective_disabled=0;fclose(f);}
    {
        unsigned eligible=0;FhDrawState cpu=s;cpu.raw_position=0;
        for(unsigned i=0;i<h[1];i+=3){FhVertex tri[3];memcpy(tri,vertices+i,sizeof(tri));
            for(unsigned j=0;j<3;j++)if(s.raw_position && tri[j].matrix_index>=0){
                float clip[4];fh_vertex_clip_position(clip,tri[j].position,(unsigned)tri[j].matrix_index,&s);memcpy(tri[j].position,clip,sizeof(clip));tri[j].matrix_index=-1;
            }
            eligible+=fh_reflective_triangle(tri,&cpu);
        }
        fprintf(logfile,"Reflection matcher=%d eligible=%u/%lu\n",fh_reflective_material(&s),eligible,(unsigned long)h[1]/3);
    }
    f=fopen("sdmc:/3ds/foxhollow/reflective_reference.txt","r");if(f){fh_reflective_disabled=1;fclose(f);}
    extern unsigned fh_unmasked_fur_disabled;
    f=fopen("sdmc:/3ds/foxhollow/unmasked_fur_reference.txt","r");if(f){fh_unmasked_fur_disabled=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/indirect_lighting.txt","r");if(f){fh_indirect_lighting_enabled=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/chained_reference.txt","r");if(f){fh_chained_reference_bounds=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/chained_scalar.txt","r");if(f){fh_chained_disabled=1;fclose(f);}
    extern unsigned fh_product_mask_disabled;
    extern unsigned fh_layered_five_tint_disabled;
    extern unsigned fh_fragment_native_grid;
    f=fopen("sdmc:/3ds/foxhollow/native_scalar_grid.txt","r");if(f){fh_fragment_native_grid=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/layered_tint_reference.txt","r");if(f){fh_layered_five_tint_disabled=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/product_mask_reference.txt","r");if(f){fh_product_mask_disabled=1;fclose(f);}
    extern unsigned fh_vertex_reference,fh_vertex_unique,fh_vertex_reused;
    /* Recover exact identities once from this legacy expanded capture. The
     * live GX decoder supplies identities during its existing cache lookup. */
    uint32_t identities[384];
    for(unsigned i=0;i<h[1];i++){
        identities[i]=i+1;
        for(unsigned j=0;j<i;j++)if(!memcmp(vertices+i,vertices+j,sizeof(*vertices))){identities[i]=identities[j];break;}
    }
    uint32_t offsets[384];uint16_t mesh_indices[384];unsigned unique=0,compiled_draws=0,rejections=0;
    for(unsigned i=0;i<h[1];i++){
        unsigned j;for(j=0;j<unique;j++)if(!memcmp((unsigned char*)vertices+offsets[j],vertices+i,sizeof(*vertices)))break;
        if(j==unique)offsets[unique++]=i*sizeof(*vertices);mesh_indices[i]=j;
    }
    FhCompiledMesh mesh={(const unsigned char*)vertices,offsets,mesh_indices,unique,h[1]};
    f=fopen("sdmc:/3ds/foxhollow/compiled_fur.txt","r");if(f){compiled=1;fclose(f);}
    if(compiled){
        FhCompiledMesh oversized=mesh;oversized.unique=1025;
        if(fh_renderer_draw_compiled(&oversized,&s,replay_decode,NULL))fh_platform_panic("Oversized compiled fur did not fall back");rejections++;
        FhVertex *first=(FhVertex*)((unsigned char*)vertices+offsets[mesh_indices[0]]);
        float tag=first->matrix_index;first->matrix_index=-1;
        if(fh_renderer_draw_compiled(&mesh,&s,replay_decode,NULL))fh_platform_panic("CPU clip vertex did not fall back");rejections++;first->matrix_index=tag;
        unsigned selected=~0u;
        for(unsigned i=0;i<mesh.count;i+=3)if(mesh.indices[i]!=mesh.indices[i+1]){selected=mesh.indices[i];break;}
        if(selected==~0u)fh_platform_panic("Captured mesh has no distinct triangle for Q regression");
        FhVertex *q=(FhVertex*)((unsigned char*)vertices+offsets[selected]);float saved=q->uv[0][2];q->uv[0][2]=saved+1;
        if(fh_renderer_draw_compiled(&mesh,&s,replay_decode,NULL))fh_platform_panic("Mixed projective Q did not fall back");rejections++;q->uv[0][2]=saved;
    }
    f=fopen("sdmc:/3ds/foxhollow/vertex_reference.txt","r");if(f){fh_vertex_reference=1;fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/repeats.txt","r");if(f){if(fscanf(f,"%u %u",&repeats,&uncached)!=2 || !repeats || repeats>1024)fh_platform_panic("Invalid replay repetition count");fclose(f);}
    for(unsigned i=0;i<repeats;i++){
        if(gpu_shell){
            float strength=(i%16)/32.f;
            s.indirect_matrix[0][0][0]=s.indirect_matrix[0][1][1]=strength;
            if(zero_second){
                if(s.tev[3].channel!=255)fh_platform_panic("Zero indirect replay needs an unmasked capture");
                memset(s.indirect_matrix[1],0,sizeof(s.indirect_matrix[1]));s.indirect_exponent[1]=-15;
            }else s.indirect_matrix[1][0][1]=s.indirect_matrix[1][1][2]=strength;
        }
        if(uncached)fh_fur_cache_invalidate();
        if(compiled && fh_renderer_draw_compiled(&mesh,&s,replay_decode,gpu_shell?replay_decode_batch:NULL))compiled_draws++;
        else fh_renderer_draw_identified(vertices,h[1],&s,identities);
        /* Diagnostic layer sequence, identical for each compared route. */
        if(repeats>1 && !gpu_shell)s.indirect_matrix[0][0][0]+=.0001f;
    }
    fh_renderer_present();
    f=fopen("sdmc:/3ds/foxhollow/indirect-lighting.json","wb");if(f){fprintf(f,"{\"triangles\":%u,\"enabled\":%u}\n",fh_indirect_lighting_triangles,fh_indirect_lighting_enabled);fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/gpu-fur.json","wb");if(f){fprintf(f,"{\"draws\":%u,\"replays\":%u}\n",fh_gpu_fur_draws,fh_gpu_fur_replays);fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/reflective.json","wb");if(f){fprintf(f,"{\"triangles\":%u,\"fallbacks\":%u,\"lod_plans\":%u}\n",fh_reflective_triangles,fh_reflective_fallbacks,fh_reflective_lod_plans);fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/compiled.json","wb");if(f){fprintf(f,"{\"requested\":%u,\"draws\":%u,\"rejections\":%u,\"unique_per_draw\":%u,\"indices_per_draw\":%lu}\n",compiled,compiled_draws,rejections,unique,(unsigned long)h[1]);fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/vertices.json","wb");if(f){fprintf(f,"{\"unique\":%u,\"reused\":%u}\n",fh_vertex_unique,fh_vertex_reused);fclose(f);}
    unsigned active,core,jobs,peak;fh_fragment_worker_stats(&active,&core,&jobs,&peak);
    f=fopen("sdmc:/3ds/foxhollow/worker.json","wb");if(f){fprintf(f,"{\"active\":%u,\"core\":%u,\"jobs\":%u,\"stack_peak_upper_bound\":%u}\n",active,core,jobs,peak);fclose(f);}
    fh_renderer_shutdown();
    for(unsigned i=0;i<8;i++){free((void*)s.textures[i].pixels);free((void*)s.textures[i].palette);}
    f=fopen("sdmc:/3ds/foxhollow/completed.txt","wb");if(f){fprintf(f,"source_frame=%lu vertices=%lu texture_bytes=%u\n",(unsigned long)h[0],(unsigned long)h[1],total);fclose(f);}
    if(logfile)fclose(logfile);gfxExit();return 0;
}
