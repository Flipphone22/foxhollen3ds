#include "fh_hardware.h"
#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include "fh_fragment_raster.h"
#include "fh_platform.h"

/* Pinned Melee render_worker.c: explicit New3DS core 2, release/acquire
 * publication, joined ownership, and synchronous fallback. Unlike its queued
 * renderer, this worker never calls Citro3D, allocates, logs, or runs game/audio
 * callbacks. The producer remains inside this call and services audio only. */
enum { STACK_BYTES=64*1024 };
static Thread worker;
static LightEvent wake,done;
static FhFragmentRaster *pending;
static unsigned published,completed,stopping,core=~0u,jobs,stack_peak;
static void work(void *unused) {
    (void)unused;__atomic_store_n(&core,svcGetProcessorID(),__ATOMIC_RELAXED);
    uintptr_t sp;__asm__ volatile("mov %0, sp":"=r"(sp));
    /* Probe an unused lower portion of this known 64 KiB stack. Keep 4 KiB
     * below the entry SP untouched and at least 16 KiB below the probe. */
    unsigned char *low=(unsigned char*)(sp-48*1024),*high=(unsigned char*)(sp-4096);
    memset(low,0xa5,high-low);
    for(;;) {
        LightEvent_Wait(&wake);
        if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))break;
        unsigned serial=__atomic_load_n(&published,__ATOMIC_ACQUIRE);
        if(serial==__atomic_load_n(&completed,__ATOMIC_RELAXED))continue;
        fh_fragment_raster_rows(pending,0,pending->height);
        unsigned untouched=0;while(low+untouched<high && low[untouched]==0xa5)untouched++;
        unsigned used=48*1024-untouched;if(used>stack_peak)__atomic_store_n(&stack_peak,used,__ATOMIC_RELAXED);
        __atomic_add_fetch(&jobs,1,__ATOMIC_RELAXED);
        __atomic_store_n(&completed,serial,__ATOMIC_RELEASE);
        LightEvent_Signal(&done);
    }
}
void fh_fragment_worker_start(void) {
    if(!fh_hardware_is_new)return; /* No core2; retain the existing synchronous path. */
    if(worker)return;
#ifdef FH_EMULATOR_TEST
    /* Keep opt-in until replay/integration and suspend checks establish this
     * worker's contract. Console promotion is a separate measured decision. */
    FILE *f=fopen("sdmc:/3ds/foxhollow/fragment_worker.txt","rb");if(!f)return;fclose(f);
#else
    return;
#endif
    LightEvent_Init(&wake,RESET_ONESHOT);LightEvent_Init(&done,RESET_ONESHOT);
    published=completed=stopping=jobs=stack_peak=0;
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    worker=threadCreate(work,NULL,STACK_BYTES,priority,2,false);
    fh_platform_log(worker?"CPU fragment worker created on core 2; 64 KiB stack\n":"Core 2 unavailable; synchronous CPU fragments\n");
}
void fh_fragment_worker_run(FhFragmentRaster *job,void (*service)(void)) {
    if(!worker || job->width*job->height<256) {
        for(unsigned y=0;y<job->height;y+=8){service();fh_fragment_raster_rows(job,y,y+8<job->height?y+8:job->height);}
        return;
    }
    pending=job;unsigned serial=__atomic_load_n(&published,__ATOMIC_RELAXED)+1;
    u64 started=svcGetSystemTick();
    __atomic_store_n(&published,serial,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    while(__atomic_load_n(&completed,__ATOMIC_ACQUIRE)!=serial){
        service();
        if(svcGetSystemTick()-started>20ULL*SYSCLOCK_ARM11)fh_platform_panic("CPU fragment worker job exceeded 20 seconds");
        if(__atomic_load_n(&completed,__ATOMIC_ACQUIRE)!=serial)LightEvent_WaitTimeout(&done,1000000);
    }
    if(stack_peak>=44*1024)fh_platform_panic("CPU fragment worker stack headroom exhausted");
    pending=NULL;
}
void fh_fragment_worker_stats(unsigned *active,unsigned *processor,unsigned *count,unsigned *peak) {
    *active=worker!=NULL;*processor=__atomic_load_n(&core,__ATOMIC_RELAXED);
    *count=__atomic_load_n(&jobs,__ATOMIC_RELAXED);*peak=__atomic_load_n(&stack_peak,__ATOMIC_RELAXED);
}
void fh_fragment_worker_stop(void) {
    if(!worker)return;
    __atomic_store_n(&stopping,1,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    if(R_FAILED(threadJoin(worker,3000000000ULL)))fh_platform_panic("CPU fragment worker failed to stop");
    threadFree(worker);worker=NULL;
}
