/* Adapt pinned Melee render_worker.c/gx_thread.c: explicit New core2,
 * release/acquire publication, bounded borrowed job and join before freeing.
 * This owner exists only during ISO preparation, before the audio owner. */
#include <3ds.h>
#include <string.h>
#include "fh_install_worker.h"
enum {STACK_BYTES=128*1024,PROBE_BYTES=96*1024};
static Thread worker;
static LightEvent wake,done;
static FhInstallWork work_function;
static void*work_context;
static unsigned published,completed,stopping,fp_control,processor=~0u,jobs,peak;
static void work(void*unused){
    (void)unused;__atomic_store_n(&processor,svcGetProcessorID(),__ATOMIC_RELAXED);
#ifndef FH_INSTALL_WORKER_TEST
    __asm__ volatile("vmsr fpscr, %0"::"r"(fp_control));
    uintptr_t sp;__asm__ volatile("mov %0, sp":"=r"(sp));
    unsigned char*low=(unsigned char*)(sp-PROBE_BYTES),*high=(unsigned char*)(sp-4096);
    memset(low,0xa5,high-low);
#endif
    for(;;){
        LightEvent_Wait(&wake);
        if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))break;
        unsigned serial=__atomic_load_n(&published,__ATOMIC_ACQUIRE);
        if(serial==__atomic_load_n(&completed,__ATOMIC_RELAXED))continue;
        work_function(work_context);
#ifndef FH_INSTALL_WORKER_TEST
        unsigned untouched=0;while(low+untouched<high&&low[untouched]==0xa5)untouched++;
        unsigned used=PROBE_BYTES-untouched;if(used>peak)__atomic_store_n(&peak,used,__ATOMIC_RELAXED);
#endif
        __atomic_add_fetch(&jobs,1,__ATOMIC_RELAXED);
        __atomic_store_n(&completed,serial,__ATOMIC_RELEASE);LightEvent_Signal(&done);
    }
}
void fh_install_worker_start(unsigned hardware_new){
    if(worker)return;
    published=completed=stopping=jobs=peak=0;processor=~0u;
    if(!hardware_new)return;
    LightEvent_Init(&wake,RESET_ONESHOT);LightEvent_Init(&done,RESET_ONESHOT);
#ifndef FH_INSTALL_WORKER_TEST
    __asm__ volatile("vmrs %0, fpscr":"=r"(fp_control));
#else
    fp_control=0;
#endif
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    worker=threadCreate(work,NULL,STACK_BYTES,priority,2,false);
}
int fh_install_worker_submit(FhInstallWork fn,void*context){
    if(!worker||!fn||__atomic_load_n(&stopping,__ATOMIC_ACQUIRE)||!fh_install_worker_complete())return 0;
    work_function=fn;work_context=context;
    unsigned serial=__atomic_load_n(&published,__ATOMIC_RELAXED)+1;
    __atomic_store_n(&published,serial,__ATOMIC_RELEASE);LightEvent_Signal(&wake);return 1;
}
int fh_install_worker_complete(void){
    return !worker||__atomic_load_n(&completed,__ATOMIC_ACQUIRE)==__atomic_load_n(&published,__ATOMIC_RELAXED);
}
void fh_install_worker_wait_slice(void){if(worker&&!fh_install_worker_complete())LightEvent_WaitTimeout(&done,1000000);}
void fh_install_worker_join(void){while(!fh_install_worker_complete())fh_install_worker_wait_slice();}
void fh_install_worker_stop(void){
    if(!worker)return;
    fh_install_worker_join();__atomic_store_n(&stopping,1,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    threadJoin(worker,U64_MAX);threadFree(worker);worker=NULL;work_function=NULL;work_context=NULL;
}
void fh_install_worker_stats(FhInstallWorkerStats*s){
    s->active=worker!=NULL;s->core=__atomic_load_n(&processor,__ATOMIC_RELAXED);
    s->jobs=__atomic_load_n(&jobs,__ATOMIC_RELAXED);s->stack_bytes=worker?STACK_BYTES:0;
    s->stack_peak=__atomic_load_n(&peak,__ATOMIC_RELAXED);
}
