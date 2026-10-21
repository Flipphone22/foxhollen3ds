#ifndef FH_SD_BURSTS_H
#define FH_SD_BURSTS_H
#include "fh_sd_trace.h"
/* Completed calls only, caller synchronized. A burst joins one file/operation/
 * thread for at most one second, with gaps <=100ms. Summed IPC spans are NOT
 * CPU time or a main-thread stall. This recorder allocates/writes nothing. */
enum { FH_SD_BURST_PENDING=8,FH_SD_BURST_EVENTS=32 };
typedef struct {
    FhSdTraceFile file;
    uint64_t begin,end,ticks,maximum,bytes;
    uint32_t thread,kind,calls,errors;
} FhSdBurst;
typedef struct {
    FhSdBurst pending[FH_SD_BURST_PENDING],events[FH_SD_BURST_EVENTS];
    uint64_t origin,hz,count,evictions,invalid;
} FhSdBursts;
_Static_assert(sizeof(FhSdBursts)<=6144,"Burst evidence RAM budget");
static inline void fh_sd_bursts_init(FhSdBursts *s,uint64_t origin,uint64_t hz){
    memset(s,0,sizeof(*s));s->origin=origin;s->hz=hz;
}
static inline void fh_sd_bursts_flush(FhSdBursts *s,unsigned i){
    FhSdBurst *b=s->pending+i;
    /* Single slow requests already belong to sd-slow.csv. */
    if(b->calls>1 && b->ticks>=s->hz/10)s->events[s->count++%FH_SD_BURST_EVENTS]=*b;
    memset(b,0,sizeof(*b));
}
static inline void fh_sd_bursts_forget(FhSdBursts *s,uint32_t handle){
    for(unsigned i=0;i<FH_SD_BURST_PENDING;i++)
        if(s->pending[i].calls && s->pending[i].file.handle==handle)fh_sd_bursts_flush(s,i);
}
static inline void fh_sd_bursts_record(FhSdBursts *s,unsigned kind,FhSdTraceFile file,
    uint32_t thread,uint64_t begin,uint64_t end,uint64_t bytes,uint32_t result){
    if(!s->hz || begin<s->origin || end<begin){s->invalid++;return;}
    unsigned slot=FH_SD_BURST_PENDING,empty=FH_SD_BURST_PENDING,oldest=0;
    for(unsigned i=0;i<FH_SD_BURST_PENDING;i++){
        FhSdBurst *b=s->pending+i;
        if(b->calls && b->thread==thread && b->kind==kind &&
           !memcmp(&b->file,&file,sizeof(file))){slot=i;break;}
        if(!b->calls)empty=i;
        if(b->end<s->pending[oldest].end)oldest=i;
    }
    if(slot==FH_SD_BURST_PENDING){
        slot=empty<FH_SD_BURST_PENDING?empty:oldest;
        if(s->pending[slot].calls){s->evictions++;fh_sd_bursts_flush(s,slot);}
    }
    FhSdBurst *b=s->pending+slot;
    if(b->calls && (begin<b->end || begin-b->end>s->hz/10 || end-b->begin>s->hz))
        fh_sd_bursts_flush(s,slot);
    if(!b->calls){b->file=file;b->thread=thread;b->kind=kind;b->begin=begin;}
    uint64_t ticks=end-begin;
    b->end=end;b->ticks+=ticks;b->bytes+=bytes;b->calls++;b->errors+=(int32_t)result<0;
    if(ticks>b->maximum)b->maximum=ticks;
}
static inline void fh_sd_bursts_finish(FhSdBursts *s){
    for(unsigned i=0;i<FH_SD_BURST_PENDING;i++)fh_sd_bursts_flush(s,i);
}
static inline void fh_sd_bursts_write(const FhSdBursts *s,FILE *f,const char *const *names,unsigned count){
    fprintf(f,"# version=1,window_ms=1000,gap_ms=100,minimum_call_ms=100,minimum_calls=2,total_events=%llu,overwritten=%llu,pending_evictions=%llu,invalid=%llu,storage_bytes=%u\n",
        (unsigned long long)s->count,(unsigned long long)(s->count>FH_SD_BURST_EVENTS?s->count-FH_SD_BURST_EVENTS:0),
        (unsigned long long)s->evictions,(unsigned long long)s->invalid,(unsigned)sizeof(*s));
    fputs("# completed IPC spans only; overlaps with sd-slow and other threads are not additive; thread is launch-local TLS, not a core\n",f);
    fputs("event,operation,thread,handle,path_hash,file,begin_ms,end_ms,calls,call_ms,max_call_ms,bytes,errors\n",f);
    uint64_t first=s->count>FH_SD_BURST_EVENTS?s->count-FH_SD_BURST_EVENTS:0;
    for(uint64_t i=first;i<s->count;i++){
        const FhSdBurst *b=&s->events[i%FH_SD_BURST_EVENTS];
        fprintf(f,"%llu,%s,%08lx,%08lx,%08lx,%s,%.3f,%.3f,%u,%.3f,%.3f,%llu,%u\n",
            (unsigned long long)i,b->kind<count?names[b->kind]:"unknown",(unsigned long)b->thread,
            (unsigned long)b->file.handle,(unsigned long)b->file.hash,b->file.name,
            (b->begin-s->origin)*1000.0/s->hz,(b->end-s->origin)*1000.0/s->hz,(unsigned)b->calls,
            b->ticks*1000.0/s->hz,b->maximum*1000.0/s->hz,(unsigned long long)b->bytes,(unsigned)b->errors);
    }
}
#endif
