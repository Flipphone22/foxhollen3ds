#include "fh_hardware.h"
#include "fh_gx_dispatch.h"
#ifdef FH_GX_DISPATCH_BUILD
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fh_platform.h"
#include "fh_audio_worker.h"
enum { STACK_BYTES=256*1024 };
static Thread worker;
static unsigned shared_owner;
unsigned fh_gx_worker_busy_us,fh_gx_producer_wait_us;
static LightEvent wake;
static unsigned stopping,failed;
static unsigned processor=~0u,stack_peak;
static char failure[160];
static aptHookCookie apt_cookie;
extern int fh_gxd_pending(void),fh_gxd_consume_batch(unsigned);
int fh_gx_dispatch_is_worker(void){return shared_owner?fh_audio_worker_is_current():worker && threadGetCurrent()==worker;}
extern void fh_renderer_service_audio(void);
void fh_gxd_platform_wake(void){if(shared_owner)fh_audio_worker_wake();else if(worker)LightEvent_Signal(&wake);}
void fh_gxd_platform_wait(void){
    if(fh_gx_dispatch_is_worker())fh_platform_panic("GX worker attempted to join itself");
    u64 start=svcGetSystemTick();
    while(fh_gxd_pending()){
        if(__atomic_load_n(&failed,__ATOMIC_ACQUIRE))fh_platform_panic(failure);
        if(svcGetSystemTick()-start>5ULL*SYSCLOCK_ARM11)fh_platform_panic("GX state command worker timeout");
        /* Rendering must not invoke game/stream callbacks from the reader.
         * Keep those on the producer while it waits for borrowed sources. */
        fh_renderer_service_audio();
        svcSleepThread(100000);
    }
    if(__atomic_load_n(&failed,__ATOMIC_ACQUIRE))fh_platform_panic(failure);
    /* Bounded-run telemetry; unsigned microseconds wrap after about71minutes. */
    fh_gx_producer_wait_us+=(u32)((svcGetSystemTick()-start)*1000000ULL/SYSCLOCK_ARM11);
}
void fh_gx_dispatch_worker_panic(const char *message){
    if(!fh_gx_dispatch_is_worker())return;
    snprintf(failure,sizeof(failure),"GX state worker: %.140s",message);
    __atomic_store_n(&failed,1,__ATOMIC_RELEASE);threadExit(1);
}
static int consume_slice(void){
    u64 start=svcGetSystemTick();unsigned packets=fh_gxd_consume_batch(64);
    if(packets)__atomic_add_fetch(&fh_gx_worker_busy_us,(u32)((svcGetSystemTick()-start)*1000000ULL/SYSCLOCK_ARM11),__ATOMIC_RELAXED);
    return packets;
}
static void work(void *unused){
    (void)unused;
    __atomic_store_n(&processor,svcGetProcessorID(),__ATOMIC_RELAXED);
    uintptr_t sp;__asm__ volatile("mov %0, sp":"=r"(sp));
    /* Known 256 KiB allocation; leave 32 KiB below and 4 KiB below entry SP.
     * Same bounded stack watermark method as the audited audio worker. */
    unsigned char *low=(unsigned char*)(sp-224*1024),*high=(unsigned char*)(sp-4096);
    memset(low,0xa5,high-low);unsigned packets=0,next_probe=4096;
    while(!__atomic_load_n(&stopping,__ATOMIC_ACQUIRE)){
        int consumed;while((consumed=consume_slice()))packets+=consumed;
        if(packets>=next_probe){
            unsigned untouched=0;while(low+untouched<high && low[untouched]==0xa5)++untouched;
            unsigned used=224*1024-untouched;
            __atomic_store_n(&stack_peak,used,__ATOMIC_RELAXED);
            if(used>=216*1024)fh_platform_panic("GX worker stack headroom exhausted");
            next_probe=packets+65536;
        }
        /* Timed wait also bounds a missed notification at the empty boundary. */
        LightEvent_WaitTimeout(&wake,1000000);
    }
}
static void on_apt(APT_HookType type,void *unused){
    (void)unused;
    if(type==APTHOOK_ONSUSPEND || type==APTHOOK_ONSLEEP)fh_gx_dispatch_barrier();
}
int fh_gxd_platform_start(void){
    if(!fh_hardware_is_new)return 0; /* No core2; native PICA submission remains on the producer. */
    if(worker || shared_owner)return 1;
    if(fh_audio_worker_active()){
        if(!fh_audio_shared_executor_enabled){fh_platform_log("GX state worker declined: audio owns core 2\n");return 0;}
        stopping=failed=0;stack_peak=0;
        /* Renderer initialization happened before this call. Register the
         * source/GPU drain after Citro3D's APT hook, as pinned Melee does. */
        shared_owner=1;
        if(!fh_audio_worker_attach(consume_slice)){shared_owner=0;return 0;}
        aptHook(&apt_cookie,on_apt,NULL);
        fh_platform_log("GX leased translation attached to existing core-2 audio owner\n");return 1;
    }
    LightEvent_Init(&wake,RESET_ONESHOT);stopping=failed=0;
    stack_peak=0;
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    worker=threadCreate(work,NULL,STACK_BYTES,priority,2,false);
    if(!worker){fh_platform_log("GX state worker unavailable; direct calls retained\n");return 0;}
    aptHook(&apt_cookie,on_apt,NULL);
    fh_platform_log(fh_gx_dispatch_enabled>=2?
        "Experimental GX worker core 2; ring=262144 stack=262144; leased draws enabled\n":
        "Experimental GX state worker core 2; ring=262144 stack=262144; serialized draws\n");return 1;
}
void fh_gx_dispatch_stop(void){
    if(!worker && !shared_owner)return;
    if(!__atomic_load_n(&failed,__ATOMIC_ACQUIRE))fh_gx_dispatch_end();
    if(shared_owner){
        /* read==published retires every source reader and GPU submission. A
         * racing empty consume_slice may return later; its static queue lives
         * until process exit. The audio thread is joined by audio shutdown. */
        aptUnhook(&apt_cookie);fh_audio_worker_detach();shared_owner=0;return;
    }
    aptUnhook(&apt_cookie);__atomic_store_n(&stopping,1,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    if(R_FAILED(threadJoin(worker,3000000000ULL)))fh_platform_panic("GX state worker failed to stop");
    threadFree(worker);worker=NULL;
}
void fh_gx_dispatch_stats(unsigned *active,unsigned *core,unsigned *peak){
    if(shared_owner){unsigned jobs;fh_audio_worker_stats(active,&jobs,peak,core);return;}
    *active=worker!=NULL;*core=__atomic_load_n(&processor,__ATOMIC_RELAXED);*peak=__atomic_load_n(&stack_peak,__ATOMIC_RELAXED);
}
#else
unsigned fh_gx_worker_busy_us,fh_gx_producer_wait_us;
void fh_gx_dispatch_stop(void){}
void fh_gx_dispatch_worker_panic(const char *message){(void)message;}
void fh_gx_dispatch_stats(unsigned *active,unsigned *core,unsigned *peak){*active=0;*core=~0u;*peak=0;}
int fh_gx_dispatch_is_worker(void){return 0;}
#endif
