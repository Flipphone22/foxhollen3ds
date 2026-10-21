/* Pinned Melee early_queue.c: completion includes queued GPU consumers, not
 * merely VBlank. Reuse Foxhollow's scene-storage join and explicit range flush.
 * No resource can be recycled on a timeout; the existing join owns that policy.
 * Publication copies a prepared packet once or reads into final owned storage,
 * without rebuilding its geometry. */
#include <3ds.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "fh_actor_fog_resident.h"
#include "fh_actor_fog_collection.h"
#include "fh_memory_budget.h"
#include "fh_gpu_cache.h"
#include "fh_scene_storage.h"
static FhActorFogResidentStats stats;
static void high_water(void){
    if(stats.cpu_bytes>stats.cpu_peak)stats.cpu_peak=stats.cpu_bytes;
    if(stats.gpu_bytes>stats.gpu_peak)stats.gpu_peak=stats.gpu_bytes;
}
static int room(unsigned cpu,unsigned gpu,unsigned scratch){
    extern u32 __ctru_heap_size,__stacksize__;
    unsigned free_linear=linearSpaceFree();
    return cpu<=FH_ACTOR_FOG_RESIDENT_CPU_LIMIT-stats.cpu_bytes &&
        gpu<=FH_ACTOR_FOG_RESIDENT_GPU_LIMIT-stats.gpu_bytes &&
        (u64)((unsigned)mallinfo().uordblks)+(u64)__stacksize__+cpu+scratch+64<=
            (u64)__ctru_heap_size*85/100 &&
        gpu<=free_linear && free_linear-gpu>=FH_NATIVE_LINEAR_RESERVE;
}
static void release_joined(FhActorFogResident*r){
    if(!r->metadata)return;
    stats.cpu_bytes-=r->asset.metadata_bytes;stats.gpu_bytes-=r->geometry_allocation_bytes;
    stats.records--;stats.releases++;
    free(r->metadata);linearFree(r->geometry);*r=(FhActorFogResident){0};
}
static int publish(FhActorFogResident*r,const void*meta,
    unsigned cpu,const void*gpu,unsigned linear,unsigned id,unsigned source_bytes,
    unsigned source_crc,unsigned variant,FhActorFogRead read,void*context,unsigned packet_crc){
    if(!r || (!read&&(!meta||!gpu)) || cpu<128 || cpu>FH_ACTOR_FOG_META_CAP ||
       !linear || linear>FH_ACTOR_FOG_GPU_CAP){
        stats.rejected++;return 0;
    }
    unsigned allocation=(linear+127u)&~127u;
    /* Include worst-case transient compressed-reader workspace before either
     * persistent allocation. Plain file readers conservatively reserve it too.
     * It is not resident CPU/GPU storage and is freed before publication. */
    if(!room(cpu,allocation,read?FH_ACTOR_FOG_COLLECTION_SCRATCH:0)){stats.rejected++;return 0;}
    void *new_meta=malloc(cpu);
    if(!new_meta){stats.allocation_failures++;return 0;}
    stats.cpu_bytes+=cpu;high_water();
    void *new_gpu=linearMemAlign(allocation,128);
    if(!new_gpu){
        free(new_meta);stats.cpu_bytes-=cpu;stats.allocation_failures++;return 0;
    }
    stats.gpu_bytes+=allocation;high_water();
    int loaded=1;
    if(read){
        loaded=read(context,0,new_meta,cpu)==1&&read(context,cpu,new_gpu,linear)==1;
        if(loaded){
            unsigned crc=(unsigned)crc32(0,new_meta,cpu);
            loaded=(unsigned)crc32(crc,new_gpu,linear)==packet_crc;
        }
    }else{memcpy(new_meta,meta,cpu);memcpy(new_gpu,gpu,linear);}
    FhActorFogAsset asset;
    if(!loaded || !fh_native_actor_fog_open(&asset,new_meta,cpu,new_gpu,linear,id,source_bytes,source_crc,variant)){
        free(new_meta);linearFree(new_gpu);stats.cpu_bytes-=cpu;
        stats.gpu_bytes-=allocation;stats.rejected++;return 0;
    }
    /* Complete publication before exposing either pointer to the consumer.
     * A failure/timeout inside flush/join must not return as successful reuse. */
    fh_gpu_cache_flush(new_gpu,linear);
    stats.records++;stats.publications++;
    if(read)stats.streamed_bytes+=(u64)cpu+linear;else stats.copied_bytes+=(u64)cpu+linear;
    if(r->metadata){fh_renderer_scene_storage_join();release_joined(r);}
    *r=(FhActorFogResident){asset,new_meta,new_gpu,source_bytes,source_crc,allocation};
    return 1;
}
int fh_actor_fog_resident_publish(FhActorFogResident*r,const void*meta,
    unsigned cpu,const void*gpu,unsigned linear,unsigned id,unsigned source_bytes,
    unsigned source_crc,unsigned variant){
    return publish(r,meta,cpu,gpu,linear,id,source_bytes,source_crc,variant,NULL,NULL,0);
}
int fh_actor_fog_resident_load(FhActorFogResident*r,unsigned cpu,unsigned linear,
    unsigned crc,FhActorFogRead read,void*context,unsigned id,unsigned source_bytes,
    unsigned source_crc,unsigned variant){
    if(!read){stats.rejected++;return 0;}
    return publish(r,NULL,cpu,NULL,linear,id,source_bytes,source_crc,variant,read,context,crc);
}
const FhActorFogAsset *fh_actor_fog_resident_find(const FhActorFogResident*r,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned variant){
    if(!r || !r->metadata || r->asset.id!=id || r->source_bytes!=source_bytes ||
       r->source_crc!=source_crc || r->asset.variant!=variant)return NULL;
    return &r->asset;
}
void fh_actor_fog_resident_release(FhActorFogResident*r){
    if(!r || !r->metadata)return;
    fh_renderer_scene_storage_join();release_joined(r);
}
FhActorFogResidentStats fh_actor_fog_resident_stats(void){return stats;}
void fh_actor_fog_resident_release_many(FhActorFogResident*r,unsigned count){
    if(!r)return;
    unsigned live=0;for(unsigned i=0;i<count;i++)live|=r[i].metadata!=NULL;
    if(!live)return;
    fh_renderer_scene_storage_join();
    for(unsigned i=0;i<count;i++)release_joined(r+i);
}
