#include "fh_pica_shadow_project.h"
#include "fh_pica_scene.h"
#include "fh_pica_world_deferred.h"
#include "fh_pica_shadow_points.h"
#include "fh_pica_model.h"
#include "fh_pica_morph.h"
#include "fh_pica_shadow_plan.h"
#include "fh_pica_fur_plan.h"
#include "fh_graphics_options.h"
#include "fh_asset_paths.h"
#include "fh_replay_timing.h"
#include <3ds.h>
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "fh_native_scene.h"
#include "fh_scene_storage.h"
#include "fh_platform.h"
#include "fh_scene_texture_identity.h"
#include "fh_model_resources.h"
#include "fh_scene_snapshot.h"
#include "fh_gpu_cache.h"
#include "fh_minimal_graphics.h"
#include "fh_memory_budget.h"
#include "fh_block_cache.h"
#include "fh_pica_texture_window.h"
#include "fh_asset_staging.h"
/* Scene-wide native storage, allocated only as live assets are admitted.
 * Every admission also retains the global normal/linear headroom guards. */
/* The long console opening exhausted the original 2 MiB metadata allowance.
 * Allocate only live demand; independent 15% normal and target-specific
 * linear reserves reject allocations before exhausting console memory. */
enum { SCENE_RECORDS=256,SCENE_CPU_LIMIT=4*1024*1024,SCENE_GPU_LIMIT=6*1024*1024,
       SCENE_FILE_LIMIT=64*1024*1024 };
#define SCENE_LINEAR_RESERVE FH_NATIVE_LINEAR_RESERVE
typedef struct {unsigned at,bytes;} SceneSpan;
typedef struct {const void *data;unsigned version;} SceneVersion;
typedef struct SceneModelView {struct SceneModelView *next;void *gpu;unsigned char *snapshots;
    SceneSpan spans[4];SceneVersion versions[4];unsigned last_used,allocation_bytes,unpublished;
    FhAnimatedBounds *model_bounds;} SceneModelView;
#include "fh_actor_fog_resident.h"
#include "fh_native_actor_fog_owner.h"
typedef struct FhSceneActorFog FhSceneActorFog;
typedef struct {const void *owner;void *meta,*gpu;FhSceneAsset asset;
    FhSceneActorFog *actor_fog;unsigned actor_fog_dirty;
    unsigned char *snapshots;unsigned snapshot_bytes,span_count,last_used,invalid,gpu_used_bytes;SceneSpan *spans;
    SceneVersion *versions;void *world_bounds;unsigned world_bounds_bytes,world_fine_count;
    union {FhWorldDrawPlan *world_plan;FhPicaWorldPlan *pica_plan;};unsigned world_plan_bytes;
    FhPicaWorldDeferredPlan *deferred;
    FhPicaModelPlan *actor_plan;unsigned actor_plan_bytes;
    FhPicaShadowPlan *shadow_plan;unsigned shadow_valid;
    FhPicaFurPlan *fur_plan;
    FhShadowPointPlan *track_plan;unsigned track_dirty;
    SceneModelView *views,*active;unsigned view_count;FhAnimatedBounds *model_bounds;unsigned model_bounds_bytes;} SceneRecord;
static SceneRecord records[SCENE_RECORDS];
unsigned fh_pica_shadow_track_enabled;
static unsigned initialized,entries,file_bytes;
static FILE *file;
typedef struct { uint32_t handle,span_bytes; uint64_t size,span_offset,calls,bytes; unsigned char *span; } FhSdReader;
int fh_sd_reader_open(FhSdReader *reader,const char *path);
int fh_sd_reader_read(FhSdReader *reader,uint64_t offset,void *out,uint32_t bytes,uint64_t hint_end);
void fh_sd_reader_close(FhSdReader *reader);
FILE *fh_sd_reader_stream(const char *path);
static FhSdReader scene_reader;
static unsigned scene_hint_end;
static FhBlockCache scene_cache;
static unsigned scene_cursor;
static u64 cache_load_ticks,cache_read_ticks,scene_sd_ticks,scene_sd_bytes;
static unsigned cache_releases,scene_sd_reads;
unsigned fh_native_scene_cache_release(void){
    unsigned bytes=scene_cache.storage_bytes;if(!bytes)return 0;
    /* Release can happen between header and payload admission. Restore the
     * original stream position before the next uncached read. */
    if(fseek(file,scene_cursor,SEEK_SET))return 0;
    fh_block_cache_close(&scene_cache);cache_releases++;return bytes;
}
static int scene_seek(unsigned at){
    scene_cursor=at;return scene_cache.storage?0:fseek(file,at,SEEK_SET);
}
static size_t scene_read(void *out,unsigned bytes){
    if(scene_cache.storage){
        u64 before_reads=scene_cache.sd_reads,before_bytes=scene_cache.sd_bytes;
        u64 start=svcGetSystemTick();int ok=fh_block_cache_read(&scene_cache,scene_cursor,out,bytes);
        u64 elapsed=svcGetSystemTick()-start;cache_read_ticks+=elapsed;
        /* Miss spans include decode/copy work; filesystem IPC timing remains
         * separately available in sd-io.csv. Never label paged reads SD-free. */
        unsigned misses=(unsigned)(scene_cache.sd_reads-before_reads);
        scene_sd_reads+=misses;scene_sd_bytes+=scene_cache.sd_bytes-before_bytes;
        if(misses)scene_sd_ticks+=elapsed;
        if(ok){scene_cursor+=bytes;return bytes;}
        /* Unpublished destination is overwritten from the original pack on a
         * cache fault. A cache never changes source admission or GPU fences. */
        fh_block_cache_close(&scene_cache);
        if(fseek(file,scene_cursor,SEEK_SET))return 0;
    }
    u64 start=svcGetSystemTick();size_t got;
    if(scene_reader.handle)got=fh_sd_reader_read(&scene_reader,scene_cursor,out,bytes,
        scene_hint_end>scene_cursor+bytes?scene_hint_end:scene_cursor+bytes)?bytes:0;
    else got=fread(out,1,bytes,file);
    scene_sd_ticks+=svcGetSystemTick()-start;scene_sd_reads++;scene_sd_bytes+=got;
    scene_cursor+=(unsigned)got;return got;
}
static unsigned char *directory;
unsigned fh_native_scene_loaded,fh_native_scene_rejected,fh_native_scene_cpu_bytes,fh_native_scene_gpu_bytes;
unsigned fh_native_scene_admission[16];
unsigned fh_native_scene_enabled;
static u64 scene_uploaded_bytes;
static unsigned world_position_refreshes;
unsigned fh_world_plan_runs,fh_world_plan_draws;
static unsigned plan_bytes,plan_peak,plan_builds,plan_rejected,plan_hits;
static unsigned world_hierarchy_bytes,world_hierarchy_peak;
static unsigned model_bounds_bytes,model_bounds_peak,model_bounds_enabled;
static u64 world_position_bytes;
static u64 scene_source_ticks,scene_crc_ticks,scene_source_bytes,scene_crc_bytes;
static FhSceneTextureIdentity texture_identities[64];
static unsigned texture_identity_cursor;
static unsigned model_views_mode=1,model_view_gpu_bytes,model_view_peak,model_view_allocations,model_view_hits,model_view_fallbacks;
#ifdef FH_EMULATOR_TEST
typedef struct {unsigned id,calls,changes,switches,masks[16];u64 bytes,switched_bytes;} RefreshProfile;
static RefreshProfile refresh_profile[SCENE_RECORDS];
#endif
static unsigned word(const unsigned char *p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static unsigned be16(const unsigned char *p){return (unsigned)p[0]<<8|p[1];}
static unsigned be32(const unsigned char *p){return be16(p)<<16|be16(p+2);}
static unsigned source_spans(SceneSpan out[262],const unsigned char *raw,unsigned bytes,unsigned block){
    if(bytes<(block?164u:252u))return 0;
    unsigned count=raw[block?0xa1:0xf5],table=be32(raw+(block?0x68:0xd0));
    if(table>bytes || count*28>bytes-table)return 0;
    unsigned offsets[4]={block?0x58:0x28,block?0x58:0x2c,block?0x5c:0x30,block?0x60:0x34};
    unsigned sizes[4]={be16(raw+(block?0x90:0xe4))*6,block?0:be16(raw+0xe6)*((raw[0x24]&8)?9:3),
        be16(raw+(block?0x94:0xe8))*2,be16(raw+(block?0x96:0xea))*4};
    for(unsigned i=0;i<4;i++)out[i]=(SceneSpan){be32(raw+offsets[i]),sizes[i]};
    out[4]=(SceneSpan){be32(raw+(block?0x78:0xd4)),be16(raw+(block?0x84:0xd8))};
    for(unsigned i=0;i<count;i++){const unsigned char *e=raw+table+i*28;out[5+i]=(SceneSpan){be32(e),be16(e+4)};}
    unsigned spans=5+count;
    if(!block)out[spans++]=(SceneSpan){be32(raw+0x54),raw[0xf4]*4u};
    else for(unsigned i=0;i<2;i++)out[spans++]=(SceneSpan){be32(raw+0x7c+i*4),be16(raw+0x86+i*2)};
    for(unsigned i=0;i<spans;i++)if(out[i].at>bytes || out[i].bytes>bytes-out[i].at)return 0;
    return spans;
}
static int normal_reserve(unsigned bytes){
    extern u32 __ctru_heap_size,__stacksize__;
    /* Drop small texture read pages before sacrificing the scene working set.
     * Neither cache owns any GPU-visible references. */
    if(bytes<=SCENE_CPU_LIMIT-fh_native_scene_cpu_bytes &&
       ((u64)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64)*100>(u64)__ctru_heap_size*85)
        fh_pica_texture_window_release();
    if(bytes<=SCENE_CPU_LIMIT-fh_native_scene_cpu_bytes &&
       ((u64)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64)*100>(u64)__ctru_heap_size*85)
        fh_native_scene_cache_release();
    return bytes<=SCENE_CPU_LIMIT-fh_native_scene_cpu_bytes &&
        ((u64)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64)*100<=(u64)__ctru_heap_size*85;
}
/* CPU metadata has no GPU references. It may share linear RAM when the
 * normal heap cannot admit it; preserve both pool reserves and the scene cap.
 * Existing wrapped SDK allocators serialize the free list. A post-allocation
 * check handles intervening allocations without weakening the reserve. */
static unsigned char scene_meta_linear[SCENE_RECORDS];
static unsigned scene_meta_linear_bytes,scene_meta_linear_peak;
static int scene_meta_room(unsigned upcoming){
    extern u32 __ctru_linear_heap_size;
    unsigned available=linearSpaceFree();
    return upcoming<=available && available-upcoming>=SCENE_LINEAR_RESERVE &&
        (u64)(available-upcoming)*100>=(u64)__ctru_linear_heap_size*15;
}
static void scene_meta_free(void *p,unsigned bytes,unsigned in_linear){
    if(!p)return;
    if(in_linear){linearFree(p);scene_meta_linear_bytes-=bytes;}
    else free(p);
}
enum { SCENE_META_NORMAL_MARGIN=64*1024 };
static int scene_meta_normal_margin(unsigned bytes){
    extern u32 __ctru_heap_size,__stacksize__;
    return ((u64)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64+SCENE_META_NORMAL_MARGIN)*100<=(u64)__ctru_heap_size*85;
}
static void *scene_meta_allocate(unsigned bytes,unsigned gpu_bytes,unsigned *in_linear){
    *in_linear=0;
    if(!bytes || bytes>SCENE_CPU_LIMIT-fh_native_scene_cpu_bytes)return NULL;
    if(scene_meta_normal_margin(bytes) && normal_reserve(bytes)){
        void *p=malloc(bytes);if(p)return p;
    }
    /* Source bounds cap metadata at 4 MiB and GPU data at 6 MiB. Widen the
     * addition nevertheless, and allow for both allocator alignments. */
    u64 required=(u64)bytes+gpu_bytes+256;
    if(required>0xffffffffu || !scene_meta_room((unsigned)required))return NULL;
    void *p=linearAlloc(bytes);if(!p)return NULL;
    if(!scene_meta_room(gpu_bytes+128)){linearFree(p);return NULL;}
    *in_linear=1;scene_meta_linear_bytes+=bytes;
    if(scene_meta_linear_bytes>scene_meta_linear_peak)scene_meta_linear_peak=scene_meta_linear_bytes;
    return p;
}
static void scene_meta_release(SceneRecord *r){
    unsigned slot=(unsigned)(r-records);
    scene_meta_free(r->meta,r->asset.metadata_bytes+r->snapshot_bytes,scene_meta_linear[slot]);
    scene_meta_linear[slot]=0;
}
#include "platform_scene_model_views.h"
#include "platform_scene_shadow_plan.h"
#include "platform_scene_fur_groom.h"
#include "platform_scene_fur_plan.h"
#include "platform_scene_shadow_points.h"
static unsigned projected_shadow_skips;
static int projected_shadows_wanted(void){
    if((fh_graphics_options&FH_GRAPHICS_SILHOUETTE_SHADOWS) && !(fh_graphics_options&FH_GRAPHICS_NO_GROUND_SHADOWS))return 1;
    projected_shadow_skips++;return 0;
}
/* CPU-only program storage has the same owner and retirement as scene data.
 * It never owns transient uniforms/textures or changes a GPU fence. */
static void prepare_world_plan(SceneRecord *r){
    if(fh_pica_scene_actors && !r->asset.block){
        unsigned count=fh_pica_model_compile(&r->asset,NULL,0);
        unsigned bytes=sizeof(FhPicaModelPlan)+count*sizeof(FhPicaModelDraw);
        if(!count || bytes>256*1024-plan_bytes || !normal_reserve(bytes)){
            extern u32 __ctru_heap_size,__stacksize__;char line[200];
            snprintf(line,sizeof line,"Native actor plan rejected: asset=%u count=%u bytes=%u plan_bytes=%u scene_cpu=%u heap_used=%u heap=%u stack=%u\n",
                r->asset.id,count,bytes,plan_bytes,fh_native_scene_cpu_bytes,(unsigned)mallinfo().uordblks,(unsigned)__ctru_heap_size,(unsigned)__stacksize__);
            fh_platform_log(line);plan_rejected++;return;
        }
        FhPicaModelPlan *p=malloc(bytes);if(!p){plan_rejected++;return;}
        if(fh_pica_model_compile(&r->asset,p->draw,count)!=count){free(p);plan_rejected++;return;}
        memset(p,0,sizeof(*p));p->count=count;r->actor_plan=p;r->actor_plan_bytes=bytes;
        fh_native_scene_cpu_bytes+=bytes;plan_bytes+=bytes;if(plan_bytes>plan_peak)plan_peak=plan_bytes;
        plan_builds++;return;
    }
    if(!r->asset.block || !fh_minimal_enabled)return;
    if(fh_pica_scene_enabled){
        unsigned count=fh_pica_world_compile(&r->asset,NULL,0);
        /* Ordinary camera culling retains only CPU draw bounds; collision shadows
         * own their separate native point plan. No visible-mesh shadow GPU buffer. */
        unsigned bytes=sizeof(FhPicaWorldPlan)+count*(sizeof(FhPicaWorldDraw)+sizeof(FhPicaShadowReceiver));
        unsigned main_draws=1;
        if(r->asset.metadata_bytes>=84)memcpy(&main_draws,r->asset.meta+80,4);
        if((!count && main_draws) || bytes>256*1024-plan_bytes || !normal_reserve(bytes)){plan_rejected++;return;}
        FhPicaWorldPlan *plan=malloc(bytes);if(!plan){plan_rejected++;return;}
        if(fh_pica_world_compile(&r->asset,plan->draw,count)!=count){free(plan);plan_rejected++;return;}
        memset(plan,0,sizeof(*plan));plan->count=count;
        if(fh_pica_shadow_receivers_compile(&r->asset,plan,(FhPicaShadowReceiver*)(plan->draw+count),count)!=count){free(plan);plan_rejected++;return;}
        r->pica_plan=plan;r->world_plan_bytes=bytes;
        fh_native_scene_cpu_bytes+=bytes;plan_bytes+=bytes;
        if(plan_bytes>plan_peak)plan_peak=plan_bytes;
        plan_builds++;return;
    }
    SceneSpan span=r->spans[4];const void *source=r->snapshots+span.at;
    unsigned commands=fh_world_plan_compile(&r->asset,source,span.bytes,NULL,0,NULL,0);
    if(!commands){plan_rejected++;return;}
    unsigned draws=word(r->asset.meta+80);
    unsigned bytes=sizeof(FhWorldDrawPlan)+commands*sizeof(FhWorldPlanOp)+draws*sizeof(FhSceneGroup);
    if(bytes>256*1024-plan_bytes || !normal_reserve(bytes)){plan_rejected++;return;}
    FhWorldDrawPlan *plan=malloc(bytes);if(!plan){plan_rejected++;return;}
    FhWorldPlanOp *ops=(FhWorldPlanOp*)(plan+1);
    FhSceneGroup *groups=(FhSceneGroup*)(ops+commands);
    if(fh_world_plan_compile(&r->asset,source,span.bytes,ops,commands,groups,draws)!=commands){
        free(plan);plan_rejected++;return;
    }
    *plan=(FhWorldDrawPlan){commands,draws,ops,groups};
    r->world_plan=plan;r->world_plan_bytes=bytes;fh_native_scene_cpu_bytes+=bytes;
    plan_bytes+=bytes;if(plan_bytes>plan_peak)plan_peak=plan_bytes;plan_builds++;
}
static void refresh_world_bounds(SceneRecord *r){
    const FhSceneAsset *a=&r->asset;if(!r->world_bounds)return;
    unsigned *offsets=r->world_bounds,used=0,coarse_used=0;
    FhWorldBounds *bounds=(FhWorldBounds*)(offsets+a->groups*2);
    FhWorldBounds *parents=bounds+r->world_fine_count;
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;fh_native_scene_group(a,i,&g);offsets[i*2]=used;offsets[i*2+1]=coarse_used;
        unsigned start=used;
        for(unsigned at=0;at<g.indices;at+=FH_WORLD_CHUNK_INDICES){
            unsigned n=g.indices-at;if(n>FH_WORLD_CHUNK_INDICES)n=FH_WORLD_CHUNK_INDICES;
            fh_world_bounds_build(bounds+used++,g.vertex,g.index+at,n,a->compact,g.format);
        }
        for(unsigned at=start;at<used;at+=FH_WORLD_COARSE_CHUNKS){
            unsigned n=used-at;if(n>FH_WORLD_COARSE_CHUNKS)n=FH_WORLD_COARSE_CHUNKS;
            fh_world_parent_build(parents+coarse_used++,bounds+at,n);
        }
    }
}
static void prepare_world_bounds(SceneRecord *r){
    const FhSceneAsset *a=&r->asset;
    if(!a->block || !a->direct || !fh_minimal_enabled)return;
    unsigned chunks=0,parents=0;
    for(unsigned i=0;i<a->groups;i++){
        FhSceneGroup g;fh_native_scene_group(a,i,&g);
        unsigned n=(g.indices+FH_WORLD_CHUNK_INDICES-1)/FH_WORLD_CHUNK_INDICES;
        chunks+=n;parents+=(n+FH_WORLD_COARSE_CHUNKS-1)/FH_WORLD_COARSE_CHUNKS;
    }
    unsigned bytes=a->groups*2*sizeof(unsigned)+(chunks+parents)*sizeof(FhWorldBounds);
    if(!normal_reserve(bytes))return;
    unsigned *offsets=malloc(bytes);if(!offsets)return;
    r->world_bounds=offsets;r->world_bounds_bytes=bytes;r->world_fine_count=chunks;fh_native_scene_cpu_bytes+=bytes;
    world_hierarchy_bytes+=a->groups*sizeof(unsigned)+parents*sizeof(FhWorldBounds);
    if(world_hierarchy_bytes>world_hierarchy_peak)world_hierarchy_peak=world_hierarchy_bytes;
    refresh_world_bounds(r);
}
static int read_gpu(void *out,unsigned bytes,unsigned *crc){
    /* Same staging contract as prepared textures. Never give stdio GPU memory. */
    unsigned char chunk[FH_ASSET_STAGE_BYTES];unsigned at=0;
    while(at<bytes){unsigned n=bytes-at;if(n>sizeof(chunk))n=sizeof(chunk);
        if(scene_read(chunk,n)!=n)return 0;
        *crc=crc32(*crc,chunk,n);memcpy((unsigned char*)out+at,chunk,n);at+=n;
    }
    return 1;
}
static void initialize(void){
    initialized=1;
    FILE *bounds_config=fopen("sdmc:/3ds/foxhollow/model_bounds.txt","r");
    if(bounds_config){unsigned mode;if(fscanf(bounds_config,"%u",&mode)==1 && mode<=1)model_bounds_enabled=mode;fclose(bounds_config);}
    FILE *view_config=fopen("sdmc:/3ds/foxhollow/model_views.txt","r");
    if(view_config){unsigned mode;if(fscanf(view_config,"%u",&mode)==1 && mode<=1)model_views_mode=mode;fclose(view_config);}
    file=fh_asset_open_staged(fh_asset_path(FH_ASSET_SCENE));if(!file)return;
    fh_sd_reader_open(&scene_reader,fh_asset_path(FH_ASSET_SCENE));
    unsigned char header[16];long n;
    if(fseek(file,0,SEEK_END) || (n=ftell(file))<16 || n>SCENE_FILE_LIMIT || fseek(file,0,SEEK_SET) ||
       fread(header,1,16,file)!=16 || memcmp(header,"FHNSSET1",8) || !(entries=word(header+8)) ||
       entries>4096 || entries*32>(unsigned)n-16 || !normal_reserve(entries*32))goto bad;
    directory=malloc(entries*32);if(!directory || fread(directory,32,entries,file)!=entries ||
       (unsigned)crc32(0,directory,entries*32)!=word(header+12))goto bad;
    unsigned end=16+entries*32;
    for(unsigned i=0;i<entries;i++){
        const unsigned char *e=directory+i*32;unsigned at=word(e+12),bytes=word(e+16),kind=word(e+24);
        if(at!=end || at>(unsigned)n || bytes>(unsigned)n-at || word(e+28) || kind>1 ||
           (!kind && (bytes<128 || bytes>SCENE_GPU_LIMIT+256*1024)) ||
           (kind && (word(e)>=4 || word(e+4)!=512 || word(e+8)!=512 || bytes!=512*512*2)))goto bad;
        for(unsigned j=0;j<i;j++){
            const unsigned char *p=directory+j*32;
            if(word(p)==word(e) && word(p+4)==word(e+4) && word(p+8)==word(e+8) && word(p+24)==kind)goto bad;
        }
        end=(at+bytes+3)&~3u;
    }
    if(end!=(unsigned)n)goto bad;
    file_bytes=(unsigned)n;fh_native_scene_cpu_bytes=entries*32;fh_native_scene_enabled=1;
    if(fh_hardware_large_memory){
    /* Spend startup time once, keeping the original pack as a safe fallback.
     * Compression preserves all geometry/UV/color bytes. The cache uses normal
     * RAM only and remains subject to the shared 15% normal-heap headroom. */
    extern u32 __ctru_heap_size,__stacksize__;
    u64 ceiling=(u64)__ctru_heap_size*85/100;
    u64 used=(unsigned)mallinfo().uordblks+(u64)__stacksize__+4096;
    unsigned budget=ceiling>used?(unsigned)(ceiling-used):0;
    if(budget>7*1024*1024)budget=7*1024*1024;
    u64 cache_start=svcGetSystemTick();
    const char *cache_path=fh_asset_path(FH_ASSET_CACHE);
    if(fh_block_cache_open(&scene_cache,cache_path,budget,file_bytes) ||
       fh_block_cache_open_window(&scene_cache,cache_path,budget,file_bytes)){
        unsigned char check[4096];int same=fh_block_cache_read(&scene_cache,0,check,16) && !memcmp(check,header,16);
        for(unsigned at=0;same && at<entries*32;){
            unsigned count=entries*32-at;if(count>sizeof(check))count=sizeof(check);
            same=fh_block_cache_read(&scene_cache,16+at,check,count) && !memcmp(check,directory+at,count);at+=count;
        }
        if(!same)fh_block_cache_close(&scene_cache);
    }
    cache_load_ticks=svcGetSystemTick()-cache_start;
    char cache_message[128];snprintf(cache_message,sizeof(cache_message),
        "Native scene RAM cache: allocation=%u original=%u startup_verified=%u windowed=%u\n",
        scene_cache.storage_bytes,file_bytes,scene_cache.storage!=NULL,scene_cache.backing!=NULL);fh_platform_log(cache_message);
    }
    fh_platform_log("Native scene package opened; live game admission required\n");return;
bad:
    free(directory);directory=NULL;fh_sd_reader_close(&scene_reader);fclose(file);file=NULL;entries=0;fh_native_scene_rejected++;
    fh_platform_log("Native scene directory rejected; original rendering retained\n");
}
static int page(unsigned id){
    if(fh_renderer_scene_page(id))return 1;
    for(unsigned i=0;i<entries;i++){
        const unsigned char *e=directory+i*32;
        if(word(e)!=id || word(e+24)!=1)continue;
        void *gpu=fh_renderer_scene_page_allocate(id);if(!gpu)return 0;
        unsigned crc=0;
        if(scene_seek(word(e+12)) || !read_gpu(gpu,word(e+16),&crc) || crc!=word(e+20)){
            fh_renderer_scene_page_cancel(id);return 0;
        }
        fh_renderer_scene_page_commit(id);return 1;
    }
    return 0;
}
#include "platform_scene_world_deferred.h"
#include "platform_live_scene.h"
static unsigned live_admitted,live_failed;
static int live_admit(const void *owner,const void *raw,unsigned bytes,unsigned id,unsigned slot){
    if(!fh_pica_scene_enabled || (id>>31) || bytes>16384)return 0;
    FhInstallScene built={0};
    if(!fh_live_scene(&built,raw,bytes,id) || !built.bytes || built.metadata_bytes<128 || built.bytes<=built.metadata_bytes){
        if(built.error[0]){char why[224];snprintf(why,sizeof(why),"Live scene declined: id=%u bytes=%u reason=%s\n",
            id,bytes,built.error);fh_platform_log(why);}
        fh_install_scene_close(&built);live_failed++;return 0;
    }
    unsigned meta_bytes=built.metadata_bytes,gpu_bytes=built.bytes-built.metadata_bytes;SceneSpan spans[262];
    unsigned span_count=source_spans(spans,raw,bytes,0),snapshot_bytes=span_count*(sizeof(SceneSpan)+sizeof(SceneVersion));
    for(unsigned j=0;j<span_count;j++)snapshot_bytes+=fh_scene_snapshot_space(spans[j].bytes);
    if(!span_count || gpu_bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes ||
       meta_bytes+snapshot_bytes>SCENE_CPU_LIMIT-fh_native_scene_cpu_bytes ||
       (!fh_hardware_large_memory && fh_minimal_enabled && !fh_minimal_linear_room(gpu_bytes)) ||
       linearSpaceFree()<gpu_bytes+SCENE_LINEAR_RESERVE){
        char why[160];snprintf(why,sizeof(why),"Live scene declined: id=%u budget spans=%u gpu=%u meta=%u\n",id,span_count,gpu_bytes,meta_bytes+snapshot_bytes);fh_platform_log(why);
        fh_install_scene_close(&built);live_failed++;return 0;}
    unsigned meta_linear=0;
    void *meta=scene_meta_allocate(meta_bytes+snapshot_bytes,gpu_bytes,&meta_linear),*gpu=linearMemAlign(gpu_bytes,128);FhSceneAsset asset;
    if(!meta || !gpu || (meta_linear && !scene_meta_room(0))){
        scene_meta_free(meta,meta_bytes+snapshot_bytes,meta_linear);linearFree(gpu);fh_install_scene_close(&built);live_failed++;return 0;
    }
    memcpy(meta,built.data,meta_bytes);memcpy(gpu,built.data+meta_bytes,gpu_bytes);fh_install_scene_close(&built);
    int ok=fh_native_scene_open(&asset,meta,meta_bytes,gpu,gpu_bytes,id,bytes,(unsigned)crc32(0,raw,bytes));
    for(unsigned j=0;ok && !asset.direct && j<asset.groups;j++){FhSceneGroup g;fh_native_scene_group(&asset,j,&g);ok=page(g.page);}
    if(!ok){scene_meta_free(meta,meta_bytes+snapshot_bytes,meta_linear);linearFree(gpu);live_failed++;return 0;}
        /* Publish matching bone palettes and vertex slot indices together,
         * before any material/pose/morph/fur/shadow plan or GPU reader exists. */
        extern unsigned fh_native_scene_schedule_palettes(FhSceneAsset*);
        fh_native_scene_schedule_palettes(&asset);
        if(fh_minimal_enabled)fh_gpu_cache_flush(gpu,gpu_bytes);
        else GSPGPU_FlushDataCache(gpu,gpu_bytes);
        unsigned char *snapshots=(unsigned char*)meta+meta_bytes;SceneSpan *saved=(SceneSpan*)snapshots;
        SceneVersion *versions=(SceneVersion*)(saved+span_count);
        memset(versions,0,span_count*sizeof(*versions));
        unsigned cursor=span_count*(sizeof(SceneSpan)+sizeof(SceneVersion));
        for(unsigned j=0;j<span_count;j++){
            saved[j]=(SceneSpan){cursor,spans[j].bytes};memcpy(snapshots+cursor,(const unsigned char*)raw+spans[j].at,spans[j].bytes);
            if((id>>31) && j<4)for(unsigned k=0;k<spans[j].bytes;k+=2){unsigned char t=snapshots[cursor+k];snapshots[cursor+k]=snapshots[cursor+k+1];snapshots[cursor+k+1]=t;}
            cursor+=fh_scene_snapshot_space(spans[j].bytes);
        }
        scene_meta_linear[slot]=(unsigned char)meta_linear;
        records[slot]=(SceneRecord){.owner=owner,.meta=meta,.gpu=gpu,.asset=asset,.snapshots=snapshots,
            .snapshot_bytes=snapshot_bytes,.span_count=span_count,.spans=saved,.versions=versions,.gpu_used_bytes=gpu_bytes};
#ifdef FH_EMULATOR_TEST
        refresh_profile[slot]=(RefreshProfile){0};
#endif
        fh_native_scene_cpu_bytes+=meta_bytes+snapshot_bytes;fh_native_scene_gpu_bytes+=gpu_bytes;fh_native_scene_loaded++;
        prepare_world_bounds(records+slot);
        prepare_world_plan(records+slot);
        prepare_world_deferred(records+slot,raw,bytes);
        unsigned shadows=projected_shadows_wanted();
        if(shadows)prepare_shadow_track(records+slot,raw,bytes);
        if(!asset.block)prepare_fur_plan(records+slot,raw,bytes);
        if(!asset.block && shadows)prepare_shadow_plan(records+slot,raw);
        char message[160];snprintf(message,sizeof(message),"Live scene asset id=%u groups=%u draws=%u cpu=%u gpu=%u\n",
            id,asset.groups,asset.draws,meta_bytes+snapshot_bytes,gpu_bytes);fh_platform_log(message);
    live_admitted++;return 1;
}
void fh_native_scene_register(const void *owner,const void *raw,unsigned bytes,unsigned id){
    if(!owner || !raw || bytes<164 || bytes>4*1024*1024 || !fh_renderer_ready())return;
    if(!initialized)initialize();
    if(!file)return;
    unsigned slot=SCENE_RECORDS;
    for(unsigned i=0;i<SCENE_RECORDS;i++){if(records[i].owner==owner)return;if(!records[i].owner && slot==SCENE_RECORDS)slot=i;}
    if(slot==SCENE_RECORDS)return;
    unsigned crc=0,hashed=0;
    for(unsigned i=0;i<entries;i++){
        const unsigned char *e=directory+i*32;
        if(word(e)!=id || word(e+4)!=bytes || word(e+24))continue;
        if(!hashed){crc=crc32(0,raw,bytes);hashed=1;}
        if(word(e+8)!=crc)continue;
        unsigned char header[96];
        scene_hint_end=word(e+12)+word(e+16);
        if(scene_seek(word(e+12)) || scene_read(header,96)!=96)break;
        /* The legacy float renderer must not interpret packed world bytes. */
        if(header[7]=='7' && !fh_minimal_enabled)break;
        if((header[7]=='A' || header[7]=='B') && !fh_pica_scene_enabled)break;
        unsigned meta_bytes=word(header+68),gpu_bytes=word(header+72);SceneSpan spans[262];
        unsigned span_count=source_spans(spans,raw,bytes,id>>31),snapshot_bytes=span_count*(sizeof(SceneSpan)+sizeof(SceneVersion));
        if(!span_count)break;
        for(unsigned j=0;j<span_count;j++)snapshot_bytes+=fh_scene_snapshot_space(spans[j].bytes);
        shadow_plans_yield(gpu_bytes);
        if(meta_bytes<128 || meta_bytes>256*1024 || gpu_bytes>SCENE_GPU_LIMIT-fh_native_scene_gpu_bytes ||
            meta_bytes+gpu_bytes!=word(e+16) || meta_bytes+snapshot_bytes>SCENE_CPU_LIMIT-fh_native_scene_cpu_bytes)break;
        if(!fh_hardware_large_memory && fh_minimal_enabled && !fh_minimal_linear_room(gpu_bytes))break;
        if(linearSpaceFree()<gpu_bytes+SCENE_LINEAR_RESERVE)fh_minimal_texture_cache_trim(gpu_bytes+SCENE_LINEAR_RESERVE);
        if(linearSpaceFree()<gpu_bytes+SCENE_LINEAR_RESERVE)break;
        unsigned meta_linear=0;
        void *meta=scene_meta_allocate(meta_bytes+snapshot_bytes,gpu_bytes,&meta_linear),*gpu=linearMemAlign(gpu_bytes,128);FhSceneAsset asset;
        if(!meta || !gpu || (meta_linear && !scene_meta_room(0))){scene_meta_free(meta,meta_bytes+snapshot_bytes,meta_linear);linearFree(gpu);break;}
        memcpy(meta,header,96);unsigned sum;
        int ok=scene_read((unsigned char*)meta+96,meta_bytes-96)==meta_bytes-96;
        sum=crc32(0,meta,meta_bytes);
        ok=ok && read_gpu(gpu,gpu_bytes,&sum) && sum==word(e+20) &&
            fh_native_scene_open(&asset,meta,meta_bytes,gpu,gpu_bytes,id,bytes,crc);
        for(unsigned j=0;ok && !asset.direct && j<asset.groups;j++){FhSceneGroup g;fh_native_scene_group(&asset,j,&g);ok=page(g.page);}
        if(!ok){scene_meta_free(meta,meta_bytes+snapshot_bytes,meta_linear);linearFree(gpu);break;}
        /* Publish matching bone palettes and vertex slot indices together,
         * before any material/pose/morph/fur/shadow plan or GPU reader exists. */
        extern unsigned fh_native_scene_schedule_palettes(FhSceneAsset*);
        fh_native_scene_schedule_palettes(&asset);
        if(fh_minimal_enabled)fh_gpu_cache_flush(gpu,gpu_bytes);
        else GSPGPU_FlushDataCache(gpu,gpu_bytes);
        unsigned char *snapshots=(unsigned char*)meta+meta_bytes;SceneSpan *saved=(SceneSpan*)snapshots;
        SceneVersion *versions=(SceneVersion*)(saved+span_count);
        memset(versions,0,span_count*sizeof(*versions));
        unsigned cursor=span_count*(sizeof(SceneSpan)+sizeof(SceneVersion));
        for(unsigned j=0;j<span_count;j++){
            saved[j]=(SceneSpan){cursor,spans[j].bytes};memcpy(snapshots+cursor,(const unsigned char*)raw+spans[j].at,spans[j].bytes);
            if((id>>31) && j<4)for(unsigned k=0;k<spans[j].bytes;k+=2){unsigned char t=snapshots[cursor+k];snapshots[cursor+k]=snapshots[cursor+k+1];snapshots[cursor+k+1]=t;}
            cursor+=fh_scene_snapshot_space(spans[j].bytes);
        }
        scene_meta_linear[slot]=(unsigned char)meta_linear;
        records[slot]=(SceneRecord){.owner=owner,.meta=meta,.gpu=gpu,.asset=asset,.snapshots=snapshots,
            .snapshot_bytes=snapshot_bytes,.span_count=span_count,.spans=saved,.versions=versions,.gpu_used_bytes=gpu_bytes};
#ifdef FH_EMULATOR_TEST
        refresh_profile[slot]=(RefreshProfile){0};
#endif
        fh_native_scene_cpu_bytes+=meta_bytes+snapshot_bytes;fh_native_scene_gpu_bytes+=gpu_bytes;fh_native_scene_loaded++;
        prepare_world_bounds(records+slot);
        prepare_world_plan(records+slot);
        prepare_world_deferred(records+slot,raw,bytes);
        unsigned shadows=projected_shadows_wanted();
        if(shadows)prepare_shadow_track(records+slot,raw,bytes);
        if(!asset.block)prepare_fur_plan(records+slot,raw,bytes);
        if(!asset.block && shadows)prepare_shadow_plan(records+slot,raw);
        char message[160];snprintf(message,sizeof(message),"Scene asset id=%u groups=%u draws=%u cpu=%u gpu=%u\n",
            id,asset.groups,asset.draws,meta_bytes+snapshot_bytes,gpu_bytes);fh_platform_log(message);return;
    }
    if(live_admit(owner,raw,bytes,id,slot))return;
    if(fh_pica_scene_enabled && !(id>>31)){
        /* Loaded resources need not be drawn, but retain their exact source
         * identity so absent bank variants can be prepared without guessing.
         * This executes once at admission, never in frame submission. */
        char message[160];
        snprintf(message,sizeof(message),"PICA actor source not admitted: id=%u bytes=%u crc=%08x size_matched=%u\n",
            id,bytes,hashed?crc:(unsigned)crc32(0,raw,bytes),hashed);
        fh_platform_log(message);
    }
    if(hashed)fh_native_scene_rejected++;
}
const FhSceneAsset *fh_native_scene_find(const void *owner){
    /* This lookup intentionally does not claim source/callback validity. */
    if(!owner)return NULL;
    for(unsigned i=0;i<SCENE_RECORDS;i++)if(records[i].owner==owner && !records[i].invalid)return &records[i].asset;
    return NULL;
}
static SceneRecord *record(const FhSceneAsset *a){
    /* Handles are embedded in this fixed directory, not arbitrary owners.
     * Validate the exact slot before dereferencing; no per-draw 256-slot scan. */
    uintptr_t at=(uintptr_t)a,first=(uintptr_t)&records[0].asset;
    if(at<first || at-first>=sizeof(records) || (at-first)%sizeof(SceneRecord))return NULL;
    SceneRecord *r=records+(at-first)/sizeof(SceneRecord);
    return r->owner?r:NULL;
}
#include "platform_scene_actor_fog.h"
#include "platform_scene_actor_fog_archive.h"
const FhPicaShadowPlan *fh_native_scene_shadow_plan(const FhSceneAsset *a){
    SceneRecord *r=record(a);
    return r && !r->invalid && r->shadow_valid && !r->active && a->gpu==r->gpu?r->shadow_plan:NULL;
}
const FhPicaWorldDeferredPlan *fh_native_scene_deferred_plan(const FhSceneAsset *a){
    SceneRecord *r=record(a);return r && !r->invalid?r->deferred:NULL;
}
const FhPicaWorldPlan *fh_native_scene_pica_plan(const FhSceneAsset *a){
    SceneRecord *r=record(a);
    return fh_pica_scene_enabled && r && !r->invalid?r->pica_plan:NULL;
}
const FhWorldDrawPlan *fh_native_scene_world_plan(const FhSceneAsset *a){
    if(fh_pica_scene_enabled)return NULL;
    SceneRecord *r=record(a);
    if(!r || r->invalid || !r->world_plan)return NULL;
    /* Caller must validate live instructions and attributes before use. */
    plan_hits++;return r->world_plan;
}
const FhWorldBounds *fh_native_scene_world_bounds(const FhSceneAsset *a,
    const FhSceneGroup *part,unsigned *first,unsigned *total,const FhWorldBounds **parents){
    *parents=NULL;
    SceneRecord *r=record(a);if(!r || r->invalid || !r->world_bounds || !part)return NULL;
    const unsigned *offsets=r->world_bounds;
    const FhWorldBounds *bounds=(const FhWorldBounds*)(offsets+a->groups*2);
    /* The first original draw already identifies its compiled group. Avoid
     * decoding every earlier group for each split/material subsection. */
    if(part->first<a->draws){
        const unsigned char *draw=a->meta+a->draws_at+part->first*8;
        unsigned i=(unsigned)draw[4]|(unsigned)draw[5]<<8;
        if(i>=a->groups)return NULL;
        FhSceneGroup g;fh_native_scene_group(a,i,&g);
        uintptr_t address=(uintptr_t)part->index,base=(uintptr_t)g.index;
        if(g.vertex!=part->vertex || address<base || (address-base)%2 || (address-base)/2>g.indices)return NULL;
        unsigned at=(unsigned)((address-base)/2);
        if(part->indices>g.indices-at || at%3)return NULL;
        *first=at;*total=g.indices;*parents=bounds+r->world_fine_count+offsets[i*2+1];return bounds+offsets[i*2];
    }
    return NULL;
}
int fh_native_scene_source(const FhSceneAsset *a,unsigned index,const void *data,unsigned bytes){
    SceneRecord *r=record(a);if(!r || r->invalid || index>=r->span_count || bytes!=r->spans[index].bytes || (bytes && !data))return 0;
    SceneVersion *v=(index<4?model_view_versions(r):r->versions)+index;
    SceneSpan *span=(index<4?model_view_spans(r):r->spans)+index;
    unsigned char *snapshots=index<4?model_view_snapshots(r):r->snapshots;
    if(v->data!=data){
        span->at=fh_scene_snapshot_align(snapshots,span->at,data,bytes);
        v->data=data;v->version=0;
    }
    /* Reuse the audited producer write/escape directory. Unknown or escaped
     * sources still get full byte comparisons; mode 2 verifies every reuse.
     * This does not infer immutability from an unchanged pointer. */
    u64 start=svcGetSystemTick();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    unsigned replay_phase=fh_replay_phase_at(FH_PHASE_SCENE_SOURCE,start);
#endif
    int same=!bytes || fh_model_resource_match(&v->version,data,snapshots+span->at,bytes);
    u64 end=svcGetSystemTick();
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase_at(replay_phase,end);
#endif
    if(!same)r->actor_fog_dirty=1; /* Never reuse immutable fog data after a source change. */
    if(index==0 && !same)r->shadow_valid=0;
    scene_source_ticks+=end-start;scene_source_bytes+=bytes;return same;
}
const FhAnimatedBounds *fh_native_scene_model_bounds(const FhSceneAsset *a,const FhSceneGroup *part){
    if(!model_bounds_enabled)return NULL;
    SceneRecord *r=record(a);
    if(!r || r->invalid || !a->compact || a->block || !part)return NULL;
    uintptr_t base=(uintptr_t)a->meta+a->groups_at+48,ptr=(uintptr_t)part->bone_ids;
    if(ptr<base || (ptr-base)%96 || (ptr-base)/96>=a->groups)return NULL;
    unsigned index=(unsigned)((ptr-base)/96);FhSceneGroup full;
    if(!fh_native_scene_group(a,index,&full) || part->vertex!=full.vertex ||
       part->vertices!=full.vertices || part->bones!=full.bones || part->format!=full.format)return NULL;
    FhAnimatedBounds **slot=r->active?&r->active->model_bounds:&r->model_bounds;
    if(!*slot){
        if(a->groups>SCENE_CPU_LIMIT/sizeof(FhAnimatedBounds))return NULL;
        unsigned bytes=a->groups*sizeof(FhAnimatedBounds);
        if(!normal_reserve(bytes))return NULL;
        *slot=calloc(a->groups,sizeof(FhAnimatedBounds));if(!*slot)return NULL;
        if(r->active)r->active->allocation_bytes+=bytes;else r->model_bounds_bytes=bytes;
        fh_native_scene_cpu_bytes+=bytes;model_bounds_bytes+=bytes;
        if(model_bounds_bytes>model_bounds_peak)model_bounds_peak=model_bounds_bytes;
    }
    FhAnimatedBounds *bound=*slot+index;
    if(!bound->used && !fh_animated_bounds_build(bound,(const FhModelCompactVertex*)full.vertex,
        full.vertices,full.bones,full.format))return NULL;
    return bound;
}
int fh_native_scene_prepare_model(const FhSceneAsset *a,const FhModelSource *source){
    FH_REPLAY_SCOPE(MODEL_REFRESH);
    SceneRecord *r=record(a);if(!r || r->invalid || a->block || !source)return 0;
    const void *data[4]={source->positions,source->normals,source->colors,source->uvs};
    unsigned bytes[4]={source->position_count*6,source->normal_count*source->normal_stride,source->color_count*2,source->uv_count*4};
    unsigned changed=0;
    for(unsigned i=0;i<4;i++)if(bytes[i]!=r->spans[i].bytes || (bytes[i] && !data[i]))return 0;
#ifdef FH_EMULATOR_TEST
    RefreshProfile *profile=refresh_profile+(r-records);profile->id=a->id;profile->calls++;
    SceneVersion *prior=model_view_versions(r);
    unsigned switched=(prior[0].data && prior[0].data!=data[0]) ||
        (prior[1].data && prior[1].data!=data[1]);
#endif
    model_view_select(r,data);
    for(unsigned i=0;i<4;i++){
        if(!fh_native_scene_source(a,i,data[i],bytes[i]))changed|=1u<<i;
    }
    if((changed&8) && !a->direct)return 0; /* Atlas domains cannot be changed in place. */
    unsigned compact=fh_minimal_enabled && a->direct && !a->compact;
    if(!changed && !compact){
        if(r->active && r->active->unpublished)model_view_publish(r);
        return 1;
    }
#ifdef FH_EMULATOR_TEST
    profile->changes++;profile->switches+=switched;profile->masks[changed]++;
    profile->bytes+=r->gpu_used_bytes;if(switched)profile->switched_bytes+=r->gpu_used_bytes;
#endif
    fh_native_scene_admission[14]++;
    fh_renderer_scene_upload_begin(*model_view_last_used(r));
    FhModelVertex scratch[64];FhModelSource s=*source;
    s.bindings=(const FhModelBinding*)(a->meta+a->bindings_at);s.binding_count=a->bindings;
    s.tails=(const float(*)[3])(a->meta+a->tails_at);s.bone_count=a->bones;
    if(compact){
        r->gpu_used_bytes=fh_native_scene_compact_model(&r->asset,&s);
        if(!r->gpu_used_bytes){r->invalid=1;return 0;}
    }
    for(unsigned i=0;!compact && i<a->groups;i++){
        FhSceneGroup g;fh_native_scene_group(a,i,&g);s.format=g.format;s.secondary_uv=g.two_uv;
        if(a->compact){
            if(!fh_native_model_refresh_compact((FhModelCompactVertex*)g.vertex,g.vertices,g.refs,g.vertices,&s,changed)){
                r->invalid=1;return 0;
            }
            continue;
        }
        if(a->direct){
            /* Only live source arrays changed. Packet topology, bind tails
             * and skin attributes remain the validated private originals. */
            if(!fh_native_model_refresh((FhModelVertex*)g.vertex,g.vertices,g.refs,g.vertices,
                &s,changed)){r->invalid=1;return 0;}
            continue;
        }
        for(unsigned at=0;at<g.vertices;at+=64){
            unsigned n=g.vertices-at;if(n>64)n=64;
            if(!fh_native_model_vertices(scratch,n,g.refs+at,n,&s,g.bone_ids,g.bones)){r->invalid=1;return 0;}
            FhModelVertex *target=(FhModelVertex*)g.vertex+at;
            if(!a->direct)for(unsigned j=0;j<n;j++){scratch[j].uv[0]=target[j].uv[0];scratch[j].uv[1]=target[j].uv[1];}
            memcpy(target,scratch,n*sizeof(*scratch));
        }
    }
    if(compact || (changed&FH_MODEL_POSITION)){
        FhAnimatedBounds *bounds=r->active?r->active->model_bounds:r->model_bounds;
        if(bounds)for(unsigned i=0;i<a->groups;i++)bounds[i].used=0;
    }
    /* One publication for the whole owned record, including all repacked
     * indices. Avoid a kernel/service operation for every material group. */
    model_view_publish(r);
    SceneSpan *spans=model_view_spans(r);unsigned char *snapshots=model_view_snapshots(r);
    for(unsigned i=0;i<4;i++)if(changed&(1u<<i))memcpy(snapshots+spans[i].at,data[i],bytes[i]);
    return 1;
}
int fh_native_scene_prepare_world_positions(const FhSceneAsset *a,const void *positions,unsigned count){
    SceneRecord *r=record(a);
    if(!r || r->invalid || !a->block || !positions || count>65535 || count*6!=r->spans[0].bytes)return 0;
    if(fh_native_scene_source(a,0,positions,count*6)){r->track_dirty=0;return 1;}
    if(a->meta[7]!='8' && a->meta[7]!='9')return 0; /* Old packs cannot identify moved vertices. */
    /* Same Melee ownership rule as model refresh: join all previous readers
     * before changing persistent geometry; publish CPU writes before drawing. */
    fh_renderer_scene_upload_begin(r->last_used);
    if(!fh_native_scene_refresh_world_positions(a,positions,count))return 0;
    refresh_world_bounds(r);
    if(fh_pica_scene_enabled && r->pica_plan &&
       fh_pica_shadow_receivers_compile(a,r->pica_plan,(FhPicaShadowReceiver*)(r->pica_plan->draw+r->pica_plan->count),r->pica_plan->count)!=r->pica_plan->count)
        fh_platform_panic("PICA world visibility mutated bounds");
    refresh_shadow_track(r,positions,count);
    fh_gpu_cache_flush(r->gpu,r->gpu_used_bytes);
    memcpy(r->snapshots+r->spans[0].at,positions,count*6);
    r->versions[0].version=0;
    world_position_refreshes++;world_position_bytes+=r->gpu_used_bytes;
    return 1;
}
static unsigned pica_position_attaches,pica_position_notifications,pica_position_updates,pica_position_reuses;
int fh_pica_world_attach_positions(const FhSceneAsset *a,const void *positions,unsigned count){
    FhPicaWorldPlan *p=(FhPicaWorldPlan*)fh_native_scene_pica_plan(a);
    if(!p || p->positions || !fh_native_scene_prepare_world_positions(a,positions,count))return 0;
    p->positions=positions;p->position_count=count;p->positions_dirty=0;pica_position_attaches++;return 1;
}
const FhPicaModelPlan *fh_native_scene_pica_model(const FhSceneAsset *a){
    SceneRecord *r=record(a);
    return fh_pica_scene_actors && r && !r->invalid?r->actor_plan:NULL;
}
void fh_native_scene_fur_groom_prepare(const FhSceneAsset*a,unsigned material,const FhTexture*t){prepare_fur_groom(record(a),material,t);}
const FhPicaFurPlan *fh_native_scene_fur_plan(const FhSceneAsset*a){
    SceneRecord*r=record(a);return r&&!r->invalid?r->fur_plan:NULL;
}
FhPicaMaterialSet *fh_native_scene_pica_materials(const FhSceneAsset *a){
    SceneRecord *r=record(a);if(!fh_pica_scene_enabled || !r || r->invalid)return NULL;
    if(a->block)return r->pica_plan?&r->pica_plan->materials:NULL;
    return r->actor_plan?&r->actor_plan->materials:NULL;
}
void fh_pica_world_positions_changed(const void *owner){
    extern void fh_native_ground_changed(void);
    fh_native_ground_changed(); /* Also publish collision vertices/bounds, even before attachment. */
    if(!fh_pica_scene_enabled)return;
    const FhSceneAsset *a=fh_native_scene_find(owner);
    SceneRecord*r=record(a);
    if(r && r->track_plan && !r->track_dirty){r->track_dirty=1;track_invalidated++;}
    FhPicaWorldPlan *p=(FhPicaWorldPlan*)fh_native_scene_pica_plan(a);
    /* Registration may fail independently; ordinary draw coverage rejects it.
     * A producer can run before attachment; attachment validates current data. */
    if(p){p->positions_dirty=1;pica_position_notifications++;}
}
int fh_pica_world_prepare_positions(const FhSceneAsset *a){
    FhPicaWorldPlan *p=(FhPicaWorldPlan*)fh_native_scene_pica_plan(a);
    if(!p || !p->positions)return 0;
    if(!p->positions_dirty){pica_position_reuses++;return 1;}
    if(!fh_native_scene_prepare_world_positions(a,p->positions,p->position_count))return 0;
    p->positions_dirty=0;pica_position_updates++;return 1;
}
void fh_native_scene_used(const FhSceneAsset *a){SceneRecord *r=record(a);if(r)*model_view_last_used(r)=fh_renderer_scene_frame();}
unsigned fh_native_scene_texture_crc(const void *data,unsigned bytes){
    for(unsigned i=0;i<64;i++)if(texture_identities[i].valid && texture_identities[i].data==data && texture_identities[i].bytes==bytes)return texture_identities[i].crc;
    u64 start=svcGetSystemTick();unsigned crc=crc32(0,data,bytes);
    scene_crc_ticks+=svcGetSystemTick()-start;scene_crc_bytes+=bytes;
    texture_identities[texture_identity_cursor++&63]=(FhSceneTextureIdentity){data,bytes,crc,1};return crc;
}
void fh_native_scene_texture_written(const void *address,size_t bytes){
    if(!fh_native_scene_enabled)return;
    fh_scene_texture_identity_invalidate(texture_identities,64,address,bytes);
}
void fh_native_scene_forget(const void *allocation,unsigned bytes){
    fh_pica_morph_forget(allocation,bytes);
    fh_minimal_texture_asset_forget(allocation,bytes);
    fh_native_scene_texture_written(allocation,bytes);
    uintptr_t base=(uintptr_t)allocation;unsigned joined=0;
    for(unsigned i=0;i<SCENE_RECORDS;i++){
        SceneRecord *r=records+i;uintptr_t owner=(uintptr_t)r->owner;
        if(!owner || owner<base || owner-base>=bytes)continue;
        if(!joined){fh_renderer_scene_storage_join();joined=1;}
        release_actor_fog(r);
        release_shadow_track(r);
        release_shadow_plan(r);
        release_fur_plan(r);
        model_views_release(r);
        if(r->actor_plan)fh_pica_materials_release(&r->actor_plan->materials);
        free(r->actor_plan);fh_native_scene_cpu_bytes-=r->actor_plan_bytes;plan_bytes-=r->actor_plan_bytes;
        if(r->world_bounds)world_hierarchy_bytes-=r->world_bounds_bytes-r->asset.groups*sizeof(unsigned)-r->world_fine_count*sizeof(FhWorldBounds);
        fh_native_scene_cpu_bytes-=r->asset.metadata_bytes+r->snapshot_bytes+r->world_bounds_bytes+r->world_plan_bytes;
        release_world_deferred(r);
        plan_bytes-=r->world_plan_bytes;if(fh_pica_scene_enabled && r->pica_plan)fh_pica_materials_release(&r->pica_plan->materials);free(r->world_plan);fh_native_scene_gpu_bytes-=r->asset.gpu_bytes;
        fh_pica_shadow_project_forget(&r->asset);
        free(r->world_bounds);scene_meta_release(r);linearFree(r->gpu);*r=(SceneRecord){0};
    }
}
void fh_native_scene_shutdown(void){
    {extern void fh_pica_world_slice_metrics(uint64_t[8]);uint64_t values[8];char line[320];fh_pica_world_slice_metrics(values);snprintf(line,sizeof line,"PICA deferred timing: ticks=%llu max_ticks=%llu calls=%llu active_frames=%llu max_frame_ticks=%llu max_frame=%llu first_frame=%llu last_frame=%llu hz=268111856\n",(unsigned long long)values[0],(unsigned long long)values[1],(unsigned long long)values[2],(unsigned long long)values[3],(unsigned long long)values[4],(unsigned long long)values[5],(unsigned long long)values[6],(unsigned long long)values[7]);fh_platform_log(line);}
    {extern void fh_pica_game_deferred_counts(unsigned[3]);
     extern unsigned fh_pica_world_slice_draw_count(void);
     unsigned counts[3];char line[192];fh_pica_game_deferred_counts(counts);
     snprintf(line,sizeof line,"PICA deferred surfaces: queued=%u dispatched=%u unsupported=%u gpu_draws=%u\n",counts[0],counts[1],counts[2],fh_pica_world_slice_draw_count());fh_platform_log(line);}

    {char line[192];snprintf(line,sizeof line,"PICA deferred plans: builds=%u rejected=%u cpu=%u peak=%u cap=65536\n",deferred_builds,deferred_rejected,deferred_bytes,deferred_peak);fh_platform_log(line);}
    {char line[160];snprintf(line,sizeof(line),"Native scene metadata placement: linear=%u peak=%u normal_reserve=15 linear_reserve=15\n",scene_meta_linear_bytes,scene_meta_linear_peak);fh_platform_log(line);}
    close_actor_fog_archive();
    {char line[200];snprintf(line,sizeof(line),"PICA collision admission: scratch_peak=%u stages=%u,%u,%u,%u,%u,%u,%u\n",track_scratch_peak,
        track_decline_stage[0],track_decline_stage[1],track_decline_stage[2],track_decline_stage[3],track_decline_stage[4],track_decline_stage[5],track_decline_stage[6]);fh_platform_log(line);}

    {char line[200];snprintf(line,sizeof(line),"PICA collision updates: calls=%u changed_triangles=%u flush_bytes=%llu cpu_cap=262144\n",track_updates,track_changed,track_flush_bytes);fh_platform_log(line);}
    {char line[240];snprintf(line,sizeof(line),"PICA collision receivers: active=%u builds=%u declined=%u invalidated=%u cpu=%u linear=%u peak=%u cap=1048576 scratch_cap=2097152\n",fh_pica_shadow_track_enabled,track_builds,track_declined,track_invalidated,track_cpu,track_gpu,track_peak);fh_platform_log(line);}
    {char line[192];snprintf(line,sizeof(line),"PICA fur groom: builds=%u declined=%u linear=%u peak=%u cpu=%u linear_cap=65536 cpu_cap=16384\n",groom_builds,groom_declined,groom_gpu,groom_peak,groom_cpu);fh_platform_log(line);}
    {char line[160];snprintf(line,sizeof(line),"PICA fur plans: builds=%u declined=%u cpu=%u peak=%u cap=32768\n",fur_plan_builds,fur_plan_declined,fur_plan_cpu,fur_plan_peak);fh_platform_log(line);}
    {char line[224];snprintf(line,sizeof(line),"PICA shadow plans: builds=%u declined=%u skipped_loads=%u cpu=%u linear=%u linear_peak=%u linear_cap=524288\n",shadow_plan_builds,shadow_plan_declined,projected_shadow_skips,shadow_plan_cpu,shadow_plan_gpu,shadow_plan_peak);fh_platform_log(line);}
    if(fh_pica_scene_enabled){char message[192];snprintf(message,sizeof(message),"PICA world positions: attached=%u notifications=%u updates=%u reuses=%u\n",pica_position_attaches,pica_position_notifications,pica_position_updates,pica_position_reuses);fh_platform_log(message);}
    {char line[192];snprintf(line,sizeof(line),"Native world draw plans: builds=%u rejected=%u hits=%u cpu=%u peak=%u cap=262144 runs=%u draws=%u\n",
        plan_builds,plan_rejected,plan_hits,plan_bytes,plan_peak,fh_world_plan_runs,fh_world_plan_draws);fh_platform_log(line);}
    {char line[128];snprintf(line,sizeof(line),"Native animated bounds: enabled=%u cpu_bytes=%u peak=%u\n",model_bounds_enabled,model_bounds_bytes,model_bounds_peak);fh_platform_log(line);}
    {char message[160];snprintf(message,sizeof(message),"Native world hierarchy: extra_cpu_bytes=%u peak=%u chunks_per_parent=%u\n",
        world_hierarchy_bytes,world_hierarchy_peak,FH_WORLD_COARSE_CHUNKS);fh_platform_log(message);}
    {char message[160];snprintf(message,sizeof(message),"Native world position refresh: updates=%u published_bytes=%llu\n",
        world_position_refreshes,(unsigned long long)world_position_bytes);fh_platform_log(message);}
    /* Renderer invokes this only after queued GPU work is drained. */
    if(initialized){char message[192];snprintf(message,sizeof(message),"Native scene storage: loaded=%u rejected=%u cpu=%u gpu=%u file=%u\n",
        fh_native_scene_loaded,fh_native_scene_rejected,fh_native_scene_cpu_bytes,fh_native_scene_gpu_bytes,file_bytes);fh_platform_log(message);}
    if(initialized){char message[320];unsigned at=snprintf(message,sizeof(message),"Native scene admission:");
        for(unsigned i=0;i<16;i++)at+=snprintf(message+at,sizeof(message)-at," %u",fh_native_scene_admission[i]);
        snprintf(message+at,sizeof(message)-at," refreshed_bytes=%llu\n",(unsigned long long)scene_uploaded_bytes);fh_platform_log(message);}
    if(initialized){extern unsigned fh_native_model_admission[16];char message[256];
        unsigned at=snprintf(message,sizeof(message),"Scene backend admission:");
        for(unsigned i=0;i<16;i++)at+=snprintf(message+at,sizeof(message)-at," %u",fh_native_model_admission[i]);
        snprintf(message+at,sizeof(message)-at,"\n");fh_platform_log(message);}
    if(initialized){char message[256];snprintf(message,sizeof(message),
        "Scene validation ticks: source=%llu crc=%llu source_bytes=%llu crc_bytes=%llu ticks_hz=268111856\n",
        (unsigned long long)scene_source_ticks,(unsigned long long)scene_crc_ticks,
        (unsigned long long)scene_source_bytes,(unsigned long long)scene_crc_bytes);fh_platform_log(message);}
#ifdef FH_EMULATOR_TEST
    for(unsigned i=0;i<SCENE_RECORDS;i++)if(refresh_profile[i].changes){
        RefreshProfile *p=refresh_profile+i;char message[384];
        unsigned at=snprintf(message,sizeof(message),"Scene refresh profile: id=%u calls=%u changes=%u switches=%u bytes=%llu switched_bytes=%llu masks=",
            p->id,p->calls,p->changes,p->switches,(unsigned long long)p->bytes,(unsigned long long)p->switched_bytes);
        for(unsigned j=0;j<16;j++)at+=snprintf(message+at,sizeof(message)-at,"%s%u",j?",":"",p->masks[j]);
        snprintf(message+at,sizeof(message)-at,"\n");fh_platform_log(message);
    }
#endif
    if(initialized){char message[256];snprintf(message,sizeof(message),
        "Native model views: mode=%u allocated=%u hits=%u fallback=%u gpu=%u peak=%u cap=%u\n",
        model_views_mode,model_view_allocations,model_view_hits,model_view_fallbacks,model_view_gpu_bytes,model_view_peak,MODEL_VIEW_GPU_LIMIT);fh_platform_log(message);}
    for(unsigned i=0;i<SCENE_RECORDS;i++){release_world_deferred(records+i);release_actor_fog(records+i);fh_pica_shadow_project_forget(&records[i].asset);release_shadow_track(records+i);release_fur_plan(records+i);release_shadow_plan(records+i);model_views_release(records+i);if(records[i].actor_plan)fh_pica_materials_release(&records[i].actor_plan->materials);free(records[i].actor_plan);if(fh_pica_scene_enabled && records[i].pica_plan)fh_pica_materials_release(&records[i].pica_plan->materials);free(records[i].world_plan);free(records[i].world_bounds);scene_meta_release(records+i);linearFree(records[i].gpu);records[i]=(SceneRecord){0};}
    if(scene_cache.storage){char message[192];snprintf(message,sizeof(message),
        "Native scene RAM cache: allocation=%u reads=%llu bytes=%llu decodes=%llu\n",
        scene_cache.storage_bytes,(unsigned long long)scene_cache.reads,
        (unsigned long long)scene_cache.bytes,(unsigned long long)scene_cache.decodes);fh_platform_log(message);}
    if(scene_cache.storage){char message[192];snprintf(message,sizeof(message),
        "Native scene window IO: active=%u reads=%llu bytes=%llu evictions=%llu\n",scene_cache.backing!=NULL,
        (unsigned long long)scene_cache.sd_reads,(unsigned long long)scene_cache.sd_bytes,(unsigned long long)scene_cache.evictions);fh_platform_log(message);}
    {char message[256];snprintf(message,sizeof(message),
        "Native scene IO: startup_ticks=%llu cache_ticks=%llu sd_ticks=%llu sd_reads=%u sd_bytes=%llu pressure_releases=%u hz=268111856\n",
        (unsigned long long)cache_load_ticks,(unsigned long long)cache_read_ticks,(unsigned long long)scene_sd_ticks,
        scene_sd_reads,(unsigned long long)scene_sd_bytes,cache_releases);fh_platform_log(message);}
    fh_block_cache_close(&scene_cache);scene_cursor=0;
    free(directory);directory=NULL;fh_sd_reader_close(&scene_reader);if(file)fclose(file);file=NULL;
    initialized=entries=file_bytes=fh_native_scene_cpu_bytes=fh_native_scene_gpu_bytes=fh_native_scene_enabled=0;
    memset(texture_identities,0,sizeof(texture_identities));texture_identity_cursor=0;
    plan_bytes=plan_peak=plan_builds=plan_rejected=plan_hits=0;
}




const FhShadowPointPlan *fh_native_scene_shadow_points(const FhSceneAsset*a){
    SceneRecord*r=record(a);
    return fh_pica_shadow_track_enabled && r && !r->invalid && !r->track_dirty?r->track_plan:NULL;
}

/* Bounds owner for ordinary world visibility only. */
const FhPicaShadowReceiver *fh_pica_shadow_receivers_for_asset(const FhSceneAsset*a){
    SceneRecord*r=record(a);
    return fh_pica_scene_enabled && a && a->block && r && !r->invalid && r->pica_plan?
        (const FhPicaShadowReceiver*)(r->pica_plan->draw+r->pica_plan->count):NULL;
}
