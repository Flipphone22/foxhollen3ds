#ifndef FH_SD_TRACE_H
#define FH_SD_TRACE_H
#include <stdint.h>
#include <string.h>
#include <stdio.h>

/* Completed slow calls only. The caller owns synchronization. No allocation,
 * filesystem access or timing syscall in this bounded in-memory recorder.
 * Tokens identify calling threads within ONE launch, not CPU core numbers.
 * Keep the most recent events, explicitly count overwritten/unlabelled data. */
enum { FH_SD_TRACE_EVENTS=64,FH_SD_TRACE_HANDLES=64,FH_SD_TRACE_NAME=48 };
typedef struct {
    uint32_t handle,hash;
    char name[FH_SD_TRACE_NAME];
} FhSdTraceFile;
typedef struct {
    uint64_t begin,end,bytes;
    uint32_t thread,kind,result;
    FhSdTraceFile file;
} FhSdTraceEvent;
typedef struct {
    FhSdTraceFile files[FH_SD_TRACE_HANDLES];
    FhSdTraceEvent events[FH_SD_TRACE_EVENTS];
    uint64_t count,untracked,threshold,origin,hz;
} FhSdTrace;
_Static_assert(sizeof(FhSdTrace)<=10240,"Slow I/O evidence RAM budget");
static inline void fh_sd_trace_init(FhSdTrace *s,uint64_t origin,uint64_t hz){
    memset(s,0,sizeof(*s));s->origin=origin;s->hz=hz;s->threshold=hz/20;
}
static inline FhSdTraceFile fh_sd_trace_file(uint32_t handle,const void *path,
                                           unsigned size,unsigned stride){
    FhSdTraceFile f={0};f.handle=handle;
    if(!path || (stride!=1 && stride!=2) || size%stride)return f;
    const unsigned char *p=path;unsigned length=size/stride,base=0;
    if(length && !p[(length-1)*stride] && (stride==1 || !p[(length-1)*stride+1]))length--;
    uint32_t hash=2166136261u;
    for(unsigned i=0;i<length;i++){
        unsigned lo=p[i*stride],hi=stride==2?p[i*stride+1]:0;
        hash=(hash^lo)*16777619u;hash=(hash^hi)*16777619u;
        if(lo=='/'&&!hi)base=i+1;
    }
    f.hash=hash;
    for(unsigned i=base;i<length && i-base<FH_SD_TRACE_NAME-1;i++){
        unsigned c=p[i*stride],hi=stride==2?p[i*stride+1]:0;
        /* CSV-safe basename. Hash retains the full original UTF-16 path. */
        f.name[i-base]=!hi&&((c>='a'&&c<='z')||(c>='A'&&c<='Z')||
            (c>='0'&&c<='9')||c=='.'||c=='_'||c=='-')?(char)c:'_';
    }
    return f;
}
static inline void fh_sd_trace_track(FhSdTrace *s,FhSdTraceFile file){
    if(!file.handle)return;
    unsigned empty=FH_SD_TRACE_HANDLES;
    for(unsigned i=0;i<FH_SD_TRACE_HANDLES;i++){
        if(s->files[i].handle==file.handle){s->files[i]=file;return;}
        if(!s->files[i].handle && empty==FH_SD_TRACE_HANDLES)empty=i;
    }
    if(empty<FH_SD_TRACE_HANDLES)s->files[empty]=file;else s->untracked++;
}
static inline void fh_sd_trace_forget(FhSdTrace *s,uint32_t handle){
    for(unsigned i=0;i<FH_SD_TRACE_HANDLES;i++)if(s->files[i].handle==handle)s->files[i]=(FhSdTraceFile){0};
}
static inline FhSdTraceFile fh_sd_trace_resolve(const FhSdTrace *s,uint32_t handle){
    for(unsigned i=0;i<FH_SD_TRACE_HANDLES;i++)
        if(handle && s->files[i].handle==handle)return s->files[i];
    return (FhSdTraceFile){.handle=handle};
}
static inline void fh_sd_trace_record(FhSdTrace *s,unsigned kind,uint32_t handle,
    const FhSdTraceFile *file,uint32_t thread,uint64_t begin,uint64_t end,
    uint64_t bytes,uint32_t result){
    if(end<begin || end-begin<s->threshold)return;
    FhSdTraceEvent *e=&s->events[s->count++%FH_SD_TRACE_EVENTS];
    *e=(FhSdTraceEvent){.begin=begin,.end=end,.bytes=bytes,.thread=thread,.kind=kind,.result=result};
    if(file)e->file=*file;
    else {
        e->file.handle=handle;
        for(unsigned i=0;i<FH_SD_TRACE_HANDLES;i++)if(handle && s->files[i].handle==handle){e->file=s->files[i];break;}
    }
}
static inline void fh_sd_trace_write(const FhSdTrace *s,FILE *f,const char *const *names,unsigned count){
    fprintf(f,"# version=1,threshold_ms=%.3f,total_events=%llu,overwritten=%llu,untracked_opens=%llu,storage_bytes=%u\n",
        s->threshold*1000.0/s->hz,(unsigned long long)s->count,
        (unsigned long long)(s->count>FH_SD_TRACE_EVENTS?s->count-FH_SD_TRACE_EVENTS:0),
        (unsigned long long)s->untracked,(unsigned)sizeof(*s));
    fputs("# completed spans only; overlaps are not additive; queue/drain are producer waits, not filesystem IPC; thread is a launch-local TLS token\n",f);
    fputs("event,operation,thread,handle,path_hash,file,begin_ms,end_ms,duration_ms,bytes,result\n",f);
    uint64_t first=s->count>FH_SD_TRACE_EVENTS?s->count-FH_SD_TRACE_EVENTS:0;
    for(uint64_t i=first;i<s->count;i++){
        const FhSdTraceEvent *e=&s->events[i%FH_SD_TRACE_EVENTS];
        fprintf(f,"%llu,%s,%08lx,%08lx,%08lx,%s,%.3f,%.3f,%.3f,%llu,%08lx\n",
            (unsigned long long)i,e->kind<count?names[e->kind]:"unknown",
            (unsigned long)e->thread,(unsigned long)e->file.handle,(unsigned long)e->file.hash,e->file.name,
            (e->begin-s->origin)*1000.0/s->hz,(e->end-s->origin)*1000.0/s->hz,
            (e->end-e->begin)*1000.0/s->hz,(unsigned long long)e->bytes,(unsigned long)e->result);
    }
}
#endif
