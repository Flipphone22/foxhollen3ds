#define _GNU_SOURCE 1
#include <3ds.h>
#include <stdio.h>
#include <string.h>
#include "fh_sd_stats.h"
#include "fh_log_io.h"
#include "fh_diagnostic_buffer.h"
#include "fh_sd_trace.h"
#include "fh_sd_bursts.h"

/* Measure actual filesystem IPC, below stdio buffering. No I/O or allocation
 * occurs in accounting. The lock covers counters only, never an FS request.
 * Timings include service waits and thread scheduling, not SD bus utilization. */
typedef struct { u64 calls,bytes,ticks,maximum,errors; } Counter;
enum { READ,WRITE,FLUSH,METADATA,ASSET_READ,RENAME,DELETE,CREATE,DIRECTORY,COUNTERS };
enum { TRACE_QUEUE=COUNTERS,TRACE_DRAIN };
static FhSdTrace slow_calls;
static FhSdBursts bursts;
static const char *const counter_names[]={"read","write","flush","metadata","asset_read","rename","delete","create","directory","queue","drain"};
static Counter totals[COUNTERS];
static LightLock lock;
static unsigned enabled;
static Handle asset_handles[8];
static FILE *output;
static u64 started;
static u64 next_observation;
static unsigned observation_started;
void fh_sd_stats_snapshot(FhSdDisplay *out){
    memset(out,0,sizeof(*out));if(!enabled)return;
    LightLock_Lock(&lock);
    out->read_bytes=totals[READ].bytes;out->read_ticks=totals[READ].ticks;
    out->read_max_ticks=totals[READ].maximum;
    out->all_ticks=totals[READ].ticks+totals[WRITE].ticks+totals[FLUSH].ticks+totals[METADATA].ticks;
    LightLock_Unlock(&lock);
}
enum { FILE_SLOTS=64, FILE_NAME=48 };
typedef struct { char name[FILE_NAME]; u64 calls,bytes,ticks,opens; } FileTotal;
static FileTotal file_totals[FILE_SLOTS];
static struct { Handle handle; unsigned slot; } file_handles[FILE_SLOTS];
static void file_open(Handle h,FS_Path path){
    const unsigned char *p=path.data;unsigned stride=path.type==PATH_UTF16?2:path.type==PATH_ASCII?1:0;
    if(!h || !p || !stride)return;
    unsigned count=path.size/stride;while(count && !p[(count-1)*stride])count--;
    unsigned start=count>FILE_NAME-1?count-(FILE_NAME-1):0;
    char name[FILE_NAME];unsigned n=0;
    for(unsigned i=start;i<count;i++)name[n++]=(char)p[i*stride];
    name[n]=0;
    unsigned slot=FILE_SLOTS;
    for(unsigned i=0;i<FILE_SLOTS;i++){
        if(!strcmp(file_totals[i].name,name)){slot=i;break;}
        if(!file_totals[i].name[0]){memcpy(file_totals[i].name,name,n+1);slot=i;break;}
    }
    if(slot==FILE_SLOTS)return;
    file_totals[slot].opens++;
    for(unsigned i=0;i<FILE_SLOTS;i++)if(!file_handles[i].handle || file_handles[i].handle==h){file_handles[i].handle=h;file_handles[i].slot=slot;return;}
}
static void file_read(Handle h,u64 ticks,u64 bytes){
    for(unsigned i=0;i<FILE_SLOTS;i++)if(file_handles[i].handle==h){
        FileTotal *t=file_totals+file_handles[i].slot;t->calls++;t->bytes+=bytes;t->ticks+=ticks;return;
    }
}
static void file_close(Handle h){
    for(unsigned i=0;i<FILE_SLOTS;i++)if(file_handles[i].handle==h)file_handles[i].handle=0;
}
static void file_dump(void){
    FILE *f=fopen("sdmc:/3ds/foxhollow/sd-files.csv","wb");if(!f)return;
    fprintf(f,"file,opens,read_calls,read_bytes,read_ms\n");
    for(unsigned i=0;i<FILE_SLOTS;i++)if(file_totals[i].name[0])
        fprintf(f,"%s,%llu,%llu,%llu,%.3f\n",file_totals[i].name,(unsigned long long)file_totals[i].opens,(unsigned long long)file_totals[i].calls,
            (unsigned long long)file_totals[i].bytes,file_totals[i].ticks*1000.0/SYSCLOCK_ARM11);
    fclose(f);
}
typedef struct { uint32_t handle,span_bytes; uint64_t size,span_offset,calls,bytes; unsigned char *span; } FhSdReader;
int fh_sd_reader_open(FhSdReader *reader,const char *path);
int fh_sd_reader_read(FhSdReader *reader,uint64_t offset,void *out,uint32_t bytes,uint64_t hint_end);
void fh_sd_reader_close(FhSdReader *reader);
FILE *fh_sd_reader_stream(const char *path);
#include <malloc.h>
enum { SD_READER_SPAN=128*1024 };
void fh_sd_reader_close(FhSdReader *r){
    if(!r)return;
    if(r->handle)FSFILE_Close(r->handle);
    free(r->span);memset(r,0,sizeof(*r));
}
int fh_sd_reader_open(FhSdReader *r,const char *path){
    if(!r)return 0;
    if(r->handle)fh_sd_reader_close(r);
    memset(r,0,sizeof(*r));
    if(!path)return 0;
    if(!strncmp(path,"sdmc:",5))path+=5;
    FS_Path name=fsMakePath(PATH_ASCII,path);Handle h=0;u64 size=0;
    if(R_FAILED(FSUSER_OpenFileDirectly(&h,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""),name,FS_OPEN_READ,0)))return 0;
    unsigned char *span=memalign(64,SD_READER_SPAN);
    if(!span || R_FAILED(FSFILE_GetSize(h,&size))){free(span);FSFILE_Close(h);return 0;}
    if(enabled){LightLock_Lock(&lock);file_open(h,name);LightLock_Unlock(&lock);}
    r->handle=h;r->size=size;r->span=span;return 1;
}
int fh_sd_reader_read(FhSdReader *r,uint64_t offset,void *out,uint32_t bytes,uint64_t hint_end){
    if(!r || !r->handle || offset>r->size || bytes>r->size-offset || (!out && bytes))return 0;
    unsigned char *dst=out;
    while(bytes){
        if(offset>=r->span_offset && offset-r->span_offset<r->span_bytes){
            uint32_t n=(uint32_t)(r->span_offset+r->span_bytes-offset);if(n>bytes)n=bytes;
            memcpy(dst,r->span+(offset-r->span_offset),n);dst+=n;offset+=n;bytes-=n;continue;
        }
        uint64_t start=offset&~511ull,end=offset+bytes;
        if(hint_end>end)end=hint_end;
        end=(end+511)&~511ull;
        if(end-start>SD_READER_SPAN)end=start+SD_READER_SPAN;
        if(end>r->size)end=r->size;
        u32 got=0;
        if(R_FAILED(FSFILE_Read(r->handle,&got,start,r->span,(u32)(end-start))) || got!=end-start){r->span_bytes=0;return 0;}
        r->span_offset=start;r->span_bytes=got;r->calls++;r->bytes+=got;
    }
    return 1;
}
typedef struct { FhSdReader reader; uint64_t position; } SdStream;
static ssize_t sd_stream_read(void *cookie,char *buffer,size_t bytes){
    SdStream *s=cookie;
    if(s->position>=s->reader.size)return 0;
    if(bytes>s->reader.size-s->position)bytes=(size_t)(s->reader.size-s->position);
    if(!fh_sd_reader_read(&s->reader,s->position,buffer,(uint32_t)bytes,0))return -1;
    s->position+=bytes;return (ssize_t)bytes;
}
static int sd_stream_seek(void *cookie,_off64_t *offset,int whence){
    SdStream *s=cookie;int64_t base=whence==SEEK_SET?0:whence==SEEK_CUR?(int64_t)s->position:whence==SEEK_END?(int64_t)s->reader.size:-1;
    if(base<0 || base+*offset<0)return -1;
    s->position=(uint64_t)(base+*offset);*offset=(_off64_t)s->position;return 0;
}
static int sd_stream_close(void *cookie){
    SdStream *s=cookie;fh_sd_reader_close(&s->reader);free(s);return 0;
}
FILE *fh_sd_reader_stream(const char *path){
    SdStream *s=calloc(1,sizeof(*s));if(!s)return NULL;
    if(!fh_sd_reader_open(&s->reader,path)){free(s);return NULL;}
    cookie_io_functions_t io={sd_stream_read,NULL,sd_stream_seek,sd_stream_close};
    FILE *f=fopencookie(s,"rb",io);
    if(!f){sd_stream_close(s);return NULL;}
    if(setvbuf(f,NULL,_IONBF,0)){fclose(f);return NULL;}
    return f;
}
static Result write_direct(Handle,u32*,u64,const void*,u32,u32);

void fh_sd_stats_init(void) {
    LightLock_Init(&lock);started=svcGetSystemTick();fh_sd_trace_init(&slow_calls,started,SYSCLOCK_ARM11);
    fh_sd_bursts_init(&bursts,started,SYSCLOCK_ARM11);
    enabled=1;fh_log_io_init(write_direct);
}
static void add(unsigned kind,u64 ticks,u64 bytes,Result result) {
    Counter *c=&totals[kind];c->calls++;c->bytes+=bytes;c->ticks+=ticks;
    if(ticks>c->maximum)c->maximum=ticks;
    c->errors+=R_FAILED(result);
}
static void account(unsigned kind,Handle handle,u64 began,u64 bytes,Result result) {
    if(!enabled)return;
    u64 ended=svcGetSystemTick(),elapsed=ended-began;
    LightLock_Lock(&lock);add(kind,elapsed,bytes,result);
    fh_sd_bursts_record(&bursts,kind,fh_sd_trace_resolve(&slow_calls,handle),(u32)(uintptr_t)getThreadLocalStorage(),began,ended,bytes,(u32)result);
    fh_sd_trace_record(&slow_calls,kind,handle,NULL,(u32)(uintptr_t)getThreadLocalStorage(),began,ended,bytes,(u32)result);
    /* Named metadata counters are subsets, not additional filesystem work. */
    if(kind>=RENAME)add(METADATA,elapsed,bytes,result);
    if(kind==READ && handle)file_read(handle,elapsed,bytes);
    if(kind==READ && handle)for(unsigned i=0;i<8;i++)if(handle==asset_handles[i]){add(ASSET_READ,elapsed,bytes,result);break;}
    LightLock_Unlock(&lock);
}
void fh_sd_stats_begin_run(void){
    /* After replay archival and directory creation, before gameplay. Replace
     * an old completed trace so a crash cannot masquerade as this run's data.
     * Detailed records stay in RAM until clean shutdown, not on the frame path. */
    FILE *f=fopen("sdmc:/3ds/foxhollow/sd-slow.csv","wb");
    if(f){fprintf(f,"# incomplete: this launch has not reached clean shutdown; origin_ticks=%llu\n",(unsigned long long)started);fclose(f);}
    f=fopen("sdmc:/3ds/foxhollow/sd-bursts.csv","wb");
    if(f){fprintf(f,"# incomplete: this launch has not reached clean shutdown; origin_ticks=%llu\n",(unsigned long long)started);fclose(f);}
}
/* Queue submission and explicit drains may wait before any FS request starts.
 * Report them separately: adding these spans to IPC would double-count a
 * blocked producer and its worker's concurrent write. No trace I/O here. */
static void queue_span(unsigned kind,Handle h,u64 began,u64 bytes,Result result){
    if(!enabled)return;
    u64 ended=svcGetSystemTick();if(ended-began<slow_calls.threshold)return;
    LightLock_Lock(&lock);
    fh_sd_trace_record(&slow_calls,kind,h,NULL,(u32)(uintptr_t)getThreadLocalStorage(),began,ended,bytes,(u32)result);
    LightLock_Unlock(&lock);
}
static Result drain(Handle h){
    u64 before=svcGetSystemTick();Result r=fh_log_io_drain(h);
    queue_span(TRACE_DRAIN,h,before,0,r);return r;
}
static unsigned path_ends_with(FS_Path path,const char *name) {
    const unsigned char *p=path.data;unsigned stride=path.type==PATH_UTF16?2:path.type==PATH_ASCII?1:0;
    if(!p || !stride || path.size<8*stride)return 0;
    unsigned count=path.size/stride;
    if(count && !p[(count-1)*stride] && (stride==1 || !p[(count-1)*stride+1]))--count;
    unsigned length=strlen(name);if(count<length)return 0;
    for(unsigned i=0;i<length;i++)if(p[(count-length+i)*stride]!=(unsigned char)name[i] || (stride==2 && p[(count-length+i)*stride+1]))return 0;
    return count==length || p[(count-length-1)*stride]=='/';
}
Result __real_FSUSER_OpenFile(Handle*,FS_Archive,FS_Path,u32,u32);
Result __wrap_FSUSER_OpenFile(Handle *out,FS_Archive archive,FS_Path path,u32 flags,u32 attributes) {
    u64 before=svcGetSystemTick();Result r=__real_FSUSER_OpenFile(out,archive,path,flags,attributes);
    /* Save completion time before bookkeeping; names never extend IPC timing. */
    u64 ended=svcGetSystemTick();
    if(enabled){
        FhSdTraceFile file=fh_sd_trace_file(R_SUCCEEDED(r)&&out?*out:0,path.data,path.size,path.type==PATH_UTF16?2:path.type==PATH_ASCII?1:0);
        LightLock_Lock(&lock);fh_sd_trace_track(&slow_calls,file);add(METADATA,ended-before,0,r);
        fh_sd_trace_record(&slow_calls,METADATA,file.handle,&file,(u32)(uintptr_t)getThreadLocalStorage(),before,ended,0,(u32)r);
        fh_sd_bursts_record(&bursts,METADATA,file,(u32)(uintptr_t)getThreadLocalStorage(),before,ended,0,(u32)r);
        LightLock_Unlock(&lock);
    }
    if(enabled && R_SUCCEEDED(r) && out){LightLock_Lock(&lock);file_open(*out,path);LightLock_Unlock(&lock);}
    if(R_SUCCEEDED(r) && out && (flags&FS_OPEN_WRITE))fh_log_io_track(*out,path);
    if(enabled && R_SUCCEEDED(r) && out){
        unsigned slot=path_ends_with(path,"disc.iso")?1:path_ends_with(path,"starfox-pica.thp")?2:0;
        if(slot){LightLock_Lock(&lock);for(unsigned i=0;i<8;i++)if(!asset_handles[i]){asset_handles[i]=*out;break;}LightLock_Unlock(&lock);}
    }
    return r;
}
/* Saving replaces a temporary GCI using these metadata requests. Omitting
 * them previously made service latency look like unaccounted CPU work.
 * Forward unchanged, take no lock across IPC, and perform no logging here. */
Result __real_FSUSER_RenameFile(FS_Archive,FS_Path,FS_Archive,FS_Path);
Result __wrap_FSUSER_RenameFile(FS_Archive src,FS_Path from,FS_Archive dst,FS_Path to) {
    u64 before=svcGetSystemTick();Result r=__real_FSUSER_RenameFile(src,from,dst,to);
    account(RENAME,0,before,0,r);return r;
}
Result __real_FSUSER_DeleteFile(FS_Archive,FS_Path);
Result __wrap_FSUSER_DeleteFile(FS_Archive archive,FS_Path path) {
    u64 before=svcGetSystemTick();Result r=__real_FSUSER_DeleteFile(archive,path);
    account(DELETE,0,before,0,r);return r;
}
Result __real_FSUSER_CreateFile(FS_Archive,FS_Path,u32,u64);
Result __wrap_FSUSER_CreateFile(FS_Archive archive,FS_Path path,u32 attributes,u64 bytes) {
    u64 before=svcGetSystemTick();Result r=__real_FSUSER_CreateFile(archive,path,attributes,bytes);
    account(CREATE,0,before,0,r);return r;
}
Result __real_FSUSER_OpenDirectory(Handle*,FS_Archive,FS_Path);
Result __wrap_FSUSER_OpenDirectory(Handle *out,FS_Archive archive,FS_Path path) {
    u64 before=svcGetSystemTick();Result r=__real_FSUSER_OpenDirectory(out,archive,path);
    account(DIRECTORY,0,before,0,r);return r;
}
Result __real_FSDIR_Read(Handle,u32*,u32,FS_DirectoryEntry*);
Result __wrap_FSDIR_Read(Handle handle,u32 *read,u32 count,FS_DirectoryEntry *entries) {
    u64 before=svcGetSystemTick();Result r=__real_FSDIR_Read(handle,read,count,entries);
    account(DIRECTORY,0,before,0,r);return r;
}
Result __real_FSDIR_Close(Handle);
Result __wrap_FSDIR_Close(Handle handle) {
    u64 before=svcGetSystemTick();Result r=__real_FSDIR_Close(handle);
    account(DIRECTORY,0,before,0,r);return r;
}
Result __real_FSFILE_Read(Handle,u32*,u64,void*,u32);
Result __wrap_FSFILE_Read(Handle h,u32 *got,u64 offset,void *buffer,u32 size) {
    Result pending=drain(h);if(R_FAILED(pending))return pending;
    u64 before=svcGetSystemTick();Result r=__real_FSFILE_Read(h,got,offset,buffer,size);
    account(READ,h,before,R_SUCCEEDED(r)&&got?*got:0,r);return r;
}
Result __real_FSFILE_Write(Handle,u32*,u64,const void*,u32,u32);
static Result write_direct(Handle h,u32 *got,u64 offset,const void *buffer,u32 size,u32 flags) {
    u64 before=svcGetSystemTick();Result r=__real_FSFILE_Write(h,got,offset,buffer,size,flags);
    account(WRITE,h,before,R_SUCCEEDED(r)&&got?*got:0,r);return r;
}
Result __wrap_FSFILE_Write(Handle h,u32 *got,u64 offset,const void *buffer,u32 size,u32 flags) {
    u64 before=svcGetSystemTick();
    Result result;if(fh_log_io_write(h,got,offset,buffer,size,flags,&result)){
        queue_span(TRACE_QUEUE,h,before,got?*got:0,result);return result;
    }
    return write_direct(h,got,offset,buffer,size,flags);
}
Result __real_FSFILE_Flush(Handle);
Result __wrap_FSFILE_Flush(Handle h) {
    Result pending=drain(h);if(R_FAILED(pending))return pending;
    u64 before=svcGetSystemTick();Result r=__real_FSFILE_Flush(h);
    account(FLUSH,h,before,0,r);return r;
}
Result __real_FSFILE_Close(Handle);
Result __wrap_FSFILE_Close(Handle h) {
    Result pending=drain(h);if(R_FAILED(pending))return pending;
    u64 before=svcGetSystemTick();Result r=__real_FSFILE_Close(h);
    account(METADATA,h,before,0,r);
    if(R_SUCCEEDED(r))fh_log_io_forget(h);
    if(enabled && R_SUCCEEDED(r)){LightLock_Lock(&lock);fh_sd_bursts_forget(&bursts,h);fh_sd_trace_forget(&slow_calls,h);file_close(h);for(unsigned i=0;i<8;i++)if(asset_handles[i]==h)asset_handles[i]=0;LightLock_Unlock(&lock);}
    return r;
}
Result __real_FSFILE_GetSize(Handle,u64*);
Result __wrap_FSFILE_GetSize(Handle h,u64 *size) {
    Result pending=drain(h);if(R_FAILED(pending))return pending;
    u64 before=svcGetSystemTick();Result r=__real_FSFILE_GetSize(h,size);
    account(METADATA,h,before,0,r);return r;
}
Result __real_FSFILE_SetSize(Handle,u64);
Result __wrap_FSFILE_SetSize(Handle h,u64 size) {
    Result pending=drain(h);if(R_FAILED(pending))return pending;
    u64 before=svcGetSystemTick();Result r=__real_FSFILE_SetSize(h,size);
    account(METADATA,h,before,0,r);return r;
}
void fh_sd_stats_observe(unsigned retrace) {
    /* Counters still measure EVERY service call. Publish one cumulative row
     * per wall second rather than rewriting unchanged totals every frame.
     * A slow frame extends the interval; elapsed_ms records the actual span. */
    u64 now=svcGetSystemTick();
    if(observation_started && now<next_observation)return;
    observation_started=1;next_observation=now+SYSCLOCK_ARM11;
    if(!output){
        output=fopen("sdmc:/3ds/foxhollow/sd-io.csv","wb");if(!output)return;
        if(fh_diagnostic_buffer(output)){fclose(output);output=NULL;return;}
        fputs("retrace,elapsed_ms",output);
        static const char *names[]={"read","write","flush","metadata","asset_read","rename","delete","create","directory"};
        for(unsigned i=0;i<COUNTERS;i++)fprintf(output,",%s_calls,%s_bytes,%s_ms,%s_max_ms,%s_errors",names[i],names[i],names[i],names[i],names[i]);
        fputc('\n',output);
    }
    Counter copy[COUNTERS];LightLock_Lock(&lock);memcpy(copy,totals,sizeof(copy));LightLock_Unlock(&lock);
    fprintf(output,"%u,%.3f",retrace,(svcGetSystemTick()-started)*1000.0/SYSCLOCK_ARM11);
    for(unsigned i=0;i<COUNTERS;i++)fprintf(output,",%llu,%llu,%.3f,%.3f,%llu",(unsigned long long)copy[i].calls,(unsigned long long)copy[i].bytes,
        copy[i].ticks*1000.0/SYSCLOCK_ARM11,copy[i].maximum*1000.0/SYSCLOCK_ARM11,(unsigned long long)copy[i].errors);
    fputc('\n',output);
}
void fh_sd_stats_shutdown(unsigned retrace) {
    observation_started=0; /* Always retain final cumulative counters. */
    fh_sd_stats_observe(retrace);if(output){fclose(output);output=NULL;}
    Result joined=fh_log_io_shutdown();FhLogIoStats logs;fh_log_io_stats(&logs);
    FILE *status=fopen("sdmc:/3ds/foxhollow/log-io-summary.json","wb");
    if(status){fprintf(status,"{\"shutdown_result\":%ld,\"queued\":%u,\"peak_records\":%u,\"backpressure_waits\":%u,\"errors\":%u}\n",(long)joined,logs.queued,logs.peak,logs.waits,logs.errors);fclose(status);}
    enabled=0;
    file_dump();
    /* All diagnostic worker calls have completed on a successful join. Never
     * race a timed-out worker or log the trace's own output back into itself. */
    if(R_SUCCEEDED(joined)){
        FILE *trace=fopen("sdmc:/3ds/foxhollow/sd-slow.csv","wb");
        if(trace){fh_sd_trace_write(&slow_calls,trace,counter_names,sizeof(counter_names)/sizeof(*counter_names));fclose(trace);}
        fh_sd_bursts_finish(&bursts);
        trace=fopen("sdmc:/3ds/foxhollow/sd-bursts.csv","wb");
        if(trace){fh_sd_bursts_write(&bursts,trace,counter_names,sizeof(counter_names)/sizeof(*counter_names));fclose(trace);}
    }
}
