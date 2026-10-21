/* Adapted from pinned melee-3ds c2ebe7c port/3ds/gpu_busy.c (GPL-3.0).
 * See licenses/ADAPTATIONS.md. Observe submission/IRQ without changing either.
 * Unlike a queue-tail timer this covers early prefixes and overlapping copies.
 * Pair event classes and expose failure/unknown events rather than treating
 * unmatched interrupts or a rejected submission as reliable GPU work timing.
 */
#include <3ds.h>
#include "fh_gpu_work.h"
static LightLock work_lock=1;
static FhGpuWork work;
static unsigned pending_event[7];
static u64 started;
extern Result __real_gspSubmitGxCommand(const u32 command[8]);
extern void __real_gxCmdQueueInterrupt(GSPGPU_Event event);
static int completion_event(unsigned command){
    switch(command){
    case 0:return GSPGPU_EVENT_DMA;
    case 1:return GSPGPU_EVENT_P3D;
    case 2:return GSPGPU_EVENT_PSC0;
    case 3:case 4:return GSPGPU_EVENT_PPF;
    default:return -1;
    }
}
Result __wrap_gspSubmitGxCommand(const u32 command[8]){
    unsigned type=command[0]&255;int event=completion_event(type);
    LightLock_Lock(&work_lock);
    work.last_command=type;
    if(type<6)++work.command_count[type];else ++work.unsupported;
    if(event>=0){
        if(!work.pending++)started=svcGetSystemTick();
        ++pending_event[event];++work.submitted;
    }
    LightLock_Unlock(&work_lock);
    /* A completion may run before this call returns; never hold our lock
     * across libctru or a GSP service operation. */
    Result result=__real_gspSubmitGxCommand(command);
    LightLock_Lock(&work_lock);
    if(R_FAILED(result)){
        ++work.failed;work.last_result=result;
        /* Some failures occur after publication. Do not resubmit, invent a
         * completion, or infer ownership from the return code. Timing is now
         * explicitly invalid; leave actual queue behavior to libctru. */
    }
    LightLock_Unlock(&work_lock);
    return result;
}
void __wrap_gxCmdQueueInterrupt(GSPGPU_Event event){
    LightLock_Lock(&work_lock);
    work.last_event=(unsigned)event;
    if((unsigned)event<7)++work.event_count[event];else ++work.unsupported;
    if(event==GSPGPU_EVENT_DMA || event==GSPGPU_EVENT_P3D ||
       event==GSPGPU_EVENT_PSC0 || event==GSPGPU_EVENT_PPF){
        if(pending_event[event]){
            --pending_event[event];++work.completed;
            if(!--work.pending)work.busy_ticks+=svcGetSystemTick()-started;
        }else ++work.unmatched;
    }
    LightLock_Unlock(&work_lock);
    __real_gxCmdQueueInterrupt(event);
}
void fh_gpu_work_snapshot(FhGpuWork *out){
    LightLock_Lock(&work_lock);
    *out=work;out->sampled_ticks=svcGetSystemTick();
    if(work.pending)out->busy_ticks+=out->sampled_ticks-started;
    LightLock_Unlock(&work_lock);
}
