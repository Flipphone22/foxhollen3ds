#include "fh_hardware.h"
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fh_audio_worker.h"
#include "fh_audio_controls.h"
#include "fh_audio_bench_mode.h"
#include "fh_stack_watermark.h"
#include "fh_platform.h"
/* Foxhollow's SDL MusyX backend uses a recursive lock for synth callbacks and
 * game updates. Adapt that contract with pinned Melee core-2 worker ownership,
 * release/acquire stop publication and a joined shutdown. Only the native
 * MusyX DMA callback is worker-eligible; movie/stream callbacks stay on main. */
enum { STACK_BYTES=96*1024 };
unsigned fh_audio_shared_executor_enabled;
static unsigned worker_stack_bytes;
static int (*extra_step)(void);
static u64 service_tick;
static unsigned audio_fp_control;
static RecursiveLock synth_lock;
static unsigned lock_ready,stopping,suspended,jobs,stack_peak,processor=~0u;
static Thread worker;
static LightEvent wake;
static aptHookCookie hook_cookie;
static int (*mix_step)(void);
extern void fh_audio_drain_pending(void) __attribute__((weak));
void fh_audio_worker_wake(void){if(worker)LightEvent_Signal(&wake);}
static void initialize_lock(void) {
    /* Called by the producer before thread creation; immutable thereafter. */
    if(!lock_ready){RecursiveLock_Init(&synth_lock);lock_ready=1;}
}
/* Protected by synth_lock, including recursive acquisitions. A drain
 * occurs only at an outer owner/API boundary, never halfway through a block. */
static unsigned synth_depth;
static uintptr_t control_producer,synth_owner;
int fh_audio_controls_can_queue(void){
    uintptr_t self=(uintptr_t)getThreadLocalStorage();
    return __atomic_load_n(&fh_audio_controls_enabled,__ATOMIC_ACQUIRE) &&
        self==control_producer && self!=__atomic_load_n(&synth_owner,__ATOMIC_ACQUIRE);
}
static void acquired(void){
    if(++synth_depth==1){
        __atomic_store_n(&synth_owner,(uintptr_t)getThreadLocalStorage(),__ATOMIC_RELEASE);
        if(__atomic_load_n(&fh_audio_controls_enabled,__ATOMIC_ACQUIRE))fh_audio_controls_drain();
    }
}
void fh_audio_lock(void) {initialize_lock();RecursiveLock_Lock(&synth_lock);acquired();}
int fh_audio_try_lock(void) {initialize_lock();if(RecursiveLock_TryLock(&synth_lock))return 0;acquired();return 1;}
void fh_audio_unlock(void) {if(!--synth_depth)__atomic_store_n(&synth_owner,0,__ATOMIC_RELEASE);RecursiveLock_Unlock(&synth_lock);}
int fh_audio_worker_active(void) {return worker!=NULL;}
int fh_audio_worker_is_current(void){return worker && threadGetCurrent()==worker;}
unsigned fh_audio_worker_stack_bytes(void){return worker?worker_stack_bytes:0;}
int fh_audio_worker_attach(int (*step)(void)){
    if(!worker || !fh_audio_shared_executor_enabled || !step || __atomic_load_n(&extra_step,__ATOMIC_ACQUIRE))return 0;
    __atomic_store_n(&extra_step,step,__ATOMIC_RELEASE);fh_audio_worker_wake();return 1;
}
void fh_audio_worker_detach(void){__atomic_store_n(&extra_step,NULL,__ATOMIC_RELEASE);}
static int mix_once(void){
    if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE) || __atomic_load_n(&suspended,__ATOMIC_ACQUIRE))return 0;
    /* A producer can hold the synth lock while waiting for rendering. The
     * shared owner must keep consuming GX work rather than block on that lock. */
    if(fh_audio_shared_executor_enabled){if(!fh_audio_try_lock())return 0;}else fh_audio_lock();
    int mixed=0;unsigned saved_fp=0;
    if(fh_audio_shared_executor_enabled){
        __asm__ volatile("vmrs %0, fpscr":"=r"(saved_fp));
        unsigned audio_fp=(saved_fp&~0x03c00000u)|audio_fp_control;
        __asm__ volatile("vmsr fpscr, %0"::"r"(audio_fp));
    }
    if(!__atomic_load_n(&stopping,__ATOMIC_ACQUIRE) && !__atomic_load_n(&suspended,__ATOMIC_ACQUIRE))mixed=mix_step();
    if(fh_audio_shared_executor_enabled)__asm__ volatile("vmsr fpscr, %0"::"r"(saved_fp));
    fh_audio_unlock();if(mixed)__atomic_add_fetch(&jobs,1,__ATOMIC_RELAXED);return mixed;
}
void fh_audio_worker_service_current(void){
    if(!fh_audio_shared_executor_enabled || !fh_audio_worker_is_current())return;
    u64 now=svcGetSystemTick();
    if(now-service_tick>=SYSCLOCK_ARM11/1000){service_tick=now;(void)mix_once();}
}
static void on_apt(APT_HookType type,void *unused) {
    (void)unused;
    if(type==APTHOOK_ONSUSPEND || type==APTHOOK_ONSLEEP) {
        __atomic_store_n(&suspended,1,__ATOMIC_RELEASE);
        fh_audio_lock();fh_audio_unlock(); /* Join any currently rendering block. */
        if(fh_audio_drain_pending)fh_audio_drain_pending();
    } else if(type==APTHOOK_ONRESTORE || type==APTHOOK_ONWAKEUP) {
        __atomic_store_n(&suspended,0,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    }
}
#ifdef FH_AUDIO_WORKER_TEST
/* Exercise the actual registered handler without driving a visible system UI.
 * This validates our barrier, not the console's suspend/service lifecycle. */
void fh_audio_worker_test_hook(unsigned type){on_apt((APT_HookType)type,NULL);}
#endif
static void work(void *unused) {
    (void)unused;__atomic_store_n(&processor,svcGetProcessorID(),__ATOMIC_RELAXED);
    __asm__ volatile("vmrs %0, fpscr":"=r"(audio_fp_control));audio_fp_control&=0x03c00000u;
    uintptr_t sp;__asm__ volatile("mov %0, sp":"=r"(sp));
    unsigned watermark_bytes=worker_stack_bytes-16*1024;
    unsigned char *low=(unsigned char*)(sp-watermark_bytes),*high=(unsigned char*)(sp-4096);
    memset(low,0xa5,high-low);
    unsigned next_probe=128;
    while(!__atomic_load_n(&stopping,__ATOMIC_ACQUIRE)) {
        int mixed=0;
        if(fh_audio_shared_executor_enabled)fh_audio_worker_service_current();else mixed=mix_once();
        int consumed=0;int (*step)(void)=__atomic_load_n(&extra_step,__ATOMIC_ACQUIRE);
        if(step && !__atomic_load_n(&suspended,__ATOMIC_ACQUIRE))consumed=step();
        unsigned n=__atomic_load_n(&jobs,__ATOMIC_RELAXED);
        if(n>=next_probe) {
                unsigned untouched=fh_stack_watermark_prefix(low,(unsigned)(high-low));
                unsigned used=watermark_bytes-untouched;if(used>stack_peak)__atomic_store_n(&stack_peak,used,__ATOMIC_RELAXED);
                if(used>=watermark_bytes-4096)fh_platform_panic("Audio worker stack headroom exhausted");
                next_probe=n+128;
        }
        if(consumed)continue;
        /* A 1 ms timed sleep after every 5 ms slice caps production at the
         * mixer cost plus that sleep. Old 3DS shares core0 and can fall behind
         * continuously. Release the synth lock and yield without imposing a
         * timer deadline while useful work remains. Idle queues still sleep. */
        if(mixed){svcSleepThread(0);continue;}
        LightEvent_WaitTimeout(&wake,1000000);
    }
}

/* Request once before creating the existing audio owner. A successful request
 * is not proof of a grant: read it back. System-core time remains reserved for
 * this process until exit; do not pretend it can be reset to zero mid-session. */
static int old_processor_cached=-1;
static unsigned old_worker_processor(void){
    /* NDSP is initialized before the mixer. Both owners share one verified
     * process grant. Startup selection is latched before either starts. */
    if(old_processor_cached>=0)return (unsigned)old_processor_cached;
    unsigned requested=fh_audio_bench_old_core(),selected=0;u32 granted=0;
    Result set=0,read=0;
    if(requested){
        u32 limits[2]={55,30};
        {FILE *f=fopen("sdmc:/3ds/foxhollow/core1_grant.txt","rb");unsigned value=0;
         if(f){if(fscanf(f,"%u",&value)!=1)value=0;fclose(f);}
         if(value>=30 && value<=80)limits[0]=value;}
        for(unsigned i=0;i<sizeof(limits)/sizeof(*limits) && !selected;i++){
            set=APT_SetAppCpuTimeLimit(limits[i]);read=0;granted=0;
            if(R_SUCCEEDED(set))read=APT_GetAppCpuTimeLimit(&granted);
            if(R_SUCCEEDED(set) && R_SUCCEEDED(read) && granted==limits[i])selected=1;
        }
    }
    char line[160];snprintf(line,sizeof line,
        "Old audio CPU grant: requested=%u set=%08lx read=%08lx granted=%lu selected=%u\n",
        requested,(unsigned long)(u32)set,(unsigned long)(u32)read,(unsigned long)granted,selected);
    fh_platform_log(line);old_processor_cached=(int)selected;return selected;
}

int fh_audio_ndsp_processor(void){
    return fh_hardware_is_new?-2:(int)old_worker_processor();
}

void fh_audio_worker_start(int (*step)(void)) {
    if(worker)return;
    initialize_lock();
    unsigned enabled=1;
#ifdef FH_EMULATOR_TEST
    enabled=0;
#endif
    /* Console candidate after lifecycle and Palace integration; an explicit
     * zero file retains synchronous mixing for physical fault isolation. */
    FILE *f=fopen("sdmc:/3ds/foxhollow/audio_worker.txt","rb");
    if(f){enabled=fgetc(f)=='1';fclose(f);}if(!enabled)return;
    (void)fh_platform_audio_space(0); /* Initialize NDSP on the producer. */
    mix_step=step;stopping=suspended=jobs=stack_peak=0;
    service_tick=0;worker_stack_bytes=fh_audio_shared_executor_enabled?256*1024:STACK_BYTES;
    LightEvent_Init(&wake,RESET_ONESHOT);
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    if(!fh_hardware_is_new){

    unsigned core=old_worker_processor();
    worker=threadCreate(work,NULL,worker_stack_bytes,priority,core,false);
    if(!worker && core){
        fh_platform_log("Old audio core1 creation failed; retaining core0 owner\n");
        core=0;worker=threadCreate(work,NULL,worker_stack_bytes,priority,core,false);
    }
    if(worker){char line[96];snprintf(line,sizeof line,
        "Old 3DS audio worker selected core %u; existing queue and stack\n",core);fh_platform_log(line);}
    else fh_platform_log("Old 3DS worker unavailable; synchronous audio retained\n");
    control_producer=(uintptr_t)getThreadLocalStorage();
    __atomic_store_n(&fh_audio_controls_enabled,worker && core==1,__ATOMIC_RELEASE);
    if(worker)aptHook(&hook_cookie,on_apt,NULL);
    }else{

    worker=threadCreate(work,NULL,worker_stack_bytes,priority,2,false);
    if(worker){aptHook(&hook_cookie,on_apt,NULL);fh_platform_log(fh_audio_shared_executor_enabled?"Shared audio/GX owner created on core 2; 256 KiB stack\n":"MusyX worker created on core 2; 96 KiB stack\n");}
    else fh_platform_log("MusyX core 2 unavailable; synchronous audio retained\n");
    }

}
void fh_audio_worker_stop(void) {
    if(!worker)return;
    if(__atomic_load_n(&extra_step,__ATOMIC_ACQUIRE))fh_platform_panic("Audio worker stopped before attached rendering drained");
    __atomic_store_n(&fh_audio_controls_enabled,0,__ATOMIC_RELEASE);
    aptUnhook(&hook_cookie);__atomic_store_n(&stopping,1,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    if(R_FAILED(threadJoin(worker,3000000000ULL)))fh_platform_panic("Audio worker failed to stop");
    if(fh_audio_drain_pending)fh_audio_drain_pending();
    threadFree(worker);worker=NULL;mix_step=NULL;
}
void fh_audio_worker_stats(unsigned *active,unsigned *count,unsigned *stack,unsigned *core) {
    *active=worker!=NULL;*count=__atomic_load_n(&jobs,__ATOMIC_RELAXED);
    *stack=__atomic_load_n(&stack_peak,__ATOMIC_RELAXED);*core=__atomic_load_n(&processor,__ATOMIC_RELAXED);
}
