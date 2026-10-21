/* Pinned Melee render_worker.c ownership, adapted to the EXISTING audio owner.
 * Only immutable native draw descriptors cross this boundary. CPU retirement
 * is independent of GPU retirement. No extra core, GPU pool or source arena.
 * Experimental opt-in until integrated/console evidence establishes benefit. */
#include "fh_audio_worker.h"
enum { PREPARED_UI, PREPARED_WORLD, PREPARED_MODEL, PREPARED_CAPACITY=32 };
typedef union { FhPreparedUi ui; FhPreparedWorld world; FhPreparedModel model; } FhPreparedPayload;
typedef struct { unsigned kind,fp; FhPreparedPayload data; } FhPreparedJob;
static FhPreparedJob *prepared_jobs;
static unsigned prepared_enabled,prepared_active,prepared_closed;
static unsigned prepared_published __attribute__((aligned(64)));
static unsigned prepared_completed __attribute__((aligned(64)));
static unsigned prepared_failed,prepared_peak,prepared_joins,prepared_jobs_count[3];
static unsigned prepared_capacity_waits;
static u64 prepared_capacity_wait_ticks;
static u64 prepared_wait_ticks,prepared_busy_ticks,prepared_prep_ticks,prepared_world_prep_ticks;
static char prepared_failure[160];
unsigned fh_minimal_worker_executed(void){return prepared_jobs_count[0]+prepared_jobs_count[1]+prepared_jobs_count[2]!=0;}
static aptHookCookie prepared_apt_cookie;
static void submit_prepared_ui(const FhPreparedUi*);
static int submit_prepared_world(const FhPreparedWorld*);
static int submit_prepared_model(const FhPreparedModel*);
static void owner_gpu_sync(void);
static int prepared_on_worker(void){
    return __atomic_load_n(&prepared_active,__ATOMIC_ACQUIRE) && fh_audio_worker_is_current();
}
void fh_minimal_worker_panic(const char *message){
    if(!prepared_on_worker())return;
    unsigned i=0;for(;i+1<sizeof(prepared_failure)&&message[i];i++)prepared_failure[i]=message[i];
    prepared_failure[i]=0;__atomic_store_n(&prepared_failed,1,__ATOMIC_RELEASE);
    threadExit(1);
}
static int __attribute__((unused)) prepared_consume(void){
    unsigned done=__atomic_load_n(&prepared_completed,__ATOMIC_RELAXED);
    if(done==__atomic_load_n(&prepared_published,__ATOMIC_ACQUIRE))return 0;
    const FhPreparedJob *job=prepared_jobs+(done%PREPARED_CAPACITY);
    unsigned saved;__asm__ volatile("vmrs %0, fpscr":"=r"(saved));
    unsigned fp=(saved&~0x03c00000u)|job->fp;
    __asm__ volatile("vmsr fpscr, %0"::"r"(fp));
    u64 start=svcGetSystemTick();
    switch(job->kind){
    case PREPARED_UI:submit_prepared_ui(&job->data.ui);break;
    case PREPARED_WORLD:(void)submit_prepared_world(&job->data.world);break;
    case PREPARED_MODEL:(void)submit_prepared_model(&job->data.model);break;
    default:fh_platform_panic("Invalid native prepared draw kind");
    }
    prepared_busy_ticks+=svcGetSystemTick()-start;
    __asm__ volatile("vmsr fpscr, %0"::"r"(saved));
    /* Last shared write: producer may reuse this slot, GPU state or buffers
     * immediately after observing completion. Empty callbacks retain no job. */
    __atomic_store_n(&prepared_completed,done+1,__ATOMIC_RELEASE);return 1;
}
/* A full resource barrier needs zero outstanding readers. Queue backpressure
 * only needs one free descriptor: draining the entire ring creates a producer
 * bubble even though the remaining immutable jobs can keep running safely. */
static void prepared_wait(unsigned max_pending){
    if(!__atomic_load_n(&prepared_active,__ATOMIC_ACQUIRE))return;
    if(prepared_on_worker())fh_platform_panic("Native draw worker attempted self join");
    unsigned published=__atomic_load_n(&prepared_published,__ATOMIC_RELAXED);
    if(__atomic_load_n(&prepared_failed,__ATOMIC_ACQUIRE))fh_platform_panic(prepared_failure);
    if(published-__atomic_load_n(&prepared_completed,__ATOMIC_ACQUIRE)<=max_pending)return;
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    unsigned replay_phase=fh_replay_phase(FH_PHASE_WORKER_WAIT);
#endif
    u64 start=svcGetSystemTick();unsigned warned=0;prepared_joins++;
    if(max_pending)prepared_capacity_waits++;
    fh_audio_worker_wake();
    while(published-__atomic_load_n(&prepared_completed,__ATOMIC_ACQUIRE)>max_pending){
        if(__atomic_load_n(&prepared_failed,__ATOMIC_ACQUIRE))fh_platform_panic(prepared_failure);
        if(svcGetSystemTick()-start>3ULL*SYSCLOCK_ARM11){
            if(!warned){warned=1;fh_platform_log("Native draw worker wait exceeds three seconds; retaining resources\n");}
            fh_platform_gpu_wait_poll();
        }
        /* No game callbacks or APT teardown while descriptors borrow storage.
         * The consumer services audio between draws and inside GPU waits. */
        svcSleepThread(100000);
    }
    u64 elapsed=svcGetSystemTick()-start;prepared_wait_ticks+=elapsed;
    if(max_pending)prepared_capacity_wait_ticks+=elapsed;
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
    fh_replay_phase(replay_phase);
#endif
}
static void prepared_join(void){prepared_wait(0);}
static void __attribute__((unused)) prepared_on_apt(APT_HookType type,void *unused){
    (void)unused;
    if(type==APTHOOK_ONSUSPEND || type==APTHOOK_ONSLEEP)prepared_join();
}
static void prepared_start(void){
    if(!fh_hardware_is_new)return;
    if(!prepared_enabled || prepared_closed || prepared_jobs || fh_minimal_frames<3 || !fh_audio_worker_active())return;
    extern u32 __ctru_heap_size,__stacksize__;
    unsigned bytes=sizeof(FhPreparedJob)*PREPARED_CAPACITY;
    if(((u64)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64)*100>(u64)__ctru_heap_size*85)return;
    prepared_jobs=malloc(bytes);if(!prepared_jobs)return;
    __atomic_store_n(&prepared_active,1,__ATOMIC_RELEASE);
    if(!fh_audio_worker_attach(prepared_consume)){
        __atomic_store_n(&prepared_active,0,__ATOMIC_RELEASE);
        free(prepared_jobs);prepared_jobs=NULL;prepared_closed=1;return;
    }
    /* Registered last: drain CPU readers before audio/Citro3D suspend hooks. */
    aptHook(&prepared_apt_cookie,prepared_on_apt,NULL);
    char line[160];snprintf(line,sizeof(line),"Native prepared worker: active=1 slots=%u bytes=%u shared_audio=1\n",PREPARED_CAPACITY,bytes);fh_platform_log(line);
}
static FhPreparedPayload *prepared_reserve(unsigned kind){
    prepared_start();if(!__atomic_load_n(&prepared_active,__ATOMIC_ACQUIRE))return NULL;
    unsigned written=__atomic_load_n(&prepared_published,__ATOMIC_RELAXED);
    if(written-__atomic_load_n(&prepared_completed,__ATOMIC_ACQUIRE)>=PREPARED_CAPACITY)prepared_wait(PREPARED_CAPACITY-1);
    FhPreparedJob *job=prepared_jobs+(written%PREPARED_CAPACITY);job->kind=kind;
    unsigned fp;__asm__ volatile("vmrs %0, fpscr":"=r"(fp));job->fp=fp&0x03c00000u;
    return &job->data;
}
static void prepared_commit(unsigned kind){
    unsigned written=__atomic_load_n(&prepared_published,__ATOMIC_RELAXED);
    unsigned depth=written+1-__atomic_load_n(&prepared_completed,__ATOMIC_ACQUIRE);
    if(depth>prepared_peak)prepared_peak=depth;
    prepared_jobs_count[kind]++;
    __atomic_store_n(&prepared_published,written+1,__ATOMIC_RELEASE);
    if(depth==1)fh_audio_worker_wake();
}
void fh_minimal_worker_stop(void){
    prepared_closed=1;
    if(!__atomic_load_n(&prepared_active,__ATOMIC_ACQUIRE))return;
    /* Audio shutdown precedes renderer shutdown. Drain both ownership layers
     * and detach before the shared owner is stopped. */
    fh_minimal_sync();aptUnhook(&prepared_apt_cookie);fh_audio_worker_detach();
    __atomic_store_n(&prepared_active,0,__ATOMIC_RELEASE);
    char line[300];snprintf(line,sizeof(line),
        "Native prepared worker result: ui=%u world=%u model=%u peak=%u joins=%u wait_ticks=%llu busy_ticks=%llu published=%u completed=%u\n",
        prepared_jobs_count[0],prepared_jobs_count[1],prepared_jobs_count[2],prepared_peak,prepared_joins,
        (unsigned long long)prepared_wait_ticks,(unsigned long long)prepared_busy_ticks,
        prepared_published,prepared_completed);fh_platform_log(line);
    snprintf(line,sizeof(line),"Native prepared backpressure: waits=%u ticks=%llu capacity=%u\n",
        prepared_capacity_waits,(unsigned long long)prepared_capacity_wait_ticks,PREPARED_CAPACITY);fh_platform_log(line);
    /* Keep small queue allocation until process exit: a detached empty callback
     * may still be returning. No main-side reset of its counters or storage. */
}
