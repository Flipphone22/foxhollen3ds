#ifndef FH_GPU_WORK_H
#define FH_GPU_WORK_H
#include <stdint.h>
typedef struct {
    uint64_t sampled_ticks,busy_ticks;
    uint32_t submitted,completed,pending,failed,unmatched,unsupported;
    int32_t last_result;
    uint32_t last_command,last_event;
    uint32_t command_count[6],event_count[7];
} FhGpuWork;
/* Software-observed outstanding-command time, including interrupt latency.
 * Not hardware utilization or additive CPU+GPU frame time. Failure/unmatched
 * deltas invalidate a measurement window. No waits, allocation or file I/O. */
void fh_gpu_work_snapshot(FhGpuWork *out);
#endif
