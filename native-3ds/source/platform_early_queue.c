/* Adapted from 2gifts/melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e.
 * GPL-3.0; see native-3ds/licenses/melee-GPL-3.0.txt and ADAPTATIONS.md. */
#define FH_ASYNC_PRESENTATION
#include <3ds.h>
#include <citro3d.h>
#include "fh_early_queue.h"
#include "../vendor/citro3d/internal.h"
extern void fh_gx_queue_hold(gxCmdQueue_s *queue);
static int active;
static unsigned pending;
static u64 started;
unsigned fh_early_queue_starts,fh_early_queue_finishes;
unsigned fh_early_queue_runs,fh_early_queue_appends,fh_early_queue_capacity;
u64 fh_early_queue_wait_ticks,fh_early_queue_span_ticks;
#ifdef FH_ASYNC_PRESENTATION
static unsigned deferred;
unsigned fh_early_queue_deferrals,fh_early_queue_retirements;
void fh_early_queue_capacity_completed(u64 wait_ticks){
    fh_early_queue_wait_ticks+=wait_ticks;
    if(deferred){fh_early_queue_finishes+=deferred;deferred=0;++fh_early_queue_retirements;
        fh_early_queue_span_ticks+=svcGetSystemTick()-started;}
    if(active){fh_early_queue_span_ticks+=svcGetSystemTick()-started;
        fh_early_queue_finishes+=pending;pending=0;active=0;}
}
void fh_early_queue_defer(void){
    if(!active)return;
    if(deferred)svcBreak(USERBREAK_PANIC);
    deferred=pending;pending=0;active=0;++fh_early_queue_deferrals;
}
/* Called only after the next successful SDK FrameBegin has joined both GPU
 * work and its display callback. It does not permit earlier resource reuse. */
void fh_early_queue_retire(void){
    if(!deferred)return;
    fh_early_queue_finishes+=deferred;
    fh_early_queue_span_ticks+=svcGetSystemTick()-started;
    deferred=0;++fh_early_queue_retirements;
}
#endif
static int submit(int append){
    if(active&&!append)return 0;
    gxCmdQueue_s*q=&C3Di_GetContext()->gxQueue;
    /* Reserve eight entries for incidental transfers/clears and the final
     * presentation. At capacity the caller simply keeps recording its tail.
     * Never clear entries or recycle command/resource memory while active. */
    if(q->numEntries+8>=q->maxEntries){++fh_early_queue_capacity;return 0;}
    unsigned before=q->numEntries;
    fh_gx_queue_hold(q);
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    if(q->numEntries==before)return 0;
    ++pending;++fh_early_queue_starts;
    if(active){
        ++fh_early_queue_appends;
    }else{
        started=svcGetSystemTick();active=1;++fh_early_queue_runs;
    }
    gxCmdQueueRun(q);
    return 1;
}
int fh_early_queue_submit(void){return submit(0);}
int fh_early_queue_append(void){return submit(1);}
void fh_early_queue_finish(void){
    if(!active)return;
    u64 wait=svcGetSystemTick();
#ifdef FH_ASYNC_PRESENTATION
    /* The candidate joins callback completion as well as GPU completion. */
    C3Di_RenderQueueWaitDone();
#else
    gxCmdQueue_s*q=&C3Di_GetContext()->gxQueue;
    gxCmdQueueWait(q,-1);
    gxCmdQueueStop(q);gxCmdQueueClear(q);
#endif
    u64 now=svcGetSystemTick();
    fh_early_queue_wait_ticks+=now-wait;
    /* Includes CPU work and possible queue starvation; this is an elapsed
     * overlap span, NOT an isolated GPU execution-time measurement. */
    fh_early_queue_span_ticks+=now-started;
    active=0;fh_early_queue_finishes+=pending;pending=0;
}
