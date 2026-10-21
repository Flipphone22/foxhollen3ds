#ifdef FH_INSTALL_STARTUP
#include "fh_install_boot.h"
#include "install_recipe.h"
#endif
#include "fh_memory_cadence.h"
#include "fh_pica_model.h"
#include "fh_pica_scene.h"
#include <3ds.h>
#include <citro3d.h>
#include "fh_frame_pacing.h"
#include "fh_stack_stats.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fh_platform.h"
#include "fh_touch_cstick.h"
#include "fh_graphics_options.h"
#include "fh_perf_display.h"
#include "fh_replay_timing.h"
#include "fh_input_tape.h"
#include "fh_hardware.h"
#include "fh_graphics.h"
#include "fh_geometry_cache.h"
#include "fh_retained_geometry.h"
#include "fh_gx_dispatch.h"
#include "fh_model_resources.h"
#include "fh_fur_detail.h"
unsigned fh_fur_shell_budget=3,fh_fur_shells_original,fh_fur_shells_rendered;
unsigned fh_fur_shell_advance=1;
#include "fh_surface_detail.h"
unsigned fh_surface_detail,fh_surface_detail_models,fh_surface_detail_fur,fh_surface_detail_world;
#include "fh_material_scope.h"
#include "fh_fragment_raster.h"
#include "fh_audio_worker.h"
#include "fh_cpu.h"
#include "fh_sd_stats.h"
#include "fh_log_io.h"
#include "fh_diagnostic_buffer.h"
#include "fh_movie_prefetch.h"
#ifdef FH_REPLAY_BUILD
#include "fh_replay.h"
#endif

/* Move CPU-owned retained geometry out of linear RAM. The additional four
 * MiB keeps normal-heap reserve while total process reservation stays below
 * 85 percent of the measured New 3DS 124 MiB entitlement. */
u32 __ctru_heap_size=46*1024*1024;
u32 __ctru_linear_heap_size=6*1024*1024;
u32 __stacksize__=512*1024;
static FILE *logfile,*input_file,*measurements;
static unsigned retraces;
static FhFramePacing frame_pacing;
static unsigned frame_pacing_enabled=1,frame_pacing_waits,frame_pacing_ready;
#ifdef FH_EMULATOR_TEST
static unsigned limit;
static FILE *host_control;
enum { PROFILE_SERVICE_COUNT=19 };
static u64 service_ticks[PROFILE_SERVICE_COUNT];
#define PROFILE_SERVICE(name,index) \
    void __real_##name(void); \
    void __wrap_##name(void){u64 start=svcGetSystemTick();__real_##name();service_ticks[index]+=svcGetSystemTick()-start;}
PROFILE_SERVICE(audioUpdate,0)
PROFILE_SERVICE(waitNextFrame,1)
PROFILE_SERVICE(loadDataFiles,2)
PROFILE_SERVICE(uiDll_runFrameEndAndLoadNext,3)
PROFILE_SERVICE(gameTextRun,4)
PROFILE_SERVICE(Sfx_UpdateLoopedObjectSounds,5)
/* Inclusive phase totals: scene rendering contains display-list/renderer
 * work. These emulator-only wrappers preserve every call and argument. */
void __real_Obj_UpdateAllObjects(u8 flags);
void __wrap_Obj_UpdateAllObjects(u8 flags){u64 start=svcGetSystemTick();__real_Obj_UpdateAllObjects(flags);service_ticks[9]+=svcGetSystemTick()-start;}
void __real_updateEnvironment(int mode);
void __wrap_updateEnvironment(int mode){u64 start=svcGetSystemTick();__real_updateEnvironment(mode);service_ticks[10]+=svcGetSystemTick()-start;}
PROFILE_SERVICE(trackIntersect,11)
void __real_sceneRender(int a,int b,int c,int d,int e,int f);
void __wrap_sceneRender(int a,int b,int c,int d,int e,int f){u64 start=svcGetSystemTick();__real_sceneRender(a,b,c,d,e,f);service_ticks[12]+=svcGetSystemTick()-start;}
void __real_curUiDllDraw(int a,int b,int c,int d);
void __wrap_curUiDllDraw(int a,int b,int c,int d){u64 start=svcGetSystemTick();__real_curUiDllDraw(a,b,c,d);service_ticks[13]+=svcGetSystemTick()-start;}
PROFILE_SERVICE(drawReflectionTexture,14)
PROFILE_SERVICE(updateReflectionTextures,15)
PROFILE_SERVICE(Rcp_UpdateDistortionTextures,16)
PROFILE_SERVICE(renderParticles,17)
struct GameObject;
void __real_objRender(int a,int b,int c,int d,struct GameObject *obj,int flag);
void __wrap_objRender(int a,int b,int c,int d,struct GameObject *obj,int flag){u64 start=svcGetSystemTick();__real_objRender(a,b,c,d,obj,flag);service_ticks[18]+=svcGetSystemTick()-start;}
int __real_uiDll_runFrameStartAndLoadNext(void);
int __wrap_uiDll_runFrameStartAndLoadNext(void){u64 start=svcGetSystemTick();int result=__real_uiDll_runFrameStartAndLoadNext();service_ticks[6]+=svcGetSystemTick()-start;return result;}
#ifdef FH_GX_DISPATCH_BUILD
/* Profile translation on its owner, not producer enqueue time. */
#define __real_GXCallDisplayList __real_fh_gxd_GXCallDisplayList
#define __wrap_GXCallDisplayList __wrap_fh_gxd_GXCallDisplayList
#endif
void __real_GXCallDisplayList(const void *list,u32 bytes);
static unsigned list_profile_first,list_profile_last,list_profile_used,list_profile_dropped;
static unsigned list_profile_render_domain,list_profile_first_observed,list_profile_last_observed;
extern unsigned fh_renderer_profile_frame(void);
typedef struct {
    const void *list;unsigned bytes,raw,stages,texgens,calls,max_vertices;
    u64 ticks,decoded,reused,phase[4],shading[3],fur[15];
    unsigned channel[4][6],gen[8][5],retained_reasons[12],compiled_profile[12];
} ListProfile;
static ListProfile list_profiles[256];
extern void fh_gx_vertex_counters(unsigned*,unsigned*);
extern unsigned fh_gx_last_list_vertices,fh_retained_reasons[12],fh_compiled_profile[12];
extern u64 fh_gx_phase_ticks[4];
extern u64 fh_gx_shading_ticks[3],fh_fur_path_ticks[15];
extern const FhDrawState *fh_gx_profile_state(unsigned*,unsigned*);
void __wrap_GXCallDisplayList(const void *list,u32 bytes){
    unsigned frame=list_profile_render_domain?fh_renderer_profile_frame():retraces;
    unsigned enabled=list_profile_first && frame>=list_profile_first && frame<=list_profile_last;
    if(enabled){if(!list_profile_first_observed)list_profile_first_observed=frame;list_profile_last_observed=frame;}
    unsigned decoded=0,reused=0,reasons[12],compiled_before[12];u64 phase[4],shading[3],fur[15];
    if(enabled){memcpy(shading,fh_gx_shading_ticks,sizeof(shading));memcpy(fur,fh_fur_path_ticks,sizeof(fur));}
    if(enabled){fh_gx_vertex_counters(&decoded,&reused);memcpy(phase,fh_gx_phase_ticks,sizeof(phase));memcpy(reasons,fh_retained_reasons,sizeof(reasons));memcpy(compiled_before,fh_compiled_profile,sizeof(compiled_before));}
    u64 start=svcGetSystemTick();__real_GXCallDisplayList(list,bytes);u64 elapsed=svcGetSystemTick()-start;service_ticks[7]+=elapsed;
    if(enabled){
        const FhDrawState *state=fh_gx_profile_state(NULL,NULL);
        unsigned i;for(i=0;i<list_profile_used;i++)if(list_profiles[i].list==list && list_profiles[i].bytes==bytes &&
            list_profiles[i].raw==state->raw_position && list_profiles[i].stages==state->stages && list_profiles[i].texgens==state->texgens)break;
        if(i==256){list_profile_dropped++;return;}
        ListProfile *p=list_profiles+i;
        if(i==list_profile_used){list_profile_used++;p->list=list;p->bytes=bytes;p->raw=state->raw_position;
            p->stages=state->stages;p->texgens=state->texgens;fh_gx_profile_state(&p->channel[0][0],&p->gen[0][0]);}
        p->calls++;p->ticks+=elapsed;if(fh_gx_last_list_vertices>p->max_vertices)p->max_vertices=fh_gx_last_list_vertices;
        unsigned after,after_reused;fh_gx_vertex_counters(&after,&after_reused);p->decoded+=after-decoded;p->reused+=after_reused-reused;
        for(unsigned j=0;j<4;j++)p->phase[j]+=fh_gx_phase_ticks[j]-phase[j];
        for(unsigned j=0;j<3;j++)p->shading[j]+=fh_gx_shading_ticks[j]-shading[j];
        for(unsigned j=0;j<15;j++)p->fur[j]+=fh_fur_path_ticks[j]-fur[j];
        for(unsigned j=0;j<12;j++){p->retained_reasons[j]+=fh_retained_reasons[j]-reasons[j];p->compiled_profile[j]+=fh_compiled_profile[j]-compiled_before[j];}
    }
}
static void write_list_profile(void){
    if(!list_profile_first)return;
    FILE *f=fopen("sdmc:/3ds/foxhollow/display-list-profile.json","wb");if(!f)return;
    fprintf(f,"{\"ticks_hz\":268123480,\"phase_ticks_hz\":268123480,\"frame_domain\":\"%s\",\"first_%s\":%u,\"last_%s\":%u,\"first_observed\":%u,\"last_observed\":%u,\"dropped_calls\":%u,\"scope\":\"Bounded emulator display-list groups; phases overlap renderer; channel/texgen state is first observed only\",\"lists\":[",list_profile_render_domain?"render.csv one-based":"retrace",list_profile_render_domain?"frame":"retrace",list_profile_first,list_profile_render_domain?"frame":"retrace",list_profile_last,list_profile_first_observed,list_profile_last_observed,list_profile_dropped);
    for(unsigned i=0;i<list_profile_used;i++){
        const ListProfile*p=list_profiles+i;
        fprintf(f,"%s{\"address\":%u,\"bytes\":%u,\"raw\":%u,\"stages\":%u,\"texgens\":%u,\"calls\":%u,\"max_vertices\":%u,\"ticks\":%llu,\"decoded\":%llu,\"reused\":%llu,\"phase_ticks\":[%llu,%llu,%llu,%llu],\"channels\":[",i?",":"",(unsigned)p->list,p->bytes,p->raw,p->stages,p->texgens,p->calls,p->max_vertices,p->ticks,p->decoded,p->reused,p->phase[0],p->phase[1],p->phase[2],p->phase[3]);
        for(unsigned j=0;j<4;j++){const unsigned*c=p->channel[j];fprintf(f,"%s[%u,%u,%u,%u,%u,%u]",j?",":"",c[0],c[1],c[2],c[3],c[4],c[5]);}
        fputs("],\"generators\":[",f);
        for(unsigned j=0;j<p->texgens && j<8;j++){const unsigned*g=p->gen[j];fprintf(f,"%s[%u,%u,%u,%u,%u]",j?",":"",g[0],g[1],g[2],g[3],g[4]);}
        fputs("],\"retained_reasons\":[",f);
        for(unsigned j=0;j<12;j++)fprintf(f,"%s%u",j?",":"",p->retained_reasons[j]);
        fputs("],\"compiled_profile\":[",f);
        for(unsigned j=0;j<12;j++)fprintf(f,"%s%u",j?",":"",p->compiled_profile[j]);
        fputs("],\"shading_ticks\":[",f);
        for(unsigned j=0;j<3;j++)fprintf(f,"%s%llu",j?",":"",p->shading[j]);
        fputs("],\"fur_ticks\":[",f);
        for(unsigned j=0;j<15;j++)fprintf(f,"%s%llu",j?",":"",p->fur[j]);
        fputs("]}",f);
    }
    fputs("]}\n",f);fclose(f);
}
void __real_fh_renderer_draw(const FhVertex *vertices,unsigned count,const FhDrawState *state);
void __wrap_fh_renderer_draw(const FhVertex *vertices,unsigned count,const FhDrawState *state){u64 start=svcGetSystemTick();__real_fh_renderer_draw(vertices,count,state);service_ticks[8]+=svcGetSystemTick()-start;}
void __real_fh_renderer_draw_identified(const FhVertex *vertices,unsigned count,const FhDrawState *state,const uint32_t *identities);
void __wrap_fh_renderer_draw_identified(const FhVertex *vertices,unsigned count,const FhDrawState *state,const uint32_t *identities){u64 start=svcGetSystemTick();__real_fh_renderer_draw_identified(vertices,count,state,identities);service_ticks[8]+=svcGetSystemTick()-start;}
#endif
static int quitting;
static int gpu_wait_exit_requested;
static FhPadState input;
static struct { unsigned frame,buttons;int x,y,cx,cy,left,right; } next_input;
static int scripted,has_input;
static unsigned char *stack_base,*stack_probe_end;
static unsigned stack_bytes;
static unsigned stack_mapping_bytes,stack_mapping_base,stack_initial_pointer;
extern u32 __ctru_heap;
static void stack_probe_init(void) {
    uintptr_t pointer;__asm__ volatile("mov %0, sp":"=r"(pointer));
    MemInfo info;PageInfo page;
    if(R_FAILED(svcQueryMemory(&info,&page,pointer)))return;
    stack_mapping_bytes=info.size;stack_mapping_base=info.base_addr;stack_initial_pointer=pointer;
    uintptr_t bottom=__ctru_heap,top=(bottom+__stacksize__+7)&~7u;
    if(bottom<info.base_addr || top>info.base_addr+info.size || pointer<bottom+8192 || pointer>top)return;
    stack_base=(unsigned char*)bottom;stack_bytes=top-bottom;
    stack_probe_end=(unsigned char*)((pointer-4096)&~31u);
    memset(stack_base,0xa5,stack_probe_end-stack_base);
}
extern void gameMain(void);
extern void fh_platform_audio_shutdown(void);
typedef struct { int numSlots,slotsUsed;unsigned char *start;int size,usedBytes; } MmRegion;
extern MmRegion gMmRegionTable[8];
extern unsigned char gMmRegionCount;
_Static_assert(sizeof(MmRegion)==20 && offsetof(MmRegion,usedBytes)==16,"Game memory telemetry ABI");
void *fh_platform_alloc(size_t bytes,size_t alignment) { return memalign(alignment,bytes); }
void fh_platform_free(void *memory) { free(memory); }
void fh_platform_log(const char *message) {
    if(logfile) {
        fprintf(logfile,"%u %s",retraces,message);
        /* Close/panic drains short-run telemetry; a full buffer still queues
         * owned bytes. Avoid periodic SD allocation while assets stream. */
    }
    svcOutputDebugString(message,strlen(message));
}
static void metrics(unsigned force) {
    if(!measurements) return;
    struct mallinfo heap=mallinfo();s64 committed=0,maximum=0;Handle resource;ResourceLimitType type=RESLIMIT_COMMIT;
    if(R_SUCCEEDED(svcGetResourceLimit(&resource,CUR_PROCESS_HANDLE))) {
        svcGetResourceLimitCurrentValues(&committed,resource,&type,1);svcGetResourceLimitLimitValues(&maximum,resource,&type,1);svcCloseHandle(resource);
    }
    static FhMemoryCadence cadence;
    static unsigned last_renderer,last_options,last_regions;
    unsigned linear_free=linearSpaceFree(),vram_free=vramSpaceFree(),game_used=fh_memory_used();
    unsigned renderer=fh_renderer_ready();
    FhGameStackStats stacks[15];unsigned stack_count=fh_game_stack_stats(stacks,15);
    unsigned pressure[FH_MEMORY_PRESSURES]={
        (unsigned)heap.uordblks,~linear_free,renderer?~vram_free:0,game_used,(unsigned)committed};
    for(unsigned i=0;i<gMmRegionCount && i<8;i++){
        pressure[5+2*i]=(unsigned)gMmRegionTable[i].usedBytes;
        pressure[6+2*i]=(unsigned)gMmRegionTable[i].slotsUsed;
    }
    for(unsigned i=0;i<stack_count;i++){
        pressure[21+i]=stacks[i].observed_used;
        if(!stacks[i].canary_ok)force=1;
    }
    force|=retraces<30 || renderer!=last_renderer || fh_graphics_options!=last_options ||
        gMmRegionCount!=last_regions || !maximum;
    if(!fh_memory_cadence_due(&cadence,svcGetSystemTick(),SYSCLOCK_ARM11,force,pressure))return;
    last_renderer=renderer;last_options=fh_graphics_options;last_regions=gMmRegionCount;
    fprintf(measurements,"{\"hardware_new\":%u,\"large_memory\":%u,",fh_hardware_is_new,fh_hardware_large_memory);
    fprintf(measurements,"\"memory_observation\":{\"samples\":%u,\"rows\":%u,\"max_heartbeat_seconds\":5,\"new_pressure_peak_forces_row\":true},",cadence.samples,cadence.rows);
    fprintf(measurements,"\"retrace\":%u,\"heap_used\":%d,\"heap_reserved\":%lu,\"linear_free\":%lu,\"linear_reserved\":%lu,\"vram_free\":%lu,\"process_commit\":%lld,\"process_limit\":%lld,\"game_os_heap_used\":%u,\"game_regions\":[",retraces,heap.uordblks,(unsigned long)__ctru_heap_size,(unsigned long)linear_free,(unsigned long)__ctru_linear_heap_size,(unsigned long)vram_free,(long long)committed,(long long)maximum,game_used);
    for(unsigned i=0;i<gMmRegionCount && i<8;i++) {
        const MmRegion *r=&gMmRegionTable[i];
        fprintf(measurements,"%s{\"capacity\":%d,\"used\":%d,\"slots\":%d,\"slots_used\":%d}",i?",":"",r->size,r->usedBytes,r->numSlots,r->slotsUsed);
    }
    unsigned untouched=0;
    /* Same pressure sampling points. Repeated full reports are coalesced,
     * but new sampled peaks, canary failures and forced events emit a row. */
    fprintf(measurements,"],\"memory_schema\":2,\"renderer_backend\":\"native-pica\",\"renderer_initialized\":%s,\"graphics_options\":%u",
        renderer?"true":"false",fh_graphics_options);
    unsigned worker_active,worker_core,worker_jobs,worker_peak;
    fh_audio_worker_stats(&worker_active,&worker_jobs,&worker_peak,&worker_core);
    fprintf(measurements,",\"audio_worker\":{\"active\":%u,\"requested_core\":2,\"reported_core\":%u,\"jobs\":%u,\"stack_bytes\":%u,\"stack_peak_upper_bound\":%u}",worker_active,worker_core,worker_jobs,fh_audio_worker_stack_bytes(),worker_peak);
    extern unsigned fh_audio_cpu_cache_enabled,fh_audio_cpu_cache_hits,fh_audio_cpu_cache_fallbacks;
    extern unsigned fh_audio_synth_jobs_enabled,fh_audio_synth_jobs_queued,fh_audio_synth_jobs_worker,fh_audio_synth_jobs_inline,fh_audio_synth_jobs_urgent;
    {FhMoviePrefetchStats s;fh_movie_prefetch_stats(&s);
    fprintf(measurements,",\"movie_prefetch\":{\"active\":%u,\"capacity\":%u,\"stack_bytes\":%u,\"queued\":%u,\"hits\":%u,\"misses\":%u,\"checks\":%u,\"errors\":%u,\"wait_ms\":%.3f,\"read_ms\":%.3f,\"max_read_ms\":%.3f,\"slow_reads\":%u,\"busy_polls\":%u}",
        s.active,s.capacity,s.stack_bytes,s.queued,s.hits,s.misses,s.checks,s.errors,
        s.wait_ticks*1000.0/SYSCLOCK_ARM11,s.read_ticks*1000.0/SYSCLOCK_ARM11,s.max_read_ticks*1000.0/SYSCLOCK_ARM11,s.slow_reads,s.busy_polls);}    {FhMoviePrefetchStats s;fh_stream_prefetch_stats(&s);
    fprintf(measurements,",\"stream_prefetch\":{\"active\":%u,\"capacity\":%u,\"stack_bytes\":%u,\"queued\":%u,\"hits\":%u,\"misses\":%u,\"checks\":%u,\"errors\":%u,\"wait_ms\":%.3f,\"read_ms\":%.3f}",
        s.active,s.capacity,s.stack_bytes,s.queued,s.hits,s.misses,s.checks,s.errors,
        s.wait_ticks*1000.0/SYSCLOCK_ARM11,s.read_ticks*1000.0/SYSCLOCK_ARM11);}    {FhMoviePrefetchStats s;fh_disc_async_stats(&s);
    fprintf(measurements,",\"disc_async\":{\"active\":%u,\"capacity\":%u,\"stack_bytes\":%u,\"queued\":%u,\"hits\":%u,\"misses\":%u,\"checks\":%u,\"errors\":%u,\"wait_ms\":%.3f,\"read_ms\":%.3f}",
        s.active,s.capacity,s.stack_bytes,s.queued,s.hits,s.misses,s.checks,s.errors,
        s.wait_ticks*1000.0/SYSCLOCK_ARM11,s.read_ticks*1000.0/SYSCLOCK_ARM11);}
    fprintf(measurements,",\"audio_pipeline\":{\"enabled\":%u,\"worker_commands\":%u,\"inline_commands\":%u,\"packets\":%u,\"peak_commands\":%u}",
        fh_audio_pipeline_enabled,__atomic_load_n(&fh_audio_pipeline_worker,__ATOMIC_RELAXED),
        __atomic_load_n(&fh_audio_pipeline_inline,__ATOMIC_RELAXED),__atomic_load_n(&fh_audio_pipeline_packets,__ATOMIC_RELAXED),fh_audio_pipeline_peak);
    fprintf(measurements,",\"audio_synth_jobs\":{\"enabled\":%u,\"queued\":%u,\"worker\":%u,\"inline\":%u,\"urgent_refills\":%u}",fh_audio_synth_jobs_enabled,__atomic_load_n(&fh_audio_synth_jobs_queued,__ATOMIC_RELAXED),__atomic_load_n(&fh_audio_synth_jobs_worker,__ATOMIC_RELAXED),__atomic_load_n(&fh_audio_synth_jobs_inline,__ATOMIC_RELAXED),__atomic_load_n(&fh_audio_synth_jobs_urgent,__ATOMIC_RELAXED));
    fprintf(measurements,",\"audio_cpu_cache\":{\"enabled\":%u,\"hits\":%u,\"fallbacks\":%u}",fh_audio_cpu_cache_enabled,fh_audio_cpu_cache_hits,fh_audio_cpu_cache_fallbacks);
    extern unsigned fh_depth_snapshot_reads,fh_depth_snapshot_reuses;
    fprintf(measurements,",\"depth_snapshots\":{\"transfers\":%u,\"reuses\":%u}",fh_depth_snapshot_reads,fh_depth_snapshot_reuses);
    {extern unsigned fh_model_lod_applied,fh_model_lod_rejected;
    fprintf(measurements,",\"model_lod\":{\"applied\":%u,\"rejected\":%u}",fh_model_lod_applied,fh_model_lod_rejected);}
    fprintf(measurements,",\"cpu_status\":\"%s\"",fh_cpu_status);
    FhLogIoStats logs;fh_log_io_stats(&logs);
    fprintf(measurements,",\"log_worker\":{\"active\":%u,\"queued\":%u,\"peak_records\":%u,\"queue_records\":%u,\"queue_payload_bytes\":%u,\"backpressure_waits\":%u,\"errors\":%u,\"stack_bytes\":%u}",logs.active,logs.queued,logs.peak,FH_LOG_IO_SLOTS,FH_LOG_IO_SLOTS*FH_LOG_IO_CHUNK,logs.waits,logs.errors,logs.stack_bytes);
    fputs(",\"game_stacks\":[",measurements);
    for(unsigned i=0;i<stack_count;i++)fprintf(measurements,"%s{\"slot\":%u,\"bytes\":%u,\"observed_used\":%u,\"canary_ok\":%s}",i?",":"",stacks[i].slot,stacks[i].bytes,stacks[i].observed_used,stacks[i].canary_ok?"true":"false");
    fputc(']',measurements);
    /* The untouched watermark persists throughout the run. Scan at joined
     * shutdown; repeatedly walking the half-MiB stack adds a periodic hitch.
     * Until then report the full reservation as the conservative upper bound. */
    if(quitting && stack_base)while(stack_base+untouched<stack_probe_end && stack_base[untouched]==0xa5)untouched++;
    fprintf(measurements,",\"main_stack_measured\":%s",quitting && stack_base?"true":"false");
    fprintf(measurements,",\"main_stack_bytes\":%u,\"main_stack_peak_upper_bound\":%u,\"stack_mapping_bytes\":%u,\"stack_mapping_base\":%u,\"stack_initial_pointer\":%u}\n",stack_bytes,stack_bytes-untouched,stack_mapping_bytes,stack_mapping_base,stack_initial_pointer);
}
#include "platform_last_frame_capture.h"
#ifdef FH_INSTALL_STARTUP
#include "platform_sd_bench.h"
#endif

/* Joined normal quit and deliberate startup-menu exits use __real_exit. Errors
 * (including JPEG error_exit) otherwise retire heaps under live workers. */
extern void __real_exit(int) __attribute__((noreturn));
__attribute__((noreturn,noinline)) void __wrap_exit(int status) {
    static unsigned entered;
    if(__atomic_exchange_n(&entered,1,__ATOMIC_ACQ_REL))svcExitProcess();
    char message[112];
    snprintf(message,sizeof(message),"Unexpected exit status=%d caller=%08lx core=%u",
        status,(unsigned long)(uintptr_t)__builtin_return_address(0),
        (unsigned)svcGetProcessorID());
    fh_platform_panic(message);
    svcExitProcess();
    __builtin_unreachable();
}

void fh_platform_quit(void) {
    fh_perf_display_shutdown();
    if(quitting) exit(2);
    fh_perf_display_report();
    extern void fh_gx_dispatch_stop(void);
    extern void fh_minimal_worker_stop(void);
    fh_minimal_worker_stop();fh_gx_dispatch_stop();fh_disc_async_join();quitting=1;metrics(1);fh_platform_log("shutdown\n");
    {char line[120];snprintf(line,sizeof(line),"Native pacing: enabled=%u waits=%u already_elapsed=%u\n",frame_pacing_enabled,frame_pacing_waits,frame_pacing_ready);fh_platform_log(line);}
    {extern void fh_model_lod_shutdown(void);fh_model_lod_shutdown();}
    {extern void fh_native_model_shutdown(void);fh_native_model_shutdown();}
    fh_record_finish();
#ifdef FH_REPLAY_BUILD
    fh_replay_input_stop();
#endif
    fh_platform_audio_shutdown();fh_renderer_shutdown();last_frame_capture(retraces);
    fh_card_shutdown();fh_dvd_shutdown();fh_memory_shutdown();
#ifdef FH_REPLAY_BUILD
    if(!fh_replay_active())
#endif
    if(!fh_graphics_settings_save())fh_platform_log("Graphics settings: save failed; prior settings retained\n");
    if(input_file) fclose(input_file);
    if(measurements) fclose(measurements);
    if(logfile){fclose(logfile);logfile=NULL;}
#ifdef FH_EMULATOR_TEST
    if(host_control)fclose(host_control);
    write_list_profile();
#endif
    fh_sd_stats_shutdown(retraces);
#ifdef FH_REPLAY_BUILD
    fh_replay_finish();
#endif
    fh_platform_cpu_exit();gfxExit();
#ifdef FH_EMULATOR_TEST
    /* A host timeout may request the same clean exit as Select. Acknowledge
     * only after renderer/card/log shutdown has drained owned buffers. */
    FILE *ack=fopen("sdmc:/3ds/foxhollow/shutdown-complete.txt","wb");
    if(ack){fprintf(ack,"retrace=%u\n",retraces);fclose(ack);}
#endif
    __real_exit(0);
}
void fh_platform_panic(const char *message) {
    extern void fh_gx_dispatch_worker_panic(const char*);
    extern void fh_minimal_worker_panic(const char*);
    fh_minimal_worker_panic(message);fh_gx_dispatch_worker_panic(message);
    fh_platform_log("PANIC: ");fh_platform_log(message);fh_platform_log("\n");metrics(1);
    FILE *file=fopen("sdmc:/3ds/foxhollow/panic.txt","wb");if(file) { fputs(message,file);fclose(file); }
    if(logfile) fflush(logfile);
    if(measurements)fflush(measurements);
    (void)fh_log_io_drain(0);
    /* Fatal paths can originate with locks held or on a worker itself.
     * Do not run libc/libctru teardown: __libctru_exit unmaps both heaps
     * before terminating other threads, including their stacks. Diagnostics
     * are drained above; let the kernel end the entire process atomically.
     * Ordinary Select shutdown still joins owners and returns via exit(0). */
    svcExitProcess();
}

/* Game/library failures can originate on an audio worker. Newlib abort reaches
 * _exit/__libctru_exit, which retires the heaps before other threads stop.
 * Preserve the caller before using the existing diagnostic/process-exit path.
 * This replaces termination machinery, never the failing game operation. */
__attribute__((noreturn,noinline)) void abort(void) {
    static unsigned entered;
    if(__atomic_exchange_n(&entered,1,__ATOMIC_ACQ_REL))svcExitProcess();
    char message[96];
    snprintf(message,sizeof(message),"Native abort caller=%08lx core=%u",
        (unsigned long)(uintptr_t)__builtin_return_address(0),
        (unsigned)svcGetProcessorID());
    fh_platform_panic(message);
    svcExitProcess();
    __builtin_unreachable();
}

void fh_platform_gpu_wait_poll(void) {
    /* Do not call quit, APT suspend, or renderer teardown inside a GPU fence.
     * A completed fence resumes the original caller before normal shutdown. */
    if(scripted)return;
    hidScanInput();u32 keys=hidKeysHeld();
    if((keys&KEY_SELECT) && !(keys&(KEY_ZL|KEY_ZR))){last_frame_requested=1;gpu_wait_exit_requested=1;}
}
void fh_platform_gpu_wait_log_flush(void) {
    if(logfile)fflush(logfile);
    (void)fh_log_io_drain(0);
}
uint64_t fh_platform_ticks(void) {
    u64 ticks=svcGetSystemTick();return (ticks/SYSCLOCK_ARM11)*40500000ULL+(ticks%SYSCLOCK_ARM11)*40500000ULL/SYSCLOCK_ARM11;
}
void fh_platform_cache(void *memory,size_t bytes,int cpu_written) { fh_gx_cache_range(memory,bytes,cpu_written); }
static void next_event(void) {
    has_input=fscanf(input_file,"%u %u %d %d %d %d %d %d",&next_input.frame,&next_input.buttons,&next_input.x,&next_input.y,&next_input.cx,&next_input.cy,&next_input.left,&next_input.right)==8;
}
static int clamp(int value,int minimum,int maximum) { return value<minimum?minimum:value>maximum?maximum:value; }
static void update_input(void) {
    if(gpu_wait_exit_requested)fh_platform_quit();
#ifdef FH_REPLAY_BUILD
    if(fh_replay_finished())fh_platform_quit();
#endif
    if(scripted) {
        if(!has_input){clearerr(input_file);next_event();}
        while(has_input && next_input.frame<=retraces) {
            input.buttons=next_input.buttons;input.x=clamp(next_input.x,-127,127);input.y=clamp(next_input.y,-127,127);
            input.cx=clamp(next_input.cx,-127,127);input.cy=clamp(next_input.cy,-127,127);
            input.left=clamp(next_input.left,0,255);input.right=clamp(next_input.right,0,255);next_event();
        }
        input.analog_a=input.buttons&0x100?255:0;input.analog_b=input.buttons&0x200?255:0;
        return;
    }
    hidScanInput();u32 keys=hidKeysHeld();circlePosition circle,stick;
    /* Pinned Melee uses Select for a clean homebrew exit. No game save is
     * synthesized here; pending card work is closed by platform shutdown. */
    if((hidKeysDown()&KEY_SELECT) && !(keys&(KEY_ZL|KEY_ZR))){last_frame_requested=1;fh_platform_quit();}
    hidCircleRead(&circle);hidCstickRead(&stick);memset(&input,0,sizeof(input));
    static const u32 source[]={KEY_A,KEY_B,KEY_X,KEY_Y,KEY_L,KEY_R,KEY_ZL,KEY_ZR,KEY_START,KEY_DLEFT,KEY_DRIGHT,KEY_DDOWN,KEY_DUP};
    static const u16 destination[]={0x100,0x200,0x400,0x800,0x40,0x20,0x10,0x10,0x1000,1,2,4,8};
    for(unsigned i=0;i<sizeof(source)/sizeof(source[0]);i++) if(keys&source[i]) input.buttons|=destination[i];
    input.x=clamp(circle.dx*127/156,-127,127);input.y=clamp(circle.dy*127/156,-127,127);
    input.cx=clamp(stick.dx*127/156,-127,127);input.cy=clamp(stick.dy*127/156,-127,127);
    if(!fh_hardware_is_new){
        static FhTouchCstick touch;
        touchPosition position={0};
        if(keys&KEY_TOUCH)hidTouchRead(&position);
        fh_touch_cstick_update(&touch,&input,(keys&KEY_TOUCH)!=0,position.px,position.py);
    }
    {
        touchPosition touch={0};if(keys&KEY_TOUCH)hidTouchRead(&touch);
        unsigned graphics_locked=fh_record_active();
#ifdef FH_REPLAY_BUILD
        graphics_locked|=fh_replay_active();
#endif
        fh_graphics_options_touch((keys&KEY_TOUCH)!=0,touch.px,touch.py,graphics_locked);
        fh_perf_display_poll();
        if(fh_hardware_is_new)fh_record_touch((keys&KEY_TOUCH)!=0,touch.px,touch.py);
        u16 w,h;uint16_t*bottom=(uint16_t*)gfxGetFramebuffer(GFX_BOTTOM,GFX_LEFT,&w,&h);
        unsigned changed=fh_graphics_options_draw(bottom,0);
        if(fh_hardware_is_new)changed|=fh_record_draw(bottom,0);
        if(changed)gfxScreenSwapBuffers(GFX_BOTTOM,false);
    }
    input.left=keys&KEY_L?255:0;input.right=keys&KEY_R?255:0;
    input.analog_a=keys&KEY_A?255:0;input.analog_b=keys&KEY_B?255:0;
}
void fh_platform_pad(FhPadState *state) {
    *state=input;
    fh_record_pad(state);
#ifdef FH_REPLAY_BUILD
    fh_replay_pad(state);
#endif
}
void fh_platform_frame(void) {
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    unsigned replay_phase=fh_replay_phase(FH_PHASE_PRESENT);
#endif
#ifdef FH_EMULATOR_TEST
    static FILE *phases;static u64 previous_end,previous_frame_start,previous_vblank;
    u64 phase[7];phase[0]=svcGetSystemTick();
#endif
    if(retraces<32)fh_platform_log("boot frame APT enter\n");
    if(!aptMainLoop()) fh_platform_quit();
    if(retraces<32)fh_platform_log("boot frame APT returned\n");
    fh_platform_cpu_poll();
    fh_renderer_present();
    if(retraces<32)fh_platform_log("boot frame present returned; wait VBlank\n");
#ifdef FH_EMULATOR_TEST
    phase[1]=svcGetSystemTick();
#endif
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(FH_PHASE_VBLANK);
#endif
    /* Native rendering retains FrameBegin's GPU/resource retirement fence.
     * A VBlank that passed during game work already satisfies display pacing;
     * forcing another edge here needlessly serializes the next simulation.
     * Do not borrow missed retraces or change the game's elapsed-time clock. */
    if(fh_pica_scene_enabled && frame_pacing_enabled){
        u32 now=C3D_FrameCounter(0);
        if(fh_frame_pacing_wait(&frame_pacing,now)){
            gspWaitForVBlank();++frame_pacing_waits;now=C3D_FrameCounter(0);
        }else ++frame_pacing_ready;
        fh_frame_pacing_consume(&frame_pacing,now);
    }else gspWaitForVBlank();
    ++retraces;
    if(retraces<32)fh_platform_log("boot frame VBlank returned; read input\n");
#ifdef FH_EMULATOR_TEST
    phase[2]=svcGetSystemTick();
#endif
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(FH_PHASE_INPUT);
#endif
    update_input();
    if(retraces<32)fh_platform_log("boot frame input returned\n");
#ifdef FH_EMULATOR_TEST
    phase[3]=svcGetSystemTick();
    fh_game_observe(retraces);
    phase[4]=svcGetSystemTick();
#endif
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(FH_PHASE_AUDIO_OBSERVE);
#endif
    fh_platform_audio_observe(retraces);
#ifdef FH_EMULATOR_TEST
    phase[5]=svcGetSystemTick();
#endif
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(FH_PHASE_METRICS);
#endif
    if(retraces%30==0 || (retraces<30 && retraces%5==0)) { char message[80];snprintf(message,sizeof(message),"retrace=%u buttons=%u\n",retraces,input.buttons);fh_platform_log(message);metrics(0); }
    if(retraces<32)fh_platform_log("boot frame return to game\n");
    fh_sd_stats_observe(retraces);
#ifdef FH_EMULATOR_TEST
    /* Keep host control out of game/service timing. An already open byte
     * channel avoids repeated failed SD opens; this never exists on console. */
    if(host_control && !(retraces&3)) {
        rewind(host_control);
        if(fgetc(host_control)=='1')fh_platform_quit();
    }
    phase[6]=svcGetSystemTick();
      static u64 previous_audio,previous_gpu,previous_video,previous_services[PROFILE_SERVICE_COUNT];
    extern uint64_t fh_audio_pump_ticks,fh_video_decode_ticks;
    extern u64 fh_gpu_begin_ticks;
    if(!phases){phases=fopen("sdmc:/3ds/foxhollow/frame-phases.csv","wb");if(phases)fputs("retrace,game_and_services_ms,present_ms,vblank_ms,input_ms,state_ms,audio_observe_ms,metrics_ms,audio_pump_ms,gpu_begin_ms,audio_update_ms,frame_setup_ms,load_files_ms,ui_end_ms,text_ms,sfx_ms,ui_start_ms,video_decode_ms,display_lists_ms,renderer_calls_ms,object_update_ms,environment_ms,collision_ms,scene_render_ms,ui_draw_ms,reflection_small_ms,reflection_update_ms,distortion_ms,particles_ms,object_render_ms,whole_interval_ms,vblank_interval_ms\n",phases);}
    if(phases){
        fprintf(phases,"%u,%.3f",retraces,previous_end?(phase[0]-previous_end)*1000.0/SYSCLOCK_ARM11:0);
        for(unsigned i=0;i<6;i++)fprintf(phases,",%.3f",(phase[i+1]-phase[i])*1000.0/SYSCLOCK_ARM11);
        fprintf(phases,",%.3f,%.3f",(fh_audio_pump_ticks-previous_audio)/40500.0,(fh_gpu_begin_ticks-previous_gpu)*1000.0/SYSCLOCK_ARM11);
          for(unsigned i=0;i<7;i++)fprintf(phases,",%.3f",(service_ticks[i]-previous_services[i])*1000.0/SYSCLOCK_ARM11);
          fprintf(phases,",%.3f",(fh_video_decode_ticks-previous_video)/40500.0);
          for(unsigned i=7;i<PROFILE_SERVICE_COUNT;i++)fprintf(phases,",%.3f",(service_ticks[i]-previous_services[i])*1000.0/SYSCLOCK_ARM11);
        /* Start-to-start includes CSV formatting/write overhead omitted by
         * game/services. Component sums alone must not establish cadence. */
        fprintf(phases,",%.3f,%.3f\n",previous_frame_start?(phase[0]-previous_frame_start)*1000.0/SYSCLOCK_ARM11:0,
            previous_vblank?(phase[2]-previous_vblank)*1000.0/SYSCLOCK_ARM11:0);
        if(retraces%16==0)fflush(phases);
    }
    previous_end=svcGetSystemTick();
    previous_frame_start=phase[0];
    previous_vblank=phase[2];
    previous_audio=fh_audio_pump_ticks;previous_gpu=fh_gpu_begin_ticks;previous_video=fh_video_decode_ticks;memcpy(previous_services,service_ticks,sizeof(service_ticks));
    if(limit && retraces>=limit) {
        FILE *file=fopen("sdmc:/3ds/foxhollow/completed.txt","wb");if(file) { fprintf(file,"retrace=%u\n",retraces);fclose(file); }
        fh_platform_quit();
    }
#endif
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(replay_phase);
#endif
}
int main(void) {
    fh_hardware_init();
    /* Replay-only A/B control; ordinary game configuration is untouched. */
#if defined(FH_REPLAY_BUILD) && !defined(FH_UNIFIED_APP)
    {FILE *f=fopen("sdmc:/3ds/foxhollow-replay/frame_pacing.txt","r");
    if(f){int c=fgetc(f);if(c=='0'||c=='1')frame_pacing_enabled=c-'0';fclose(f);}}
#endif
    fh_sd_stats_init();
    stack_probe_init();
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow",0777);
#ifdef FH_INSTALL_STARTUP
    if(!fh_install_boot(&fh_install_game_spec,FH_INSTALL_BUILD_ID,fh_hardware_is_new,sd_bench_diagnostics,1))__real_exit(0);
#elif defined(FH_REPLAY_BUILD)
    if(!fh_replay_menu(&retraces))__real_exit(0);
#endif
    fh_sd_stats_begin_run();
    remove("sdmc:/3ds/foxhollow/previous-native.log");
    rename("sdmc:/3ds/foxhollow/native.log","sdmc:/3ds/foxhollow/previous-native.log");
    remove("sdmc:/3ds/foxhollow/previous-last-frame-top.ppm");
    rename("sdmc:/3ds/foxhollow/last-frame-top.ppm","sdmc:/3ds/foxhollow/previous-last-frame-top.ppm");
    logfile=fopen("sdmc:/3ds/foxhollow/native.log","wb");
    if(fh_diagnostic_buffer(logfile))fh_platform_panic("Log buffer allocation failed");
#if !defined(FH_REPLAY_BUILD) && !defined(FH_INSTALL_STARTUP)
    fh_graphics_settings_load("sdmc:/3ds/foxhollow");
#endif
    fh_graphics_geometry_init();
    measurements=fopen("sdmc:/3ds/foxhollow/memory.jsonl","wb");
    if(fh_diagnostic_buffer(measurements))fh_platform_panic("Memory telemetry buffer allocation failed");
    freopen("sdmc:/3ds/foxhollow/game.log","wb",stdout);setvbuf(stdout,NULL,_IOLBF,4096);
    bool is_new=fh_hardware_is_new;
    fh_platform_log(is_new?"Unified target: New 3DS\n":"Unified target: Old 3DS\n");
    char heap_profile_log[120];
    snprintf(heap_profile_log,sizeof(heap_profile_log),"Memory profile: %lu MiB normal / %lu MiB linear\n",
        (unsigned long)(__ctru_heap_size/(1024*1024)),(unsigned long)(__ctru_linear_heap_size/(1024*1024)));
    fh_platform_log(heap_profile_log);
    fh_platform_cpu_init(is_new);
    gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false);
    /* gfxInit allocates but does not clear either LCD's double buffers.
     * Melee initializes a bottom console; we keep stdout in game.log and
     * explicitly initialize both buffers before Citro3D owns top swaps. */
    for(unsigned buffer=0;buffer<2;buffer++) {
        for(unsigned screen=GFX_TOP;screen<=GFX_BOTTOM;screen++) {
            u16 width,height;
            u8 *pixels=gfxGetFramebuffer((gfxScreen_t)screen,GFX_LEFT,&width,&height);
            size_t bytes=(size_t)width*height*(screen==GFX_TOP?4:2);
            memset(pixels,0,bytes);
            if(screen==GFX_BOTTOM)fh_graphics_options_draw((uint16_t*)pixels,1);
            if(screen==GFX_BOTTOM && !is_new)fh_touch_cstick_draw((uint16_t*)pixels);
            if(screen==GFX_BOTTOM && is_new)fh_record_draw((uint16_t*)pixels,1);
            if(R_FAILED(GSPGPU_FlushDataCache(pixels,bytes)))fh_platform_panic("LCD initialization cache flush failed");
            gfxScreenSwapBuffers((gfxScreen_t)screen,false);
        }
        gspWaitForVBlank();
    }
    FILE *detail_config=fopen("sdmc:/3ds/foxhollow/fur_shells.txt","r");
    /* The old render_disabled.txt modes no longer select a backend. */
#ifdef FH_REPLAY_BUILD
    if(fh_replay_active()){extern unsigned fh_replay_render_mode;
    fh_pica_scene_textured=fh_replay_render_mode>=8;
    fh_pica_scene_actors=fh_replay_render_mode==9;}
#endif
    fh_platform_log("PICA-only renderer: legacy backend selection removed; fidelity incomplete\n");
    {FILE *f=fopen("sdmc:/3ds/foxhollow/audio_pipeline.txt","r");
    if(f){fh_audio_pipeline_enabled=fgetc(f)=='1';fclose(f);}}
    {FILE *f=fopen("sdmc:/3ds/foxhollow/movie_prefetch.txt","r");
    if(f){int mode=fgetc(f);if(mode=='1'||mode=='2')fh_movie_prefetch_mode=mode-'0';fclose(f);}}
    {FILE *f=fopen("sdmc:/3ds/foxhollow/surface_detail.txt","r");
    if(f){unsigned mode;if(fscanf(f,"%u",&mode)==1 && mode<=1)fh_surface_detail=mode;fclose(f);}}
    {extern unsigned fh_fragment_batch_enabled;
    FILE *f=fopen("sdmc:/3ds/foxhollow/fragment_batch.txt","r");
    if(f){unsigned mode;if(fscanf(f,"%u",&mode)==1 && mode<=1)fh_fragment_batch_enabled=mode;fclose(f);}}
    if(detail_config){unsigned budget;if(fscanf(detail_config,"%u",&budget)==1 && (budget==0 || (budget>=2 && budget<=16)))fh_fur_shell_budget=budget;fclose(detail_config);}
    fh_platform_log("Foxhollow native ARM game startup\n");metrics(1);
#ifndef FH_EMULATOR_TEST
    /* Physical validation candidate: isolate either change with an explicit
     * zero file. Game assets, saves and simulation settings are unaffected. */
    extern unsigned fh_audio_synth_jobs_enabled,fh_audio_cpu_cache_enabled;
    fh_audio_synth_jobs_enabled=fh_audio_cpu_cache_enabled=1;
    FILE *audio_config=fopen("sdmc:/3ds/foxhollow/audio_synth_jobs.txt","r");
    if(audio_config){fh_audio_synth_jobs_enabled=fgetc(audio_config)=='1';fclose(audio_config);}
    audio_config=fopen("sdmc:/3ds/foxhollow/audio_cpu_cache.txt","r");
    if(audio_config){fh_audio_cpu_cache_enabled=fgetc(audio_config)=='1';fclose(audio_config);}
#ifdef FH_SHARED_WORKER_CONSOLE
    /* Explicit hardware experiment after shared-owner lifecycle/menu tests.
     * A zero file selects direct rendering in this same binary for comparison. */
    extern unsigned fh_gx_dispatch_enabled;
    fh_audio_shared_executor_enabled=fh_hardware_is_new && fh_audio_synth_jobs_enabled!=0;
    audio_config=fopen("sdmc:/3ds/foxhollow/shared_worker.txt","r");
    if(audio_config){fh_audio_shared_executor_enabled&=fgetc(audio_config)=='1';fclose(audio_config);}
    fh_gx_dispatch_enabled=fh_audio_shared_executor_enabled?2:0;
    /* Promote the integrated, checked model/GX preparation path. Keep the
     * same source-reader joins, GPU transfers and core-2 executor. This does
     * not enable cross-scope source borrowing or the GPU-copy experiment. */
    unsigned prepared=1;
    audio_config=fopen("sdmc:/3ds/foxhollow/render_preparation.txt","r");
    if(audio_config){prepared=fgetc(audio_config)=='1';fclose(audio_config);}
    prepared&=fh_hardware_is_new;
    if(prepared){
        extern unsigned fh_gx_state_batch_mode,fh_palette_scope_mode,fh_fur_post_mode;
        extern unsigned fh_fur_coefficients_mode,fh_movie_batch_enabled;
        fh_gx_dispatch_batch_enabled=1;
        fh_gx_state_batch_mode=1;
        fh_gx_matrix_block_enabled=1;
        fh_retained_key_mode=1;
        fh_retained_model_scope_enabled=2;
        fh_palette_scope_mode=fh_fur_post_mode=fh_fur_coefficients_mode=1;
        fh_material_scope_mode=1;
        fh_movie_batch_enabled=1;
    }
    fh_platform_log(prepared?"Prepared model/GX backend enabled (resource generations off; GPU copy fences retained)\n":
        "Legacy model/GX preparation selected\n");
    fh_platform_log(fh_audio_shared_executor_enabled?
        "Physical validation: shared audio/GX owner requested\n":
        "Physical validation: direct GX selected\n");
#endif
#endif
#ifdef FH_EMULATOR_TEST
    input_file=fopen("sdmc:/3ds/foxhollow/input.txt","r");
    FILE *batch_config=fopen("sdmc:/3ds/foxhollow/gx_dispatch_batch.txt","r");
    if(batch_config){fh_gx_dispatch_batch_enabled=fgetc(batch_config)=='1';fclose(batch_config);}
    extern unsigned fh_gx_state_batch_mode;
    extern unsigned fh_gx_owned_sources_mode;
    batch_config=fopen("sdmc:/3ds/foxhollow/gx_owned_sources.txt","r");
    if(batch_config){unsigned mode=0;if(fscanf(batch_config,"%u",&mode)==1 && mode<=2)fh_gx_owned_sources_mode=mode;fclose(batch_config);}
    batch_config=fopen("sdmc:/3ds/foxhollow/gx_state_batch.txt","r");
    if(batch_config){unsigned mode=0;if(fscanf(batch_config,"%u",&mode)==1 && mode<=2)fh_gx_state_batch_mode=mode;fclose(batch_config);}
    if(input_file) { scripted=1;next_event(); }
    FILE *config=fopen("sdmc:/3ds/foxhollow/run_frames.txt","r");if(config) { if(fscanf(config,"%u",&limit)!=1) limit=0;fclose(config); }
    config=fopen("sdmc:/3ds/foxhollow/material_scope.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_material_scope_mode=mode;fclose(config);}
    host_control=fopen("sdmc:/3ds/foxhollow/stop-request.txt","rb");
    extern unsigned fh_compiled_fast_mode;
    extern unsigned fh_palette_scope_mode,fh_fur_post_mode;
    extern unsigned fh_texgen_lazy_mode;
    config=fopen("sdmc:/3ds/foxhollow/lazy_texgen.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_texgen_lazy_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/retained_sets.txt","r");if(config){fh_retained_two_sets=fgetc(config)=='1';fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/retained_ranges.txt","r");if(config){fh_retained_ranges_enabled=fgetc(config)=='1';fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/retained_key.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_retained_key_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/model_resources.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_model_resources_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/palette_scope.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_palette_scope_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/fur_post.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_fur_post_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/fast_decode.txt","r");if(config){unsigned mode=1;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_compiled_fast_mode=mode;fclose(config);}
    extern unsigned fh_post_uvgen_enabled;
    config=fopen("sdmc:/3ds/foxhollow/post_uvgen.txt","r");if(config){fh_post_uvgen_enabled=fgetc(config)=='1';fclose(config);}
    extern unsigned fh_compiled_native_mode;
    extern unsigned fh_compiled_compact_mode;
    config=fopen("sdmc:/3ds/foxhollow/compact_vertices.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_compiled_compact_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/native_decode.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_compiled_native_mode=mode;fclose(config);}
    extern unsigned fh_bounds_hint_mode;
    extern unsigned fh_bounds_pair_enabled;
    extern unsigned fh_audio_cpu_cache_enabled;
    extern unsigned fh_audio_synth_jobs_enabled;
    config=fopen("sdmc:/3ds/foxhollow/audio_synth_jobs.txt","r");if(config){fh_audio_synth_jobs_enabled=fgetc(config)=='1';fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/audio_cpu_cache.txt","r");if(config){fh_audio_cpu_cache_enabled=fgetc(config)=='1';fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/bounds_pair.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=1)fh_bounds_pair_enabled=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/bounds_hint.txt","r");if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1 && mode<=2)fh_bounds_hint_mode=mode;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/geometry_scope.txt","r");
    if(config){unsigned mode=0;if(fscanf(config,"%u",&mode)==1){fh_retained_scope_validate=mode==1;fh_retained_scope_disabled=mode==2;}fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/model_geometry_scope.txt","r");if(config){int c=fgetc(config);fh_retained_model_scope_enabled=c=='2'?2:c=='1'?1:0;fclose(config);}
    extern unsigned fh_gx_dispatch_enabled;
    config=fopen("sdmc:/3ds/foxhollow/gx_dispatch.txt","r");if(config){int c=fgetc(config);fh_gx_dispatch_enabled=c=='2'?2:c=='1'?1:0;fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/shared_worker.txt","r");if(config){fh_audio_shared_executor_enabled=fgetc(config)=='1' && fh_gx_dispatch_enabled==2 && fh_hardware_is_new;fclose(config);}
    extern unsigned fh_gx_state_filter_enabled;
    config=fopen("sdmc:/3ds/foxhollow/gx_state_filter.txt","r");if(config){fh_gx_state_filter_enabled=fgetc(config)=='1';fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/gx_matrix_filter.txt","r");if(config){fh_gx_matrix_filter_enabled=fgetc(config)=='1';fclose(config);}
    config=fopen("sdmc:/3ds/foxhollow/gx_matrix_block.txt","r");if(config){fh_gx_matrix_block_enabled=fgetc(config)=='1';fclose(config);}
    if(host_control)setvbuf(host_control,NULL,_IONBF,0);
    config=fopen("sdmc:/3ds/foxhollow/profile_display_lists.txt","rb");
    if(config){if(fscanf(config,"%u %u %u",&list_profile_first,&list_profile_last,&list_profile_render_domain)<2 || list_profile_render_domain>1 || !list_profile_first || list_profile_last<list_profile_first || list_profile_last-list_profile_first>60)list_profile_first=0;fclose(config);}
#endif
    update_input();gameMain();fh_platform_quit();
}
