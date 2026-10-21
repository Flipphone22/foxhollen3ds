#include "fh_replay.h"
/* Opt-in, bounded LCD telemetry. No files, allocations or GPU fences here.
 * Filesystem durations include service/scheduling waits across worker threads;
 * they are not exclusive main-thread stalls. Audio counts whole queued blocks. */
#include <3ds.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "fh_perf_display.h"
#include "fh_graphics_options.h"
#include "fh_sd_stats.h"
#include "fh_gpu_work.h"
#include "fh_platform.h"
extern unsigned fh_minimal_frames;
FhPerfDisplay fh_perf_display;

#ifdef FH_CPU_PANEL_BUILD
#include "fh_replay_timing.h"
#include "fh_hardware.h"
unsigned fh_cpu_panel_enabled;
static FhReplayClock cpu_clock;
static Thread cpu_owner;
static uint64_t cpu_previous[FH_PHASE_COUNT];
static unsigned cpu_frames;
static uint64_t cpu_log_ticks[FH_PHASE_COUNT],cpu_log_frames;
static unsigned cpu_log_samples;
static uint64_t cpu_last_ticks[FH_PHASE_COUNT];
static unsigned cpu_last_frames;
unsigned fh_cpu_panel_phase_at(unsigned phase,uint64_t now){
    if(!__atomic_load_n(&fh_cpu_panel_enabled,__ATOMIC_RELAXED)||threadGetCurrent()!=__atomic_load_n(&cpu_owner,__ATOMIC_RELAXED))return FH_PHASE_COUNT;
    /* A scope entered before PERF was enabled has no recorded parent. */
    return fh_replay_clock_enter(&cpu_clock,phase<FH_PHASE_COUNT?phase:FH_PHASE_OTHER,now);
}
unsigned fh_cpu_panel_phase(unsigned phase){
    if(threadGetCurrent()!=__atomic_load_n(&cpu_owner,__ATOMIC_RELAXED))return FH_PHASE_COUNT;
    return fh_cpu_panel_phase_at(phase,svcGetSystemTick());
}
/* Every exclusive phase belongs to exactly one group. Native scopes subtract
 * themselves from their enclosing scene/actor/particle scopes automatically. */
static unsigned cpu_group(unsigned phase){
    switch(phase){
    case FH_PHASE_OBJECT_UPDATE:case FH_PHASE_ENVIRONMENT:case FH_PHASE_COLLISION:return 0;
    case FH_PHASE_SCENE_RENDER:case FH_PHASE_OBJECT_RENDER:case FH_PHASE_NATIVE_WORLD:
    case FH_PHASE_NATIVE_MODEL:case FH_PHASE_SCENE_SOURCE:case FH_PHASE_MODEL_REFRESH:
    case FH_PHASE_WORLD_VISIBILITY:case FH_PHASE_MODEL_PALETTE:return 1;
    case FH_PHASE_NATIVE_SUBMIT:return 2;
    case FH_PHASE_TEXTURE_RESOLVE:return 3;
    case FH_PHASE_AUDIO_UPDATE:case FH_PHASE_AUDIO_OBSERVE:case FH_PHASE_AUDIO_PUMP:case FH_PHASE_SFX:return 4;
    case FH_PHASE_LOAD_FILES:case FH_PHASE_DVD_CALLBACKS:return 5;
    case FH_PHASE_REFLECTION_SMALL:case FH_PHASE_REFLECTION_UPDATE:case FH_PHASE_DISTORTION:case FH_PHASE_PARTICLES:return 6;
    case FH_PHASE_UI_END:case FH_PHASE_UI_START:case FH_PHASE_TEXT:case FH_PHASE_UI_DRAW:return 7;
    case FH_PHASE_GPU_WAIT:return 8;
    case FH_PHASE_WORKER_WAIT:return 9;
    case FH_PHASE_VBLANK:return 10;
    case FH_PHASE_FRAME_SETUP:return 12;
    case FH_PHASE_PRESENT:case FH_PHASE_PRESENT_PREP:case FH_PHASE_PRESENT_COPY:
    case FH_PHASE_PRESENT_PUBLISH:case FH_PHASE_RETRACE_CALLBACKS:return 13;
    case FH_PHASE_INPUT:return 14;
    case FH_PHASE_METRICS:return 15;
    default:return 11;
    }
}
static void cpu_panel_visibility(u64 now){
    unsigned enabled=fh_graphics_perf_visible&&!fh_hardware_is_new;
#ifdef FH_UNIFIED_APP
    enabled=enabled&&!fh_replay_active(); /* Replay owns the exclusive clock. */
#endif
    if(enabled==__atomic_load_n(&fh_cpu_panel_enabled,__ATOMIC_RELAXED))return;
    if(enabled){
        memset(&cpu_clock,0,sizeof(cpu_clock));memset(cpu_previous,0,sizeof(cpu_previous));
        __atomic_store_n(&cpu_owner,threadGetCurrent(),__ATOMIC_RELAXED);cpu_frames=fh_minimal_frames;
        fh_replay_clock_enter(&cpu_clock,FH_PHASE_OTHER,now);
    }
    fh_perf_display.cpu_valid=0;
    __atomic_store_n(&fh_cpu_panel_enabled,enabled,__ATOMIC_RELAXED);
}
static void cpu_panel_sample(u64 now){
    if(!__atomic_load_n(&fh_cpu_panel_enabled,__ATOMIC_RELAXED))return;
    uint64_t current[FH_PHASE_COUNT],groups[16]={0},total=0;
    fh_replay_clock_snapshot(&cpu_clock,now,current);
    unsigned count=fh_minimal_frames-cpu_frames;
    for(unsigned i=0;i<FH_PHASE_COUNT;i++){
        uint64_t delta=current[i]-cpu_previous[i];groups[cpu_group(i)]+=delta;total+=delta;
    }
    fh_perf_display.cpu_valid=count&&!cpu_clock.errors;
    if(fh_perf_display.cpu_valid){
        for(unsigned i=0;i<FH_PHASE_COUNT;i++){
            cpu_last_ticks[i]=current[i]-cpu_previous[i];cpu_log_ticks[i]+=cpu_last_ticks[i];
        }
        cpu_last_frames=count;
        cpu_log_frames+=count;cpu_log_samples++;
        fh_perf_display.cpu_total_us=total*1000000/SYSCLOCK_ARM11/count;
        for(unsigned i=0;i<16;i++)fh_perf_display.cpu_us[i]=groups[i]*1000000/SYSCLOCK_ARM11/count;
    }
    memcpy(cpu_previous,current,sizeof(current));cpu_frames=fh_minimal_frames;
}
#endif

static u64 next_sample,last_sample;
#ifdef FH_REPLAY_BUILD
static u64 next_gpu_sample;
#endif
static FhSdDisplay previous;
static unsigned was_visible;
static FhGpuWork previous_gpu;
static unsigned previous_frames,timing_started;
static u64 next_timing;
/* Queue occupancy includes copies, service and interrupt latency. It overlaps
 * CPU work and is neither a shader-stage duration nor pure GPU execution time. */
static void sample_gpu_timing(u64 now){
    if(!fh_graphics_gpu_visible){timing_started=0;return;}
    if(timing_started&&now<next_timing)return;
    FhGpuWork current;fh_gpu_work_snapshot(&current);
    unsigned frames=fh_minimal_frames,count=frames-previous_frames;
    fh_perf_display.gpu_timing_valid=timing_started&&count&&
        current.failed==previous_gpu.failed&&current.unmatched==previous_gpu.unmatched&&
        current.unsupported==previous_gpu.unsupported;
    if(fh_perf_display.gpu_timing_valid){
        fh_perf_display.frame_us=(current.sampled_ticks-previous_gpu.sampled_ticks)*1000000/SYSCLOCK_ARM11/count;
        fh_perf_display.gpu_queue_us=(current.busy_ticks-previous_gpu.busy_ticks)*1000000/SYSCLOCK_ARM11/count;
    }
    previous_gpu=current;previous_frames=frames;next_timing=now+SYSCLOCK_ARM11;timing_started=1;
    fh_perf_display.gpu_generation++;
}
#if !defined(FH_REPLAY_BUILD) || defined(FH_UNIFIED_APP)
/* Reuse the physically exercised bounded FIFO protocol. The replay application
 * owns its own probe and exposes samples instead of starting a second owner. */
#define FH_HOTSPOT_NO_FILE_OUTPUT
#include "platform_replay_hotspot.h"
static aptHookCookie perf_hook;
static unsigned hooked,suspended;
static void perf_apt(APT_HookType type,void *unused){
    (void)unused;
    if(type==APTHOOK_ONSUSPEND||type==APTHOOK_ONSLEEP){suspended=1;hotspot_restore();}
    if(type==APTHOOK_ONRESTORE||type==APTHOOK_ONWAKEUP){suspended=0;hotspot_next=0;}
}
#endif

void fh_perf_display_report(void){
#ifdef FH_CPU_PANEL_BUILD
    if(!cpu_log_frames)return;
    static const char*names[]={
#define CPU_NAME(symbol,name) #name,
        FH_REPLAY_PHASES(CPU_NAME)
#undef CPU_NAME
    };
    char line[192];uint64_t total=0;
    for(unsigned i=0;i<FH_PHASE_COUNT;i++)total+=cpu_log_ticks[i];
    snprintf(line,sizeof(line),"Main CPU panel: windows=%u frames=%llu ticks=%llu hz=%u exclusive=1 profiler_overhead_included=1\n",
        cpu_log_samples,(unsigned long long)cpu_log_frames,(unsigned long long)total,SYSCLOCK_ARM11);
    fh_platform_log(line);
    for(unsigned i=0;i<FH_PHASE_COUNT;i++){
        snprintf(line,sizeof(line),"Main CPU phase %s: ticks=%llu mean_us=%llu\n",names[i],
            (unsigned long long)cpu_log_ticks[i],(unsigned long long)(cpu_log_ticks[i]*1000000/SYSCLOCK_ARM11/cpu_log_frames));
        fh_platform_log(line);
    }
    /* Retain the exact same completed window as the LCD groups. Whole-run
     * means mix loading, movies and gameplay and cannot explain that window. */
    if(fh_perf_display.cpu_valid && cpu_last_frames)for(unsigned i=0;i<FH_PHASE_COUNT;i++){
        snprintf(line,sizeof(line),"Main CPU last phase %s: ticks=%llu frames=%u us=%llu\n",names[i],
            (unsigned long long)cpu_last_ticks[i],cpu_last_frames,
            (unsigned long long)(cpu_last_ticks[i]*1000000/SYSCLOCK_ARM11/cpu_last_frames));fh_platform_log(line);
    }
    snprintf(line,sizeof(line),"Main CPU last window: valid=%u total_us=%u\n",fh_perf_display.cpu_valid,fh_perf_display.cpu_total_us);fh_platform_log(line);
    for(unsigned i=0;i<16;i++){
        snprintf(line,sizeof(line),"Main CPU last group %u: us=%u\n",i,fh_perf_display.cpu_us[i]);fh_platform_log(line);
    }
#endif
}

void fh_perf_display_shutdown(void){
#if !defined(FH_REPLAY_BUILD) || defined(FH_UNIFIED_APP)
    hotspot_restore();if(hooked){aptUnhook(&perf_hook);hooked=0;}
#endif
}
void fh_perf_display_poll(void){
    if(!fh_graphics_perf_visible&&!fh_graphics_gpu_visible&&!was_visible
#if !defined(FH_REPLAY_BUILD) || defined(FH_UNIFIED_APP)
       &&!hotspot_requested
#endif
    )return;
    u64 now=svcGetSystemTick();
#ifdef FH_CPU_PANEL_BUILD
    cpu_panel_visibility(now);
#endif
    sample_gpu_timing(now);
#if !defined(FH_REPLAY_BUILD) || defined(FH_UNIFIED_APP)
#ifdef FH_UNIFIED_APP
    if(!fh_replay_active()){
#endif
    if(!fh_graphics_gpu_visible){
        if(hotspot_requested){fh_perf_display_shutdown();hotspot_requested=hotspot_failed=0;fh_perf_display.gpu_valid=fh_perf_display.gpu_failed=0;}
    }else{
        if(!hotspot_requested){hotspot_requested=1;hotspot_next=0;hotspot_failed=0;hotspot_count=0;}
        if(!hooked){aptHook(&perf_hook,perf_apt,NULL);hooked=1;}
        if(!suspended)hotspot_poll(1,0,0);
        if(hotspot_count){
            HotspotSample *s=hotspot_samples+hotspot_count-1;
            memcpy(fh_perf_display.gpu_words,s->words,sizeof(s->words));
            fh_perf_display.gpu_valid=s->valid;fh_perf_display.gpu_failed=hotspot_failed;
            fh_perf_display.gpu_ipc_us=(u64)s->ipc_ticks*1000000/SYSCLOCK_ARM11;
            fh_perf_display.gpu_generation++;hotspot_count=0;hotspot_next=now+SYSCLOCK_ARM11;
        }else if(hotspot_failed&&!fh_perf_display.gpu_failed){fh_perf_display.gpu_failed=1;fh_perf_display.gpu_generation++;}
    }
#ifdef FH_UNIFIED_APP
    }
#endif
#endif
#ifdef FH_REPLAY_BUILD
    if(fh_replay_active() && fh_graphics_gpu_visible && now>=next_gpu_sample){
        fh_perf_display.gpu_valid=fh_replay_gpu_display(fh_perf_display.gpu_words,&fh_perf_display.gpu_failed,&fh_perf_display.gpu_ipc_us);
        fh_perf_display.gpu_generation++;
        next_gpu_sample=now+SYSCLOCK_ARM11;
    }
#endif
    if(!fh_graphics_perf_visible){was_visible=0;return;}
    if(was_visible&&now<next_sample)return;
#ifdef FH_CPU_PANEL_BUILD
    cpu_panel_sample(now);
#endif
    extern u32 __ctru_heap_size,__ctru_linear_heap_size;
    struct mallinfo heap=mallinfo();
    fh_perf_display.heap_used=(unsigned)heap.uordblks;fh_perf_display.heap_limit=__ctru_heap_size;
    fh_perf_display.linear_limit=__ctru_linear_heap_size;
    fh_perf_display.linear_used=__ctru_linear_heap_size-linearSpaceFree();
    fh_perf_display.vram_used=6*1024*1024-vramSpaceFree();
    fh_perf_display.audio_valid=fh_audio_display_snapshot(fh_perf_display.audio_ms,fh_perf_display.audio_underruns);
    FhSdDisplay sd;fh_sd_stats_snapshot(&sd);
    u64 elapsed=now-last_sample;
    fh_perf_display.read_kib_s=was_visible&&elapsed?(sd.read_bytes-previous.read_bytes)*SYSCLOCK_ARM11/elapsed/1024:0;
    fh_perf_display.read_ms=was_visible?(sd.read_ticks-previous.read_ticks)*1000/SYSCLOCK_ARM11:0;
    fh_perf_display.fs_ms=was_visible?(sd.all_ticks-previous.all_ticks)*1000/SYSCLOCK_ARM11:0;
    fh_perf_display.read_max_ms=sd.read_max_ticks*1000/SYSCLOCK_ARM11;
    previous=sd;last_sample=now;next_sample=now+SYSCLOCK_ARM11;was_visible=1;fh_perf_display.generation++;
}
