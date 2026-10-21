#include "fh_pica_world_stream.h"
unsigned fh_pica_world_water_active;
#include "fh_pica_world_packet_owner.h"
#include "fh_pica_actor_stream.h"
#include "fh_pica_color_fade.h"
#include "fh_pica_actor_live.h"
#include "fh_pica_fur_groom.h"
#include "fh_pica_texture_swizzle.h"
#include "fh_pica_diffuse_map.h"
#include "fh_pica_sampler.h"
#include "fh_pica_texture_coordinates.h"
#include "fh_pica_specular.h"
#include "fh_pica_layered.h"
#include "fh_pica_layered_specular_plan.h"
#include "fh_pica_postlight_specular_plan.h"
#include "fh_pica_actor_raster.h"
#include "fh_pica_fog.h"
#include "fh_graphics_options.h"
#include "fh_pica_options.h"
/* Native resident images/materials. All decoding, source verification and SD
 * reads occur at material loading; draw binding only publishes small state.
 * No calls to the legacy texture resolver, GX interpreter or prepared queue. */
#include <3ds.h>
#include <citro3d.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "fh_pica_scene.h"
#include "fh_pica_texture.h"
#include "fh_pica_image.h"
#include "fh_texture_pack.h"
#include "fh_asset_paths.h"
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_hardware.h"
#include "fh_minimal_graphics.h"
#include "fh_pica_hologram.h"
#include "fh_pica_ui_model.h"
#include "fh_pica_movie.h"
#include "fh_pica_cloud.h"
enum { IMAGES=512, MATERIAL_BYTES=512*1024 };
typedef struct {C3D_Tex gpu;unsigned refs,offset,bytes,source_width,source_height,mip_bytes;} Image;
typedef struct FhPicaWorldCommands FhPicaWorldCommands;
typedef struct FhPicaActorCommands FhPicaActorCommands;
typedef struct {unsigned handle;C3D_Tex sampler;float scale[2];const FhPicaSamplerCommands *commands;} Frame;
typedef struct {
    Frame *frames[2];unsigned count[2],current[2],flags,layers,ready,intensity[2];
    C3D_TexEnv env[4];float tint[4],scroll[2][2];unsigned konst,actor,masked,layered;
    C3D_TexEnv lit_env[4];unsigned environment,environment_slot,environment_strength,program;
    Frame *fur_mask;unsigned fur_mask_intensity;
    C3D_Tex *projected_samplers; /* Only hologram materials allocate this at load. */
    FhPicaUiMaterial ui; /* Live semantic callback state; no per-draw GX callback. */
    unsigned specular_program,specular_slot,specular_alpha,alpha_constant;
    /* SPECULAR_PREFETCH_V1: only item zero stores the aggregate load-time mask. */
    unsigned specular_demand;
    unsigned diffuse_program;
    unsigned swizzle_program,swizzle_source,swizzle,swizzle_environment[3],swizzle_diffuse[4];
    unsigned post_state;
    unsigned actor_fog;
    uint32_t actor_raster_plan;unsigned actor_raster;
    const void *movie_pixels;unsigned movie_active;
    unsigned cloud_state;
    unsigned moon_alpha;float moon_phase;
    FhPicaWorldCommands *world_commands;
    FhPicaActorCommands *actor_commands;
} Material;
static int actor_stream_prepare(const FhSceneAsset*,FhPicaMaterialSet*);
static void actor_stream_release(FhPicaMaterialSet*);
static int world_stream_prepare(const FhSceneAsset*,FhPicaMaterialSet*);
static void world_stream_release(FhPicaMaterialSet*);
#include "fh_pica_height_fog.h"
#include "fh_pica_actor_fog_tev.h"
enum { PROGRAMS=128 };
typedef struct {C3D_TexEnv stages[4];unsigned refs,buffer_mask,actor_height;FhPicaActorMaterial commands;} Program;
static Program programs[PROGRAMS];
static unsigned actor_height_binding,actor_height_color;
static unsigned bound_program,bound_colors[4],program_binds,program_reuses,program_peak,program_count;
static unsigned ui_program[16],ui_draws[5],ui_hidden[5];
static unsigned post_draws[3];
unsigned fh_pica_cloud_mode,fh_pica_cloud_glare_draws;
unsigned fh_pica_celestial_updates;
static unsigned cloud_draws[2];
static unsigned actor_live_publications;
static unsigned diffuse_admitted,diffuse_partial,diffuse_bindings,diffuse_unready;
static unsigned diffuse_mask_bindings,diffuse_layered_bindings,diffuse_shared_bindings,diffuse_plain_bindings;
static unsigned specular_bindings,specular_unready,specular_mask_bindings,specular_postlight_bindings;
static unsigned alpha_constant_materials,alpha_replaced_masks;
static unsigned moon_program[2],moon_draws;
static Image images[IMAGES];static FhTexturePack pack;
static unsigned gpu_bytes,gpu_peak,cpu_bytes,cpu_peak,loads,conversions,errors,bindings;
static u64 load_ticks;
enum{MIP_MARGIN=1024*1024};
static unsigned mip_images,mip_failures,mip_skipped;
static unsigned swizzle_admitted,swizzle_partial,swizzle_draws,swizzle_missing,swizzle_diffuse_draws,swizzle_environment_draws,swizzle_masked_draws;
#include "platform_pica_texture_window.h"
/* CPU-only compiled command plans share the bounded linear pool with their
 * packet templates. They are not direct GPU inputs. Keep the existing total
 * material metadata cap, charge their complete extent to linear RAM, and leave
 * 15% free. Required morph/source metadata retains normal-heap headroom. */
static void *allocate_command_plan(unsigned bytes){
    extern u32 __ctru_linear_heap_size;
    unsigned available=linearSpaceFree();
    if(!bytes || bytes>MATERIAL_BYTES-cpu_bytes || bytes>available ||
       (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15)return NULL;
    void *p=linearAlloc(bytes);if(!p)return NULL;
    if((u64)linearSpaceFree()*100<(u64)__ctru_linear_heap_size*15){linearFree(p);return NULL;}
    memset(p,0,bytes);cpu_bytes+=bytes;
    if(cpu_bytes>cpu_peak)cpu_peak=cpu_bytes;
    return p;
}
static void deallocate_command_plan(void *p,unsigned bytes){
    if(p){linearFree(p);cpu_bytes-=bytes;}
}
/* Material/animation sampler descriptors use the same bounded owner as
 * compiled commands. Their pointers are consumed by CPU code; texture storage,
 * GPU publication and resource retirement are unchanged. This releases normal
 * heap room for required game/animation allocations on Old 3DS. */
static void *allocate(unsigned bytes){return allocate_command_plan(bytes);}
static void deallocate(void *p,unsigned bytes){deallocate_command_plan(p,bytes);}

static unsigned acquire_image(const FhTexture *source){
    if(!source || !source->pixels)return 0;
    u64 start=svcGetSystemTick();FhPackedTexture prepared={0};C3D_Tex candidate={0};void *staging=NULL;
    unsigned found=fh_texture_pack_find_typed(&pack,source->pixels,
        fh_texture_source_bytes(source->format,source->width,source->height),
        source->width,source->height,source->format,!fh_graphics_textures_active,&prepared);
    if(found)for(unsigned i=0;i<IMAGES;i++)if(images[i].refs && images[i].offset==prepared.offset){images[i].refs++;load_ticks+=svcGetSystemTick()-start;return i+1;}
    unsigned slot=0;while(slot<IMAGES && images[slot].refs)slot++;
    unsigned w=found?prepared.width:fh_texture_extent(source->width),h=found?prepared.height:fh_texture_extent(source->height);
    unsigned cmpr16=!found && source->format==14;
    unsigned format=found?prepared.format:cmpr16?2:fh_texture_native_format(source);
    unsigned bytes=found?prepared.bytes:cmpr16?w*h*2:fh_texture_native_bytes(source,w,h);
    extern u32 __ctru_linear_heap_size;
    /* Opening flight's live set is 2.83 MiB with HUD and characters. The old
     * 2 MiB diagnostic cap rejected visible images despite >5 MiB free linear
     * RAM. Both models use a 4 MiB ceiling; every allocation still preserves
     * 15% of that model's actual linear grant for other resident resources. */
    unsigned limit=4u*1024u*1024u,free_bytes=linearSpaceFree();
    if(!bytes)goto failed;
    /* Unknown prepared entries are converted once at LOAD. Compare canonical
     * GPU bytes before allocating another resident copy. This is exact (no
     * hash-only identity or borrowed source pointer); never executed by draws. */
    if(!found){
        /* Unpublished CPU-writable PICA storage: no duplicate decoded heap copy.
         * Keep the actual linear reserve, including allocator rounding. */
        if(bytes>limit || bytes>free_bytes ||
           (u64)(free_bytes-bytes)*100<(u64)__ctru_linear_heap_size*15)goto failed;
        if(!C3D_TexInit(&candidate,w,h,(GPU_TEXCOLOR)format))goto failed;
        if((u64)linearSpaceFree()*100<(u64)__ctru_linear_heap_size*15)goto failed;
        staging=candidate.data;
        unsigned source_bytes=fh_texture_source_bytes(source->format,source->width,source->height);
        if(!(cmpr16?fh_texture_convert_cmpr16(staging,bytes,w,h,source,source_bytes):
            fh_texture_convert(staging,bytes,w,h,source,source_bytes)))goto failed;
        conversions++;
        for(unsigned i=0;i<IMAGES;i++){
            Image *im=images+i;
            if(im->refs && im->gpu.width==w && im->gpu.height==h && im->gpu.fmt==format && im->bytes==bytes &&
                !memcmp(im->gpu.data,staging,bytes)){
                im->refs++;C3D_TexDelete(&candidate);load_ticks+=svcGetSystemTick()-start;return i+1;
            }
        }
    }
    if(slot==IMAGES || bytes>limit-gpu_bytes || bytes>free_bytes ||
        (u64)(free_bytes-bytes)*100<(u64)__ctru_linear_heap_size*15)goto failed;
    Image *im=images+slot;
    if(candidate.data){im->gpu=candidate;memset(&candidate,0,sizeof(candidate));}
    else{
        FhPackedMips mips={0};
        unsigned wants=found && source->min_filter>=2 && source->max_lod>source->min_lod &&
            fh_texture_pack_mips(&pack,&prepared,source->width,source->height,source->format,&mips);
        if(wants && ((u64)gpu_bytes+bytes+mips.bytes+MIP_MARGIN>limit || (u64)bytes+mips.bytes+MIP_MARGIN>free_bytes ||
           (u64)(free_bytes-bytes-mips.bytes-MIP_MARGIN)*100<(u64)__ctru_linear_heap_size*15)){wants=0;mip_skipped++;}
        if(wants && C3D_TexInitWithParams(&im->gpu,NULL,(C3D_TexInitParams){w,h,(u8)mips.levels,(GPU_TEXCOLOR)format,GPU_TEX_2D,false})){
            if(!fh_texture_pack_read_mips(&pack,&mips,(unsigned char*)im->gpu.data+bytes,mips.bytes)){
                C3D_TexDelete(&im->gpu);memset(&im->gpu,0,sizeof(im->gpu));mip_failures++;
                if(!C3D_TexInit(&im->gpu,w,h,(GPU_TEXCOLOR)format))goto failed;
            }else{im->mip_bytes=mips.bytes;mip_images++;}
        }else if(!C3D_TexInit(&im->gpu,w,h,(GPU_TEXCOLOR)format))goto failed;
    }
    if(found && !fh_texture_pack_read(&pack,&prepared,im->gpu.data,bytes)){
        C3D_TexDelete(&im->gpu);memset(im,0,sizeof(*im));goto failed;
    }
    /* Decoded bytes already occupy the final allocation; publish after flush. */
    fh_gpu_cache_flush(im->gpu.data,bytes+im->mip_bytes);bytes+=im->mip_bytes;
    im->refs=1;im->offset=found?prepared.offset:0;im->bytes=bytes;im->source_width=source->width;im->source_height=source->height;
    gpu_bytes+=bytes;if(gpu_bytes>gpu_peak)gpu_peak=gpu_bytes;loads++;
    load_ticks+=svcGetSystemTick()-start;return slot+1;
failed:
    if(candidate.data)C3D_TexDelete(&candidate);
    {char line[256];snprintf(line,sizeof(line),"PICA image load failure: source=%ux%u fmt=%u native=%ux%u fmt=%u bytes=%u resident=%u free=%u prepared=%u slot=%u\n",(unsigned)source->width,(unsigned)source->height,(unsigned)source->format,w,h,format,bytes,gpu_bytes,free_bytes,found,slot);fh_platform_log(line);}
    errors++;load_ticks+=svcGetSystemTick()-start;return 0;
}
#include "platform_pica_glow_mask_images.h"
static void release_image(unsigned handle){
    if(!handle)return;
    if(handle>IMAGES || !images[handle-1].refs)fh_platform_panic("PICA material image lifetime");
    Image *im=images+handle-1;
    if(--im->refs)return;
    glow_mask_image_retire(handle);
    gpu_bytes-=im->bytes;C3D_TexDelete(&im->gpu);memset(im,0,sizeof(*im));
}
unsigned fh_pica_image_acquire(const FhTexture *source){return acquire_image(source);}
void fh_pica_image_release(unsigned handle){release_image(handle);}
void fh_pica_image_sampler(unsigned handle,const FhTexture *source,C3D_Tex *sampler,float scale[2]){
    if(!handle || handle>IMAGES || !images[handle-1].refs)fh_platform_panic("PICA image sampler lifetime");
    Image *im=images+handle-1;*sampler=im->gpu;
    C3D_TexSetWrap(sampler,source->wrap_s==1?GPU_REPEAT:source->wrap_s==2?GPU_MIRRORED_REPEAT:GPU_CLAMP_TO_EDGE,
        source->wrap_t==1?GPU_REPEAT:source->wrap_t==2?GPU_MIRRORED_REPEAT:GPU_CLAMP_TO_EDGE);
    fh_pica_sampler_filter(sampler,source->min_filter,source->mag_filter);
    if(sampler->maxLevel)C3D_TexSetLodBias(sampler,source->lod_bias<-1.f?-1.f:source->lod_bias);
    /* Exact GPU-byte dedup can share differently padded source rectangles.
     * UV extents belong to this caller, not the first image-pool owner. */
    scale[0]=(float)source->width/(im->gpu.width<source->width?fh_texture_extent(source->width):im->gpu.width);
    scale[1]=(float)source->height/(im->gpu.height<source->height?fh_texture_extent(source->height):im->gpu.height);
}
static FhPicaMaterialSet *plan_for(const FhSceneAsset *asset){return fh_native_scene_pica_materials(asset);}
static Material *material_for(const FhSceneAsset *asset,unsigned index){
    FhPicaMaterialSet *p=plan_for(asset);return p && p->items && index<p->count?(Material*)p->items+index:NULL;
}
void fh_pica_materials_invalidate(void){bound_program=0;}
static unsigned acquire_program(const C3D_TexEnv stages[4],unsigned buffer_mask){
    unsigned free_slot=PROGRAMS;
    for(unsigned i=0;i<PROGRAMS;i++){
        Program *p=programs+i;
        if(!p->refs){if(free_slot==PROGRAMS)free_slot=i;continue;}
        if(p->buffer_mask==buffer_mask && !memcmp(p->stages,stages,sizeof(p->stages))){p->refs++;return i+1;}
    }
    if(free_slot==PROGRAMS)return 0;
    Program *p=programs+free_slot;memcpy(p->stages,stages,sizeof(p->stages));p->refs=1;p->buffer_mask=buffer_mask;p->actor_height=0;
    fh_pica_actor_material_compile(&p->commands,stages,buffer_mask);
    program_count++;if(program_count>program_peak)program_peak=program_count;
    return free_slot+1;
}
static int compile_program(Material *m){
    if(m->program)return 0;
    m->program=acquire_program(m->environment?m->lit_env:m->env,m->environment?1:0);
    return m->program!=0;
}
int fh_pica_material_swizzle_compile(const FhSceneAsset *asset,unsigned index,unsigned source,unsigned plain){
    Material *m=material_for(asset,index);if(!m || !m->ready || m->swizzle_program || m->swizzle_environment[0])return 0;
    m->swizzle_source=source;
    if(!source)return 1;
    C3D_TexEnv stages[4];
    if(!plain || !m->actor || m->layers!=1+m->masked || m->layered || 
       !fh_pica_texture_swizzle_opacity_compile(stages,m->env)){swizzle_partial++;return 1;}
    if(m->environment){
        for(unsigned channel=0;channel<3;channel++){
            if(!fh_pica_texture_swizzle_environment_compile(stages,m->env,m->lit_env,channel)){swizzle_partial++;return 1;}
            m->swizzle_environment[channel]=acquire_program(stages,1);
            if(!m->swizzle_environment[channel])return 0;
            if(m->diffuse_program){
                C3D_TexEnv mapped[4];fh_pica_texture_swizzle_diffuse_compile(mapped,stages,1);
                m->swizzle_diffuse[channel+1]=acquire_program(mapped,1);
                if(!m->swizzle_diffuse[channel+1])return 0;
            }
        }
    }else{
        m->swizzle_program=acquire_program(stages,0);
        if(!m->swizzle_program)return 0;
        if(m->diffuse_program){
            C3D_TexEnv mapped[4];fh_pica_texture_swizzle_diffuse_compile(mapped,stages,0);
            m->swizzle_diffuse[0]=acquire_program(mapped,0);
            if(!m->swizzle_diffuse[0])return 0;
        }
    }
    swizzle_admitted++;return 1;
}
void fh_pica_material_swizzle_live(const FhSceneAsset *asset,unsigned index,unsigned selector){
    Material *m=material_for(asset,index);if(!m || !m->ready)return;
    if(selector>=24)fh_platform_panic("PICA texture channel selector");
    m->swizzle=m->swizzle_source?selector:0;
}
static void release_program(unsigned handle){
    if(!handle || handle>PROGRAMS || !programs[handle-1].refs)fh_platform_panic("PICA material program release");
    if(!--programs[handle-1].refs){
        unsigned child=programs[handle-1].actor_height;
        programs[handle-1].actor_height=0;program_count--;
        if(child)release_program(child);
    }
    if(bound_program==handle)bound_program=0;
}
#include "platform_pica_specular_material.h"
static void ui_programs_init(void){
    for(unsigned k=0;k<16;k++){
        C3D_TexEnv e[4];for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,k&1?GPU_CONSTANT:GPU_PRIMARY_COLOR,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_CONSTANT,0);
        C3D_TexEnvOpAlpha(e,k&2?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        if(k&4){ /* Swizzled opacity replaces, rather than multiplies, base alpha. */
            C3D_TexEnvSrc(e+1,C3D_Alpha,GPU_TEXTURE1,GPU_CONSTANT,0);
            C3D_TexEnvOpAlpha(e+1,k&8?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
            C3D_TexEnvFunc(e+1,C3D_Alpha,GPU_MODULATE);
        }
        ui_program[k]=acquire_program(e,0);
        if(!ui_program[k])fh_platform_panic("PICA UI program capacity");
    }
}
static void moon_programs_init(void){
    for(unsigned intensity=0;intensity<2;intensity++){
        C3D_TexEnv e[4];for(unsigned i=0;i<4;i++)C3D_TexEnvInit(e+i);
        /* RGB temporarily holds the phase mask; alpha retains the base
         * opacity, avoiding a readback or a TEV buffer dependency. */
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_CONSTANT,0);
        C3D_TexEnvOpAlpha(e,intensity?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        C3D_TexEnvSrc(e+1,C3D_RGB,GPU_TEXTURE1,GPU_CONSTANT,0);
        C3D_TexEnvOpRgb(e+1,intensity?GPU_TEVOP_RGB_SRC_COLOR:GPU_TEVOP_RGB_SRC_ALPHA,GPU_TEVOP_RGB_SRC_ALPHA,0);
        C3D_TexEnvFunc(e+1,C3D_RGB,GPU_MODULATE);C3D_TexEnvScale(e+1,C3D_RGB,GPU_TEVSCALE_4);
        C3D_TexEnvSrc(e+2,C3D_RGB,GPU_TEXTURE0,0,0);C3D_TexEnvFunc(e+2,C3D_RGB,GPU_REPLACE);
        C3D_TexEnvSrc(e+2,C3D_Alpha,GPU_PREVIOUS,GPU_PREVIOUS,0);
        C3D_TexEnvOpAlpha(e+2,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_ONE_MINUS_SRC_R,0);
        C3D_TexEnvFunc(e+2,C3D_Alpha,GPU_MODULATE);
        moon_program[intensity]=acquire_program(e,0);
        if(!moon_program[intensity])fh_platform_panic("PICA moon program capacity");
    }
}
int fh_pica_materials_compile(const FhSceneAsset *asset){
    FhPicaMaterialSet *p=plan_for(asset);if(!p || !p->items)return 0;
    for(unsigned i=0;i<p->count;i++){
        Material *m=(Material*)p->items+i;
        if(m->ready && !compile_program(m))return 0;
    }
    return world_stream_prepare(asset,p) && actor_stream_prepare(asset,p);
}
int fh_pica_materials_begin(const FhSceneAsset *asset,unsigned count){
    FhPicaMaterialSet *p=plan_for(asset);if(!p || !count || count>64 || p->items)return 0;
    p->items=allocate(count*sizeof(Material));if(!p->items)return 0;p->count=count;return 1;
}
int fh_pica_material_begin(const FhSceneAsset *asset,unsigned index,unsigned layers,unsigned n0,unsigned n1,unsigned flags){
    Material *m=material_for(asset,index);if(!m || layers>2 || n0>128 || n1>128 || m->ready)return 0;
    m->layers=layers;m->flags=flags;
    unsigned counts[2]={n0,n1};
    for(unsigned i=0;i<layers;i++){if(!counts[i])return 0;m->count[i]=counts[i];m->frames[i]=allocate(counts[i]*sizeof(Frame));if(!m->frames[i])return 0;}
    for(unsigned i=0;i<4;i++)m->tint[i]=1;
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(m->env+i);
    C3D_TexEnvSrc(m->env,C3D_Both,GPU_PRIMARY_COLOR,0,0);
    m->ready=1;return 1;
}
static GPU_TEXTURE_WRAP_PARAM wrap(unsigned value){return value==1?GPU_REPEAT:value==2?GPU_MIRRORED_REPEAT:GPU_CLAMP_TO_EDGE;}
/* Fur's original indirect alpha becomes a resident third sampler. Geometry,
 * source image bytes and animation remain unchanged; acquisition is load-only. */
int fh_pica_material_fur_mask(const FhSceneAsset *asset,unsigned index,const FhTexture *source){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || m->fur_mask || !source || !source->pixels)return 0;
    Frame *f=allocate(sizeof(*f));if(!f)return 0;
    f->handle=acquire_image(source);
    if(!f->handle){deallocate(f,sizeof(*f));return 0;}
    fh_pica_image_sampler(f->handle,source,&f->sampler,f->scale);
    m->fur_mask=f;m->fur_mask_intensity=source->format<=1;
    fh_native_scene_fur_groom_prepare(asset,index,source);return 1;
}
int fh_pica_material_frame(const FhSceneAsset *asset,unsigned index,unsigned layer,unsigned frame,const FhTexture *source){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || layer>=m->layers || frame>=m->count[layer] || m->frames[layer][frame].handle)return 0;
    if(!source || !source->pixels)return 0;
    if(frame && m->intensity[layer]!=(source->format<=1))return 0;
    unsigned handle=acquire_image(source);if(!handle)return 0;
    Image *im=images+handle-1;Frame *f=m->frames[layer]+frame;
    f->handle=handle;f->sampler=im->gpu;
    C3D_TexSetWrap(&f->sampler,wrap(source->wrap_s),wrap(source->wrap_t));
    fh_pica_sampler_filter(&f->sampler,source->min_filter,source->mag_filter);
    if(f->sampler.maxLevel)C3D_TexSetLodBias(&f->sampler,source->lod_bias<-1.f?-1.f:source->lod_bias);
    f->scale[0]=(float)source->width/(im->gpu.width<source->width?fh_texture_extent(source->width):im->gpu.width);
    f->scale[1]=(float)source->height/(im->gpu.height<source->height?fh_texture_extent(source->height):im->gpu.height);
    m->intensity[layer]=source->format<=1;
    /* Compile the native combiner once. Runtime binds these register images. */
    C3D_TexEnv *e=m->env;
    if(m->layers==1){
        C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,GPU_TEXTURE0,0);C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
        if(m->intensity[0])C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    }else{
        C3D_TexEnvSrc(e,C3D_RGB,GPU_TEXTURE0,GPU_CONSTANT,0);C3D_TexEnvSrc(e,C3D_Alpha,GPU_TEXTURE0,GPU_PRIMARY_COLOR,0);C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
        if(m->intensity[0])C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_R,GPU_TEVOP_A_SRC_ALPHA,0);
        e=m->env+1;C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_TEXTURE1,GPU_PREVIOUS);C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);C3D_TexEnvFunc(e,C3D_RGB,GPU_INTERPOLATE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE1,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        if(m->intensity[1])C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
        e=m->env+2;C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_PRIMARY_COLOR,0);C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_CONSTANT,0);C3D_TexEnvFunc(e,C3D_Both,GPU_MODULATE);
    }
    return 1;
}
/* Actor base+opacity program is compiled once, after image frames. Unlike
 * world type-9 layering, the second image only modulates alpha. */
int fh_pica_material_actor(const FhSceneAsset *asset,unsigned index,unsigned masked){
    Material *m=material_for(asset,index);if(!m || !m->ready || (masked && m->layers!=2))return 0;
    m->actor=1;m->masked=masked;
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(m->env+i);
    C3D_TexEnv *e=m->env;
    C3D_TexEnvSrc(e,C3D_Both,GPU_PRIMARY_COLOR,m->layers?GPU_TEXTURE0:GPU_PRIMARY_COLOR,0);
    C3D_TexEnvFunc(e,C3D_Both,m->layers?GPU_MODULATE:GPU_REPLACE);
    if(m->layers && m->intensity[0])C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    if(masked){
        e=m->env+1;C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,0,0);C3D_TexEnvFunc(e,C3D_RGB,GPU_REPLACE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE1,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,m->intensity[1]?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
    }
    return 1;
}
/* Swizzled/KAlpha source layers use object/material alpha, not vertex alpha.
 * A separate opacity plane REPLACES base image alpha in that source family. */
int fh_pica_material_actor_alpha_source(const FhSceneAsset *asset,unsigned index,unsigned constant){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || !m->actor || m->program)return 0;
    m->alpha_constant=!!constant;
    if(constant){
        alpha_constant_materials++;alpha_replaced_masks+=m->masked!=0;
        C3D_TexEnv *e=m->env;
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,GPU_TEXTURE0,0);
        C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,m->intensity[0]?GPU_TEVOP_A_SRC_R:GPU_TEVOP_A_SRC_ALPHA,0);
        C3D_TexEnvFunc(e,C3D_Alpha,m->layers && !m->masked?GPU_MODULATE:GPU_REPLACE);
    }
    return 1;
}
int fh_pica_material_actor_raster(const FhSceneAsset *asset,unsigned index,unsigned model_flags){
    Material *m=material_for(asset,index);if(!m || !m->ready || !m->actor)return 0;
    m->actor_raster_plan=fh_pica_actor_raster_plan(model_flags,m->flags);
    m->actor_fog=!(model_flags&0x100);
    m->actor_raster=fh_pica_actor_raster_live(m->actor_raster_plan,255,0);
    return 1;
}
void fh_pica_material_actor_alpha(const FhSceneAsset *asset,unsigned index,unsigned alpha,unsigned threshold){
    Material *m=material_for(asset,index);if(!m || !m->ready || !m->actor)return;
    m->actor_raster=fh_pica_actor_raster_live(m->actor_raster_plan,alpha,threshold);
}
int fh_pica_material_actor_solid(const FhSceneAsset *asset,unsigned index,unsigned lit){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || !m->actor || m->layers || m->masked || m->program)return 0;
    C3D_TexEnv *e=m->env;
    C3D_TexEnvSrc(e,C3D_RGB,lit?GPU_PRIMARY_COLOR:GPU_CONSTANT,GPU_CONSTANT,0);
    C3D_TexEnvFunc(e,C3D_RGB,lit?GPU_MODULATE:GPU_REPLACE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_CONSTANT,0,0);
    C3D_TexEnvFunc(e,C3D_Alpha,GPU_REPLACE);
    return 1;
}
/* Compile the original addTexLayerStage color operation once. Layer alpha
 * multiplies accumulated alpha; interpolate uses PREVIOUS alpha, not the
 * incoming texture's alpha. No extra draw pass or geometry copy is needed. */
int fh_pica_material_actor_layered(const FhSceneAsset *asset,unsigned index,unsigned mode,unsigned light_after){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || m->layers!=2 || m->masked || !m->actor ||
       (mode!=0 && mode!=1 && mode!=4 && mode!=6))return 0;
    m->layered=1 | (!!light_after<<1) | (mode<<2);
    C3D_TexEnv *e=m->env+1;
    C3D_TexEnvSrc(e,C3D_RGB,mode==0?GPU_PRIMARY_COLOR:GPU_PREVIOUS,GPU_TEXTURE1,GPU_PREVIOUS);
    C3D_TexEnvOpRgb(e,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_COLOR,GPU_TEVOP_RGB_SRC_ALPHA);
    C3D_TexEnvFunc(e,C3D_RGB,mode==4?GPU_ADD:(mode==0 || mode==6)?GPU_MODULATE:GPU_INTERPOLATE);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,GPU_TEXTURE1,0);C3D_TexEnvFunc(e,C3D_Alpha,GPU_MODULATE);
    if(m->intensity[1])C3D_TexEnvOpAlpha(e,GPU_TEVOP_A_SRC_ALPHA,GPU_TEVOP_A_SRC_R,0);
    return 1;
}
unsigned fh_pica_material_actor_has_layer(const FhSceneAsset *asset,unsigned index){
    Material *m=material_for(asset,index);return m && m->layered;
}
/* Source white-selector layers bypass raster RGB without diffuse/specular
 * maps. Replace only RGB operands; opacity and live object alpha stay intact.
 * Descriptors are compiled once. No new program state or per-draw branch. */
int fh_pica_material_actor_unlit(const FhSceneAsset *asset,unsigned index,unsigned enabled){
    Material *m=material_for(asset,index);
    if(!m || !m->ready || !m->actor || m->program)return 0;
    if(enabled && m->layers)for(unsigned i=0;i<4;i++)for(unsigned n=0;n<3;n++){
        C3D_TexEnv *e=m->env+i;
        if(((e->srcRgb>>(4*n))&15)==GPU_PRIMARY_COLOR)
            e->srcRgb=(e->srcRgb&~(15u<<(4*n)))|(GPU_CONSTANT<<(4*n));
    }
    return 1;
}
#include "platform_pica_diffuse_material.h"
int fh_pica_material_environment_compile(const FhSceneAsset *asset,unsigned index,unsigned enabled){
    Material *m=material_for(asset,index);if(!m || !m->ready || !m->actor)return 0;
    m->environment=enabled && FH_PICA_ACTOR_LIGHTING_EXPERIMENT;
    for(unsigned i=0;i<4;i++)C3D_TexEnvInit(m->lit_env+i);
    C3D_TexEnv *e=m->lit_env;
    C3D_TexEnvSrc(e,C3D_RGB,GPU_FRAGMENT_PRIMARY_COLOR,GPU_CONSTANT,GPU_PRIMARY_COLOR);
    C3D_TexEnvFunc(e,C3D_RGB,GPU_MULTIPLY_ADD);
    C3D_TexEnvSrc(e,C3D_Alpha,GPU_PRIMARY_COLOR,0,0);
    for(unsigned i=0;i<3;i++){
        e=m->lit_env+i+1;*e=m->env[i];
        for(unsigned n=0;n<3;n++)if(((e->srcRgb>>(4*n))&15)==GPU_PRIMARY_COLOR)
            e->srcRgb=(e->srcRgb&~(15u<<(4*n)))|((i?GPU_PREVIOUS_BUFFER:GPU_PREVIOUS)<<(4*n));
    }
    if(m->layered&2){
        /* Source addShaderLayerStages: with two pre-light layers, compose
         * the unlit textures first, then addLitColorStage applies C1 to the
         * entire result. Lighting only the base leaves eyes self-lit.
         * Stage0 already retains C1 in the existing RGB buffer. */
        for(unsigned i=0;i<2;i++){
            e=m->lit_env+i+1;*e=m->env[i];
            for(unsigned n=0;n<3;n++)if(((e->srcRgb>>(4*n))&15)==GPU_PRIMARY_COLOR)
                e->srcRgb=(e->srcRgb&~(15u<<(4*n)))|(GPU_CONSTANT<<(4*n));
        }
        e=m->lit_env+3;C3D_TexEnvInit(e);
        C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_PREVIOUS_BUFFER,0);
        C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
        C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,0,0);
        if(!m->environment){
            /* addLitColorStage(0) uses raster light instead of C1. Keep
             * both texture layers unlit until they have been composed.
             * Reuse the compiled texture stages; alpha stays unchanged.
             * Layered C2/specular remains a separately counted omission. */
            m->env[0]=m->lit_env[1];m->env[1]=m->lit_env[2];
            e=m->env+2;C3D_TexEnvInit(e);
            C3D_TexEnvSrc(e,C3D_RGB,GPU_PREVIOUS,GPU_PRIMARY_COLOR,0);
            C3D_TexEnvFunc(e,C3D_RGB,GPU_MODULATE);
            C3D_TexEnvSrc(e,C3D_Alpha,GPU_PREVIOUS,0,0);
        }
    }
    return diffuse_material_compile(m) && compile_program(m);
}
void fh_pica_material_environment(const FhSceneAsset *asset,unsigned index,unsigned enabled,unsigned slot,unsigned strength){
    Material *m=material_for(asset,index);if(!m || !m->ready || !m->actor)return;
    if(FH_PICA_ACTOR_LIGHTING_EXPERIMENT && m->environment!=!!enabled)fh_platform_panic("PICA environment material identity changed");
    m->environment_slot=slot;m->environment_strength=strength;
}
void fh_pica_material_live(const FhSceneAsset *asset,unsigned index,const float color[4],const float scroll[2][2],const unsigned frames[2],unsigned konst,unsigned flags){
    Material *m=material_for(asset,index);if(!m || !m->ready)return;
    m->ui.kind=0;m->post_state=FH_PICA_POST_NONE;m->movie_pixels=NULL;m->movie_active=0;m->cloud_state=0;m->moon_alpha=0;
    memcpy(m->tint,color,sizeof(m->tint));memcpy(m->scroll,scroll,sizeof(m->scroll));m->konst=konst;m->flags=flags;
    for(unsigned i=0;i<m->layers;i++)m->current[i]=(frames[i]>>8)<m->count[i]?(frames[i]>>8):m->count[i]-1;
}
void fh_pica_material_ui(const FhSceneAsset *asset,unsigned index,const FhPicaUiMaterial *state){
    Material *m=material_for(asset,index);if(!m || !m->ready)return; /* Unused source render op. */
    if(state->kind<1 || state->kind>4 || !m->actor)fh_platform_panic("PICA UI semantic material");
    m->ui=*state;
}
void fh_pica_material_post(const FhSceneAsset *asset,unsigned index,unsigned state){
    Material *m=material_for(asset,index);if(!m || !m->ready)return;
    if(state>FH_PICA_POST_OPAQUE || !m->actor)fh_platform_panic("PICA post material state");
    m->post_state=state;
}
void fh_pica_material_movie(const FhSceneAsset *asset,unsigned index,const void *pixels){
    Material *m=material_for(asset,index);if(!m || !m->ready)return;
    m->movie_pixels=pixels;m->movie_active=1;
}
void fh_pica_material_moon(const FhSceneAsset *asset,unsigned index,float phase,unsigned alpha){
    Material *m=material_for(asset,index);if(!m || !m->ready)return;
    if(!m->actor || !m->layers || alpha>255)fh_platform_panic("PICA moon material contract");
    m->moon_phase=phase;m->moon_alpha=alpha+1;
}
static int bind_moon(Material *m,unsigned format,unsigned compact){
    Frame *f=m->frames[0]+m->current[0];unsigned handle=moon_program[m->intensity[0]!=0];
    if(!f->handle || !images[f->handle-1].refs || !handle)fh_platform_panic("PICA moon image/program lifetime");
    unsigned changed=bound_program!=handle;
    if(changed){C3D_TexEnvBufUpdate(C3D_Both,0);program_binds++;}else program_reuses++;
    for(unsigned i=0;i<4;i++){
        unsigned color=(i==0?m->moon_alpha-1:i==1?62:0)<<24;
        if(changed)C3D_SetTexEnv(i,programs[handle-1].stages+i);
        if(changed || bound_colors[i]!=color)C3D_TexEnvColor(C3D_GetTexEnv(i),color);
        bound_colors[i]=color;
    }
    bound_program=handle;fh_pica_scene_actor_second_uv(0);fh_pica_environment_bind(0,0);
    C3D_TexBind(0,&f->sampler);C3D_TexBind(1,&f->sampler);fh_pica_texture2_coord1(0);C3D_TexBind(2,NULL);
    float us=compact?(format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,1,1,1,1);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,8,f->scale[0]*us,f->scale[1]*us,0,0);
    C3D_FVUnifSet(GPU_VERTEX_SHADER,9,f->scale[0]*us,f->scale[1]*us,m->moon_phase*f->scale[0],0);
    C3D_CullFace(GPU_CULL_NONE);C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_SRC_ALPHA,GPU_ONE_MINUS_SRC_ALPHA,GPU_ONE,GPU_ZERO);
    moon_draws++;bindings++;return 1;
}
void fh_pica_cloud_material(const FhSceneAsset *asset,unsigned index,unsigned flags,unsigned threshold,unsigned alpha,unsigned shadow){
    Material *m=material_for(asset,index);if(!m || !m->ready)return;
    if(fh_pica_cloud_mode<1 || fh_pica_cloud_mode>2)fh_platform_panic("PICA cloud material scope");
    m->cloud_state=fh_pica_cloud_raster(fh_pica_cloud_mode,flags,m->flags,threshold,alpha,shadow);
}
extern const C3D_Tex *fh_cmenu_capture_sampler;
static int bind_ui(Material *m,unsigned format,unsigned compact){
    const FhPicaUiMaterial *u=&m->ui;
    if(!(u->rgba>>24)){ui_hidden[u->kind]++;return 2;}
    unsigned constant=u->kind<=FH_PICA_UI_ICON,base_intensity=0,mask_intensity=0;
    float scale[2][2]={{1,1},{1,1}};const C3D_Tex *base=NULL,*mask=NULL;
    if(u->kind==FH_PICA_UI_ICON){
        base=fh_pica_hud_sampler_layer(u->page,0,scale[0],&base_intensity);
        mask=fh_pica_hud_sampler_layer(u->page,1,scale[1],&mask_intensity);
    }else{
        if(!m->layers || !m->count[0])fh_platform_panic("PICA UI base image not prepared");
        Frame *f=m->frames[0];base=&f->sampler;memcpy(scale[0],f->scale,sizeof(f->scale));base_intensity=m->intensity[0];
        if(constant && m->masked){f=m->frames[1];mask=&f->sampler;memcpy(scale[1],f->scale,sizeof(f->scale));mask_intensity=m->intensity[1];}
    }
    fh_cmenu_capture_sampler=base;
    unsigned key=constant|(base_intensity<<1)|((mask!=NULL)<<2)|(mask_intensity<<3);
    unsigned handle=ui_program[key];if(!handle)fh_platform_panic("PICA UI uncompiled program");
    if(bound_program!=handle){
        C3D_TexEnvBufUpdate(C3D_Both,0);
        for(unsigned i=0;i<4;i++){C3D_SetTexEnv(i,programs[handle-1].stages+i);C3D_TexEnvColor(C3D_GetTexEnv(i),u->rgba);bound_colors[i]=u->rgba;}
        bound_program=handle;program_binds++;
    }else{
        for(unsigned i=0;i<4;i++)if(bound_colors[i]!=u->rgba){C3D_TexEnvColor(C3D_GetTexEnv(i),u->rgba);bound_colors[i]=u->rgba;}
        program_reuses++;
    }
    fh_pica_scene_actor_second_uv(0);fh_pica_environment_bind(0,0);
    C3D_TexBind(0,(C3D_Tex*)base);C3D_TexBind(1,(C3D_Tex*)mask);fh_pica_texture2_coord1(0);C3D_TexBind(2,NULL);
    float us=compact?(format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,1,1,1,1);
    for(unsigned i=0;i<2;i++)C3D_FVUnifSet(GPU_VERTEX_SHADER,8+i,scale[i][0]*us,scale[i][1]*us,0,0);
    /* Source C-menu callbacks explicitly select ALWAYS with depth disabled.
     * Reticles retain reverse-depth GEQUAL. Publish the complete source state,
     * including the function routed by the SDK to the fragment depth block. */
    C3D_DepthTest(u->depth_test,u->depth_test?GPU_GEQUAL:GPU_ALWAYS,u->depth_write?GPU_WRITE_ALL:GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);
    if(u->kind>=FH_PICA_UI_ACTION)C3D_CullFace(GPU_CULL_FRONT_CCW);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,u->blend?GPU_SRC_ALPHA:GPU_ONE,u->blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    ui_draws[u->kind]++;bindings++;return 1;
}
void fh_pica_hologram_prepare(const FhSceneAsset *asset){
    if(!asset)return;
    FhPicaMaterialSet *set=fh_native_scene_pica_materials(asset);
    if(!set || !set->items)return;
    for(unsigned i=0;i<set->count;i++){
        Material *m=(Material*)set->items+i;
        if(!m->ready || m->projected_samplers)continue;
        if(!m->layers || !m->count[0])fh_platform_panic("PICA hologram base image");
        m->projected_samplers=allocate(m->count[0]*sizeof(C3D_Tex));
        if(!m->projected_samplers)fh_platform_panic("PICA hologram sampler budget");
        for(unsigned k=0;k<m->count[0];k++){
            C3D_Tex *t=m->projected_samplers+k;*t=m->frames[0][k].sampler;
            t->param=(t->param&~(7u<<28))|(GPU_TEX_PROJECTION<<28);
        }
    }
}
const C3D_Tex *fh_pica_material_projected_sampler(const FhPicaMaterialSet *set,unsigned index,float scale[2]){
    if(!set || !set->items || index>=set->count)fh_platform_panic("PICA hologram material index");
    Material *m=(Material*)set->items+index;
    if(!m->projected_samplers || m->current[0]>=m->count[0])fh_platform_panic("PICA hologram sampler not prepared");
    Frame *f=m->frames[0]+m->current[0];
    if(!f->handle || !images[f->handle-1].refs)fh_platform_panic("PICA hologram image lifetime");
    memcpy(scale,f->scale,sizeof(f->scale));return m->projected_samplers+m->current[0];
}
static C3D_Tex neutral_texture;
C3D_Tex *fh_pica_options_texture(const C3D_Tex *texture,unsigned disabled_bit){
    return texture && (fh_graphics_options&disabled_bit)?&neutral_texture:(C3D_Tex*)texture;
}
unsigned fh_pica_material_cloud_selected(const FhPicaMaterialSet *set,unsigned index){
    if(!set || !set->items || index>=set->count)return 0;
    const Material *m=(const Material*)set->items+index;
    return m->ready && m->actor && m->cloud_state && !m->ui.kind && !m->environment && !m->specular_program && !m->diffuse_program;
}
void fh_pica_material_specular_require(const FhPicaMaterialSet *set,unsigned compact){
    if(!set || !set->items || !set->count)return;
    unsigned mask=((const Material*)set->items)->specular_demand;
    if(compact && (mask>>3))fh_pica_specular_require_half((mask>>3)&7);
}
/* PICA_AMBIENT_V1: exclude active normal-dependent and custom-coordinate owners.
 * These are prepared flags; no source decode, material scan or allocation. */
unsigned fh_pica_material_ambient_selected(const FhPicaMaterialSet *set,unsigned index){
    if(!set || !set->items || index>=set->count)return 0;
    const Material *m=(const Material*)set->items+index;
    if(!m->ready || !m->actor || m->ui.kind || m->moon_alpha || m->movie_active || m->cloud_state)return 0;
    if(m->environment && !(fh_graphics_options&FH_GRAPHICS_NO_ENVIRONMENT))return 0;
    if(m->specular_program && (fh_graphics_options&FH_GRAPHICS_SPECULAR) &&
       !(fh_graphics_options&FH_GRAPHICS_NO_ACTOR_TEX))return 0;
    return 1;
}
unsigned fh_pica_material_ui_selected(const FhPicaMaterialSet *set,unsigned index){
    if(!set || !set->items || index>=set->count)return 0;
    const Material *m=(const Material*)set->items+index;
    return m->ready && m->ui.kind!=0;
}
unsigned fh_pica_material_layered_selected(const FhPicaMaterialSet *p,unsigned index){
    if(!p->items || index>=p->count)return 0;
    const Material *m=(const Material*)p->items+index;
    if(!m->ready || !m->actor || m->ui.kind || m->moon_alpha || m->movie_active ||
       (fh_graphics_options&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ACTOR_TEX)))return 0;
    unsigned base=m->environment && !(fh_graphics_options&FH_GRAPHICS_NO_ENVIRONMENT);
    /* This shader uses c7 for layer UVs and does not multiply vertex
     * colour by tint. Constant alpha plus unit RGB tint makes single-layer
     * and masked draws compatible, avoiding repeated pipeline/palette upload. */
    if(m->tint[0]!=1.f || m->tint[1]!=1.f || m->tint[2]!=1.f)return 0;
    if(m->specular_program && (fh_graphics_options&FH_GRAPHICS_SPECULAR)){
        if(!m->layered)return 0; /* Single-layer C2 consumes the full map. */
        if(!fh_pica_specular_half_sampler(m->specular_slot))return 0;
    }else{
        if(!m->alpha_constant || !m->diffuse_program || !(fh_graphics_options&FH_GRAPHICS_DIFFUSE_MAPS) ||
           (base && !fh_pica_diffuse_map_sampler(m->environment_slot)))return 0;
        base=0; /* Diffuse map replaces the analytic light; retain fragment fog. */
    }
    return fh_pica_environment_layered_available(m->environment_slot,base,fh_pica_fog_layered_enabled(m->actor_fog));
}
static Material *actor_height_material(const FhPicaMaterialSet*p,unsigned index){
    if(!p || !p->items || index>=p->count)return NULL;
    Material*m=(Material*)p->items+index;
    /* UV2/sampler2 belongs to height fog in this shader. Special sphere,
     * procedural and callback families require their own admission. */
    if(!m->ready || !m->actor || m->layers>2 || m->environment ||
       m->specular_program || m->ui.kind || m->moon_alpha ||
       m->movie_active || m->cloud_state || m->post_state || (m->flags&0x60u) ||
       !m->program || m->program>PROGRAMS || !programs[m->program-1].refs)return NULL;
    return m;
}
static unsigned actor_height_program(const Material*m){
    /* Match ordinary binding's current channel selection. Unknown channel
     * families stay on that ordinary path, never silently lose their swizzle. */
    unsigned handle=m->swizzle?(!m->layered?m->swizzle_program:0):m->program;
    return handle && handle<=PROGRAMS && programs[handle-1].refs?handle:0;
}
static int actor_height_prepare_program(unsigned handle){
    if(!handle || handle>PROGRAMS || !programs[handle-1].refs)return 0;
    Program*base=programs+handle-1;
    if(base->actor_height)return 1;
    C3D_TexEnv stages[4];
    if(!fh_pica_actor_fog_tev(stages,base->stages))return 0;
    unsigned child=acquire_program(stages,base->buffer_mask);
    if(!child)return 0;
    base->actor_height=child;return 1;
}
static int actor_height_program_capable(unsigned handle){
    if(!handle || handle>PROGRAMS || !programs[handle-1].refs)return 0;
    const Program*base=programs+handle-1;
    if(base->actor_height)return 1;
    C3D_TexEnv stages[4];
    return fh_pica_actor_fog_tev(stages,base->stages);
}
int fh_pica_material_actor_height_capable(const FhPicaMaterialSet*p,unsigned index){
    Material*m=actor_height_material(p,index);if(!m)return 0;
    /* Test potential selectors, not only the default selector at load time.
     * No program references or GPU storage acquired on a missing asset. */
    return actor_height_program_capable(m->program) ||
        (!m->layered && actor_height_program_capable(m->swizzle_program));
}
int fh_pica_material_actor_height_prepare(const FhPicaMaterialSet*p,unsigned index){
    Material*m=actor_height_material(p,index);if(!m)return 0;
    int prepared=actor_height_prepare_program(m->program);
    if(m->swizzle_program && !m->layered)
        prepared|=actor_height_prepare_program(m->swizzle_program);
    return prepared;
}
int fh_pica_material_actor_height_ready(const FhPicaMaterialSet*p,unsigned index){
    Material*m=actor_height_material(p,index);
    unsigned handle=m?actor_height_program(m):0;
    return handle && programs[handle-1].actor_height!=0;
}
int fh_pica_material_actor_height_bind(const FhPicaMaterialSet*p,unsigned index,
    unsigned format,unsigned compact,unsigned color){
    Material*m=actor_height_material(p,index);
    const C3D_Tex*sampler=fh_pica_height_fog_sampler();
    unsigned handle=m?actor_height_program(m):0;
    if(!handle || !programs[handle-1].actor_height || !sampler || fh_pica_scene_layered_active())return 0;
    if(actor_height_binding)fh_platform_panic("PICA actor height reentry");
    actor_height_binding=1;actor_height_color=color;
    int result=fh_pica_material_bind(p,index,format,compact);
    actor_height_binding=0;
    if(result==1)C3D_TexBind(2,(C3D_Tex*)sampler);
    return result;
}
unsigned fh_pica_material_height_fog_eligible(const FhPicaMaterialSet *p,unsigned index){
    if(!p || !p->items || index>=p->count)return 0;
    const Material *m=(const Material*)p->items+index;
    /* UV1/sampler1 or UV2/sampler2 supplies height fog. Stage 4 is
     * free on world draws even when all four material stages are used.
     * Source cloud/ring owners still take precedence. */
    if(!m->ready || m->actor || m->layers>2 || m->masked || m->environment ||
       m->ui.kind || m->moon_alpha || m->movie_active || m->cloud_state || m->post_state ||
       (m->flags&0x60u) || !m->program)return 0;
    return m->layers==2?2:1;
}
#include "platform_pica_world_material.h"
#include "platform_pica_world_commands.h"
#include "platform_pica_actor_commands.h"
int fh_pica_material_bind(const FhPicaMaterialSet *p,unsigned index,unsigned format,unsigned compact){
    if(!p->items || index>=p->count)return 0;
    Material *m=(Material*)p->items+index;if(!m->ready){fh_platform_log("PICA material uninitialized\n");return 0;}
    unsigned layered=fh_pica_scene_layered_active();
    unsigned height=fh_pica_scene_actors && !m->actor && fh_pica_height_fog_world_active()?
        fh_pica_material_height_fog_eligible(p,index):0;
    if(height==2 || actor_height_binding){
        float parameters[2];unsigned fog=fh_pica_fog_layered_bind(actor_height_binding?m->actor_fog:!!(m->flags&4),parameters);
        if(!fh_pica_environment_layered_bind(0,0,fog,parameters))fh_platform_panic("PICA height fog fragment owner");
    }else if(!layered){
        if(!m->actor)C3D_LightEnvBind(NULL);
        fh_pica_fog_bind(fh_pica_scene_actors && !m->ui.kind && !m->moon_alpha && !m->movie_active && (m->actor?m->actor_fog:!!(m->flags&4)));
    }
    fh_pica_scene_specular(0);
    if(!layered && world_material_plain(m))return world_material_bind(m,format,compact);
    if(m->ui.kind)return bind_ui(m,format,compact);
    if(m->moon_alpha)return bind_moon(m,format,compact);
    if(m->movie_active){
        if(!m->movie_pixels || (fh_graphics_options&FH_GRAPHICS_NO_MOVIES))return 2;
        bound_program=0;bindings++;return fh_pica_movie_bind(m->movie_pixels,format,compact);
    }
    unsigned environment_on=m->environment && !(fh_graphics_options&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ENVIRONMENT));
    const C3D_Tex *diffuse=NULL;
    if(m->diffuse_program && (environment_on || layered) && !(fh_graphics_options&FH_GRAPHICS_NO_LIGHTING) && (fh_graphics_options&FH_GRAPHICS_DIFFUSE_MAPS) &&
       !(fh_graphics_options&FH_GRAPHICS_NO_ACTOR_TEX) && (!m->layered || layered) &&
       !(m->specular_program && (fh_graphics_options&FH_GRAPHICS_SPECULAR))) {
        diffuse=environment_on?fh_pica_diffuse_map_sampler(m->environment_slot):&neutral_texture;
        if(!diffuse || !fh_pica_scene_specular(1)){diffuse=NULL;diffuse_unready++;}
    }
    if(m->actor){
        fh_pica_scene_actor_second_uv(layered?0:m->layered);
        if(layered){
            float parameters[2];unsigned fog=fh_pica_fog_layered_bind(m->actor_fog,parameters);
            if(!fh_pica_environment_layered_bind(m->environment_slot,environment_on && !diffuse,fog,parameters))fh_platform_panic("PICA layered light admission");
        }else if(!actor_height_binding)fh_pica_environment_bind(m->environment_slot,environment_on && !diffuse);
    }
    if(!m->actor && (m->flags&((fh_pica_world_water_active?0u:0x80000000u)|2u)))return 2;
    if(!m->program || !programs[m->program-1].refs)fh_platform_panic("PICA material program lifetime");

    const C3D_Tex *specular=NULL;
    if(m->specular_program && (!m->layered || layered) && (fh_graphics_options&FH_GRAPHICS_SPECULAR) &&
       !(fh_graphics_options&(FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ACTOR_TEX))) {
        specular=layered?fh_pica_specular_half_sampler(m->specular_slot):fh_pica_specular_sampler(m->specular_slot);
        if(!specular || !fh_pica_scene_specular(1)){specular=NULL;specular_unready++;}
    }
    fh_pica_texture2_coord1((specular || diffuse) && m->masked);
    if(!specular && !diffuse)C3D_TexBind(2,NULL);
    const C3D_Tex *sphere=specular?specular:diffuse;
    unsigned swizzle_handle=m->environment?(m->swizzle>=6?m->swizzle_environment[fh_pica_texture_swizzle_missing(m->swizzle)]:0):m->swizzle_program;
    if(diffuse)swizzle_handle=m->environment?(m->swizzle>=6?m->swizzle_diffuse[1+fh_pica_texture_swizzle_missing(m->swizzle)]:0):m->swizzle_diffuse[0];
    unsigned swizzle=m->swizzle && swizzle_handle && !specular && (!layered || diffuse) && !m->ui.kind && !m->moon_alpha;
    swizzle_draws+=swizzle;swizzle_missing+=m->swizzle && !swizzle;
    swizzle_diffuse_draws+=swizzle && diffuse;swizzle_environment_draws+=swizzle && m->environment;swizzle_masked_draws+=swizzle && m->masked;
    unsigned handle=swizzle?swizzle_handle:specular?m->specular_program:diffuse?m->diffuse_program:m->program;
    if(actor_height_binding)handle=programs[handle-1].actor_height;
    Program *program=programs+handle-1;
    if(!program->refs)fh_platform_panic("PICA specular material lifetime");
    unsigned changed=bound_program!=handle;
    if(changed){C3D_TexEnvBufUpdate(C3D_RGB,program->buffer_mask&15);C3D_TexEnvBufUpdate(C3D_Alpha,program->buffer_mask>>4);program_binds++;}
    else program_reuses++;
    for(unsigned i=0;i<4;i++){
        unsigned color=m->environment && !i?0xff000000u|(environment_on?m->environment_strength*0x010101u:0):m->konst;
        if(swizzle)color=m->environment?fh_pica_texture_swizzle_environment_color(m->swizzle,i,m->konst,environment_on?m->environment_strength:0):fh_pica_texture_swizzle_color(m->swizzle,i,m->konst);
        else if(layered && specular){
            if(m->layered&2){
                if(i==0)color=(environment_on?m->environment_strength*0x010101u:0)|(((m->konst>>24)/2)<<24);
                if(i==2)color=0x80000000u;
            }else{
                if(i==0)color=(environment_on?m->environment_strength*0x010101u:0)|(m->konst&0xff000000u);
                if(i==1)color=0x00808080u;
            }
        }else if(specular && i==1)color=m->specular_alpha<<24;
        if(actor_height_binding && i==3)color=actor_height_color;
        if(changed){
            C3D_SetTexEnv(i,program->stages+i);
            C3D_TexEnvColor(C3D_GetTexEnv(i),color);
        }else if(bound_colors[i]!=color)C3D_TexEnvColor(C3D_GetTexEnv(i),color);
        bound_colors[i]=color;
    }
    bound_program=handle;specular_postlight_bindings+=specular && layered && !(m->layered&2);specular_bindings+=specular!=NULL;diffuse_bindings+=diffuse && environment_on;diffuse_plain_bindings+=diffuse && !environment_on;diffuse_mask_bindings+=diffuse && m->masked;diffuse_layered_bindings+=diffuse && m->layered;diffuse_shared_bindings+=diffuse && layered && !m->layered;
    float cs=compact && !m->actor?1.f/15:1.f,us=compact?(format==5?1.f/256:1.f/1024):1.f;
    C3D_FVUnifSet(GPU_VERTEX_SHADER,7,m->tint[0]*cs,m->tint[1]*cs,m->tint[2]*cs,m->tint[3]*cs);
    for(unsigned i=0;i<2;i++){
        Frame *f=i<m->layers?m->frames[i]+m->current[i]:NULL;
        if(f && (!f->handle || !images[f->handle-1].refs))fh_platform_panic("PICA material image lifetime");
        float sx=f?f->scale[0]:1,sy=f?f->scale[1]:1;
        unsigned unit=sphere?i+1:i;
        C3D_TexBind(unit,fh_pica_options_texture(f?&f->sampler:NULL,m->actor?FH_GRAPHICS_NO_ACTOR_TEX:FH_GRAPHICS_NO_WORLD_TEX));
        if(layered && i)C3D_FVUnifSet(GPU_VERTEX_SHADER,7,sx*us,sy*us,m->scroll[i][0]*sx,m->scroll[i][1]*sy);
        else if(!sphere || !i)C3D_FVUnifSet(GPU_VERTEX_SHADER,8+unit,sx*us,sy*us,m->scroll[i][0]*sx,m->scroll[i][1]*sy);
    }
    if(sphere){
        /* Mask and base share their source coordinates, checked once at load.
         * UV2 stays dedicated to distance fog; no extra geometry pass. */
        C3D_TexBind(0,(C3D_Tex*)sphere);
        C3D_FVUnifSet(GPU_VERTEX_SHADER,8,-.5f,-.5f,.5f,.5f);
        specular_mask_bindings+=specular && m->masked;
    }
    unsigned blend=!!(m->flags&0x60000000u) || (m->actor && m->tint[3]<1.f);
    if(m->cloud_state){
        unsigned s=m->cloud_state,mask=((s&8)?GPU_WRITE_COLOR:0)|((s&4)?GPU_WRITE_DEPTH:0);
        C3D_DepthTest(!!(s&2),GPU_GEQUAL,mask);
        C3D_AlphaTest(!!(s&32),GPU_GREATER,(s>>8)&255);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,(s&16)?GPU_SRC_ALPHA:GPU_ONE,(s&16)?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
        cloud_draws[!(s&8)]++;
    }else if(m->post_state){
        /* These known original callbacks replace the default raster state
         * after material setup. Their blend decision uses object renderAlpha,
         * not the combined object/material alpha, and never writes depth. */
        C3D_DepthTest(true,GPU_GEQUAL,GPU_WRITE_COLOR);
        C3D_AlphaTest(false,GPU_ALWAYS,0);
        unsigned additive=m->post_state==FH_PICA_POST_ADDITIVE;
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,additive?GPU_SRC_ALPHA:GPU_ONE,additive?GPU_ONE:GPU_ZERO,GPU_ONE,GPU_ZERO);
        post_draws[m->post_state]++;
    }else if(m->actor){
        unsigned s=m->actor_raster;
        C3D_DepthTest(!!(s&FH_AR_DEPTH),GPU_GEQUAL,GPU_WRITE_COLOR|((s&FH_AR_WRITE)?GPU_WRITE_DEPTH:0));
        C3D_AlphaTest(!!(s&FH_AR_ALPHA),GPU_GREATER,(s>>8)&255);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,(s&FH_AR_BLEND)?GPU_SRC_ALPHA:GPU_ONE,
            (s&FH_AR_BLEND)?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    }else{
        C3D_DepthTest(true,GPU_GEQUAL,blend?GPU_WRITE_COLOR:GPU_WRITE_ALL);
        C3D_AlphaTest(!blend && (m->masked || ((m->flags&0x400) && !(m->flags&0x80))),GPU_GREATER,0);
        C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,blend?GPU_SRC_ALPHA:GPU_ONE,blend?GPU_ONE_MINUS_SRC_ALPHA:GPU_ZERO,GPU_ONE,GPU_ZERO);
    }
    bindings++;return 1;
}
void fh_pica_materials_release(FhPicaMaterialSet *p){
    if(!p || !p->items)return;
    actor_stream_release(p);world_stream_release(p);
    Material *m=p->items;
    for(unsigned i=0;i<p->count;i++)if(m[i].fur_mask){
        release_image(m[i].fur_mask->handle);deallocate(m[i].fur_mask,sizeof(Frame));
    }
    for(unsigned i=0;i<p->count;i++)deallocate(m[i].projected_samplers,m[i].count[0]*sizeof(C3D_Tex));
    for(unsigned i=0;i<p->count;i++)if(m[i].program)release_program(m[i].program);
    for(unsigned i=0;i<p->count;i++)if(m[i].specular_program)release_program(m[i].specular_program);
    for(unsigned i=0;i<p->count;i++)if(m[i].diffuse_program)release_program(m[i].diffuse_program);
    for(unsigned i=0;i<p->count;i++)if(m[i].swizzle_program)release_program(m[i].swizzle_program);
    for(unsigned i=0;i<p->count;i++)for(unsigned j=0;j<3;j++)if(m[i].swizzle_environment[j])release_program(m[i].swizzle_environment[j]);
    for(unsigned i=0;i<p->count;i++)for(unsigned j=0;j<4;j++)if(m[i].swizzle_diffuse[j])release_program(m[i].swizzle_diffuse[j]);
    for(unsigned i=0;i<p->count;i++)for(unsigned j=0;j<2;j++){
        if(m[i].frames[j])for(unsigned k=0;k<m[i].count[j];k++)release_image(m[i].frames[j][k].handle);
        deallocate(m[i].frames[j],m[i].count[j]*sizeof(Frame));
    }
    deallocate(m,p->count*sizeof(Material));p->items=NULL;p->count=0;
}
void fh_pica_materials_init(void){
    if(!C3D_TexInit(&neutral_texture,8,8,GPU_RGBA8))fh_platform_panic("PICA neutral texture");
    memset(neutral_texture.data,255,256);fh_gpu_cache_flush(neutral_texture.data,256);
    C3D_TexSetWrap(&neutral_texture,GPU_REPEAT,GPU_REPEAT);
    if(fh_pica_scene_textured){
        fh_texture_pack_open(&pack,fh_asset_path(FH_ASSET_TEXTURE),4096*44);
        extern FILE *fh_sd_reader_stream(const char*);
        FILE *fast=pack.file?fh_sd_reader_stream(fh_asset_path(FH_ASSET_TEXTURE)):NULL;
        if(fast){fclose(pack.file);pack.file=fast;}
        texture_window_init();
    }
    if(fh_pica_scene_actors){ui_programs_init();moon_programs_init();}
}
static void fur_material_report(void);
void fh_pica_materials_shutdown(void){
    {char line[192];snprintf(line,sizeof line,"PICA actor command stream: plans=%u draws=%u other=%u bytes=%u peak=%u program_bytes=%u\n",
        actor_stream_plans,actor_stream_draws,actor_stream_other,actor_stream_bytes,actor_stream_peak,(unsigned)(PROGRAMS*sizeof(FhPicaActorMaterial)));fh_platform_log(line);}
    {char line[192];snprintf(line,sizeof line,"PICA world command stream: plans=%u draws=%u other=%u hidden=%u bytes=%u peak=%u\n",
        world_stream_plans,world_stream_draws,world_stream_other,world_stream_hidden,world_stream_bytes,world_stream_peak);fh_platform_log(line);}
    {char line[100];snprintf(line,sizeof line,"PICA plain world material: draws=%u\n",world_material_binds);fh_platform_log(line);}
    {char line[120];snprintf(line,sizeof(line),"PICA diffuse families: masked=%u layered=%u shared=%u plain=%u\n",diffuse_mask_bindings,diffuse_layered_bindings,diffuse_shared_bindings,diffuse_plain_bindings);fh_platform_log(line);}

    {char line[180];snprintf(line,sizeof(line),"PICA diffuse materials: admitted=%u partial=%u draws=%u unready=%u\n",diffuse_admitted,diffuse_partial,diffuse_bindings,diffuse_unready);fh_platform_log(line);}

    world_packets_report();
    fur_material_report();
    {char line[144];snprintf(line,sizeof(line),"PICA source alpha: constant_materials=%u replaced_base_masks=%u\n",alpha_constant_materials,alpha_replaced_masks);fh_platform_log(line);}
    {char specular_line[144];snprintf(specular_line,sizeof(specular_line),"PICA specular materials: draws=%u unready=%u masked=%u\n",specular_bindings,specular_unready,specular_mask_bindings);fh_platform_log(specular_line);}

    C3D_TexDelete(&neutral_texture);
    char line[256];snprintf(line,sizeof(line),"PICA materials: loads=%u conversions=%u errors=%u bindings=%u gpu=%u gpu_peak=%u cpu=%u cpu_peak=%u load_ticks=%llu sd_bytes=%llu\n",loads,conversions,errors,bindings,gpu_bytes,gpu_peak,cpu_bytes,cpu_peak,(unsigned long long)load_ticks,(unsigned long long)pack.read_bytes);fh_platform_log(line);snprintf(line,sizeof(line),"PICA mip images: loaded=%u failures=%u skipped=%u reads=%u chains=%u\n",mip_images,mip_failures,mip_skipped,pack.mip_reads,pack.mip_count);fh_platform_log(line);
    if(gpu_bytes || cpu_bytes)fh_platform_panic("PICA material lifetime leak");
    for(unsigned i=0;i<16;i++)if(ui_program[i]){release_program(ui_program[i]);ui_program[i]=0;}
    for(unsigned i=0;i<2;i++)if(moon_program[i]){release_program(moon_program[i]);moon_program[i]=0;}
    snprintf(line,sizeof(line),"PICA moon: draws=%u celestial_updates=%u\n",moon_draws,fh_pica_celestial_updates);fh_platform_log(line);
    for(unsigned i=1;i<=4;i++){
        snprintf(line,sizeof(line),"PICA UI model: kind=%u draws=%u hidden=%u\n",i,ui_draws[i],ui_hidden[i]);fh_platform_log(line);
    }
    snprintf(line,sizeof(line),"PICA post material: additive_draws=%u opaque_no_depth_write_draws=%u\n",post_draws[1],post_draws[2]);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA postlight specular: draws=%u\n",specular_postlight_bindings);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA clouds: color_draws=%u depth_draws=%u glare_draws=%u\n",cloud_draws[0],cloud_draws[1],fh_pica_cloud_glare_draws);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA actor live: publications=%u extra_resident_bytes=4\n",actor_live_publications);fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA programs: binds=%u reuses=%u peak=%u resident=%u static_bytes=%u\n",program_binds,program_reuses,program_peak,program_count,(unsigned)sizeof(programs));fh_platform_log(line);
    snprintf(line,sizeof(line),"PICA texture channels: admitted=%u partial=%u draws=%u missing=%u extra_images=0 diffuse=%u environment=%u masked=%u\n",swizzle_admitted,swizzle_partial,swizzle_draws,swizzle_missing,swizzle_diffuse_draws,swizzle_environment_draws,swizzle_masked_draws);fh_platform_log(line);
    if(program_count)fh_platform_panic("PICA material program leak");
    texture_window_report();fh_pica_texture_window_release();
    fh_texture_pack_close(&pack);
}

#include "platform_pica_fur_material.h"


FhPicaActorLiveTarget *fh_pica_actor_live_target(const FhSceneAsset *asset,unsigned index,unsigned *dual){
    Material *m=material_for(asset,index);
    if(!m || !m->ready)return NULL;
    if(!m->actor)fh_platform_panic("PICA actor live target identity");
    if(!actor_stream_admit(asset)){
        char line[160];snprintf(line,sizeof line,"PICA actor command admission id=%u stage=%u requested=%u material_cpu=%u heap=%u\n",
            asset->id,actor_stream_error,actor_stream_requested,cpu_bytes,(unsigned)mallinfo().uordblks);fh_platform_log(line);
        fh_platform_panic("PICA actor command admission");
    }
    *dual=m->layered;
    return (FhPicaActorLiveTarget*)m;
}
void fh_pica_actor_live_publish(FhPicaActorLiveTarget *target,const FhPicaActorLiveCommon *object,const FhPicaActorLiveValues *value){
    Material *m=(Material*)target;
    /* The checked resolver and publication are synchronous on the render
     * producer. This does not cache source pointers or bypass retirement. */
    if(value->selector>=24)fh_platform_panic("PICA texture channel selector");
    if(object->post_state>FH_PICA_POST_OPAQUE)fh_platform_panic("PICA post material state");
    if(FH_PICA_ACTOR_LIGHTING_EXPERIMENT && m->environment!=!!value->environment_enabled)
        fh_platform_panic("PICA environment material identity changed");
    m->ui.kind=0;m->post_state=object->post_state;m->movie_pixels=NULL;m->movie_active=0;
    m->cloud_state=0;m->moon_alpha=0;
    m->tint[0]=m->tint[1]=m->tint[2]=1.f;m->tint[3]=value->alpha/255.f;
    memcpy(m->scroll,value->scroll,sizeof(m->scroll));m->konst=value->konst;m->flags=value->flags;
    for(unsigned i=0;i<m->layers;i++)m->current[i]=(value->frames[i]>>8)<m->count[i]?
        value->frames[i]>>8:m->count[i]-1;
    m->swizzle=m->swizzle_source?value->selector:0;
    m->actor_raster=fh_pica_actor_raster_live(m->actor_raster_plan,object->object_alpha,object->threshold);
    m->environment_slot=object->environment_slot;m->environment_strength=object->environment_strength;
    if(object->cloud_mode){
        if(object->cloud_mode>2)fh_platform_panic("PICA cloud material scope");
        m->cloud_state=fh_pica_cloud_raster(object->cloud_mode,object->model_flags,m->flags,
            object->threshold,object->object_alpha,object->shadow);
    }
    actor_live_publications++;
}
