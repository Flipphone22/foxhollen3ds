#include <3ds.h>
#include <stdio.h>
#include "fh_gpu_cache.h"
#include "fh_platform.h"
#include "fh_audio_worker.h"
#include "fh_pica_glow.h"
#include "fh_pica_world_packet_owner.h"

/* Pinned Melee command_cache.c: clean owned CPU ranges before publishing
 * command segments, including Citro3D's internal clear/transfer splits.
 * The enclosing FrameEnd still gets GX_CMDLIST_FLUSH so it does not flush
 * the entire linear heap. Preserve legacy behavior outside native mode. */
#include "fh_render_floor.h"
static unsigned unavailable,commands;
static u64 command_bytes;
/* Main uploads and shared-owner command/index publication can overlap. Keep
 * telemetry per owner; report only after both are joined at shutdown. */
static struct {unsigned direct_calls,service_calls;u64 cleaned_bytes,flush_ticks;} owner_stats[2];
void fh_gpu_cache_flush(const void *data,unsigned bytes){
    if(!bytes)return;
    unsigned owner=fh_audio_worker_is_current()!=0;
    u64 start=svcGetSystemTick();Result result=-1;
    if(!__atomic_load_n(&unavailable,__ATOMIC_RELAXED)){
        owner_stats[owner].direct_calls++;
        result=svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)(uintptr_t)data,bytes);
        if(R_FAILED(result))__atomic_store_n(&unavailable,1,__ATOMIC_RELAXED);
    }
    if(__atomic_load_n(&unavailable,__ATOMIC_RELAXED)){owner_stats[owner].service_calls++;result=GSPGPU_FlushDataCache(data,bytes);}
    if(R_FAILED(result))fh_platform_panic("Native GPU range flush failed");
    owner_stats[owner].cleaned_bytes+=bytes;owner_stats[owner].flush_ticks+=svcGetSystemTick()-start;
}
Result __real_GX_ProcessCommandList(u32*,u32,u8);
extern void fh_minimal_publish_world_indices(void);
Result __wrap_GX_ProcessCommandList(u32 *buffer,u32 bytes,u8 flags){
    if(fh_minimal_enabled){
        /* Covers early queue, clear/transfer splits and final FrameEnd. Flush
         * append-only index storage before making any reader executable. */
        if(!fh_pica_packet_publish(buffer,bytes))fh_platform_panic("PICA packet segment publication");
        fh_minimal_publish_world_indices();
        fh_pica_glow_publish();
        fh_gpu_cache_flush(buffer,bytes);commands++;command_bytes+=bytes;
        flags&=~GX_CMDLIST_FLUSH;
    }
    return __real_GX_ProcessCommandList(buffer,bytes,flags);
}
void fh_gpu_cache_report(void){
    char line[240];snprintf(line,sizeof(line),
        "Native cache: direct=%u service=%u commands=%u bytes=%llu command_bytes=%llu ticks=%llu\n",
        owner_stats[0].direct_calls+owner_stats[1].direct_calls,
        owner_stats[0].service_calls+owner_stats[1].service_calls,commands,
        (unsigned long long)(owner_stats[0].cleaned_bytes+owner_stats[1].cleaned_bytes),
        (unsigned long long)command_bytes,
        (unsigned long long)(owner_stats[0].flush_ticks+owner_stats[1].flush_ticks));fh_platform_log(line);
}
