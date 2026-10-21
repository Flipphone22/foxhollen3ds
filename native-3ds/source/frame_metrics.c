#include "fh_frame_metrics.h"
#include <string.h>
void fh_frame_metrics_init(FhFrameMetrics *m,FILE *file,uint64_t hz,unsigned full){
    memset(m,0,sizeof(*m));m->file=file;m->hz=hz;m->full=full;
    m->threshold33=hz*335/10000;m->threshold100=hz/10;
    if(file)fputs("frame,draws,indices,submit_ms,gpu_ms,missing,rejected,present_elapsed_ms,present_interval_ms,texture_uploads,texture_changes,texture_slot_evictions,texture_budget_evictions,texture_live,texture_gpu_bytes,texture_cpu_bytes,window_frames,window_draws,window_indices,window_submit_ms,window_gpu_ms,window_intervals,window_interval_ms,window_max_interval_ms,window_max_frame,window_over_33_5ms,window_over_100ms\n",file);
}
void fh_frame_metrics_flush(FhFrameMetrics *m){
    if(!m->file || !m->frames)return;
    const FhFrameSample *s=&m->sample;double ms=1000.0/m->hz;
    fprintf(m->file,"%u,%u,%u,%.3f,%.3f,%u,%u,%.3f,%.3f,%u,%u,%u,%u,%u,%u,%u,%u,%llu,%llu,%.3f,%.3f,%u,%.3f,%.3f,%u,%u,%u\n",
        s->frame,s->draws,s->indices,s->submit_ticks*ms,s->gpu_ms,s->missing,s->rejected,
        (s->now-m->origin)*ms,(s->now-m->last)*ms,s->uploads,s->changes,s->slot_evictions,s->budget_evictions,s->live,s->gpu_bytes,s->cpu_bytes,
        m->frames,(unsigned long long)m->draws,(unsigned long long)m->indices,m->submit*ms,m->gpu_us/1000.0,
        m->intervals,m->interval*ms,m->maximum*ms,m->max_frame,m->over33,m->over100);
    /* stdio/owned logger controls writes. Never force an SD flush per row. */
    m->published=s->now;m->draws=m->indices=m->submit=m->interval=m->maximum=m->gpu_us=0;
    m->frames=m->intervals=m->over33=m->over100=m->max_frame=0;
}
void fh_frame_metrics_observe(FhFrameMetrics *m,const FhFrameSample *s){
    if(!m->file)return;
    uint64_t interval=0;
    if(m->started){m->last=m->sample.now;interval=s->now-m->last;m->intervals++;}
    else {m->started=1;m->origin=m->last=m->published=s->now;}
    m->sample=*s;m->frames++;m->draws+=s->draws;m->indices+=s->indices;
    m->submit+=s->submit_ticks;m->gpu_us+=(uint32_t)(s->gpu_ms*1000.0f+0.5f);
    m->interval+=interval;
    if(interval>m->maximum){m->maximum=interval;m->max_frame=s->frame;}
    if(interval>m->threshold33)m->over33++;
    if(interval>m->threshold100)m->over100++;
    /* Retain a timestamped row for each >100 ms gap; no catch-up burst after
     * stalls, and the final short window is explicitly published at shutdown. */
    if(m->full || s->now-m->published>=m->hz || interval>m->threshold100)
        fh_frame_metrics_flush(m);
}
