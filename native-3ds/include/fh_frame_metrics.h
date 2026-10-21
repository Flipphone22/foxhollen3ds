#ifndef FH_FRAME_METRICS_H
#define FH_FRAME_METRICS_H
#include <stdint.h>
#include <stdio.h>
/* Counts and durations cover every presented frame, even when normal gameplay
 * only serializes one interval per second. Replay retains frame resolution. */
typedef struct {
    unsigned frame,draws,indices,missing,rejected;
    unsigned uploads,changes,slot_evictions,budget_evictions,live,gpu_bytes,cpu_bytes;
    uint64_t now,submit_ticks; float gpu_ms;
} FhFrameSample;
typedef struct {
    FILE *file; uint64_t hz,origin,last,published,threshold33,threshold100;
    uint64_t draws,indices,submit,interval,maximum,gpu_us;
    unsigned frames,intervals,over33,over100,max_frame,started,full;
    FhFrameSample sample;
} FhFrameMetrics;
void fh_frame_metrics_init(FhFrameMetrics*,FILE*,uint64_t,unsigned);
void fh_frame_metrics_observe(FhFrameMetrics*,const FhFrameSample*);
void fh_frame_metrics_flush(FhFrameMetrics*);
#endif
