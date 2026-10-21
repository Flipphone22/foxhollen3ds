#include "fh_log_io.h"
#include <string.h>
/* Pinned Melee log_io.c: owned bounded bytes, release/acquire publication,
 * sleeping native I/O worker and joined close. Here records retain file offset
 * and flags for several diagnostic streams. Saves and assets never enter it.
 * Full queues apply backpressure instead of silently dropping measurements. */
/* One complete 64 KiB stdio block per filesystem request. Preserve the same
 * payload budget and backpressure: splitting a full block into four writes
 * amplifies SD growth/service latency and stalls concurrent asset reads. */
enum { SLOTS=FH_LOG_IO_SLOTS,CHUNK=FH_LOG_IO_CHUNK,HANDLES=8,STACK=16384 };
_Static_assert(SLOTS*CHUNK==65536,"Diagnostic payload budget changed");
typedef struct { Handle handle;u64 offset;u32 size,flags;unsigned char data[CHUNK]; } Record;
typedef struct { Handle handle;Result error; } Tracked;
static Record queue[SLOTS];
static Tracked tracked[HANDLES];
static LightLock lock;
static LightEvent wake;
static Thread worker;
static FhLogWrite direct_write;
static unsigned read_cursor,write_cursor,stopping,peak,waits,errors;
static const Result io_error=MAKERESULT(RL_PERMANENT,RS_INTERNAL,RM_APPLICATION,RD_NO_DATA);
static const Result timeout=MAKERESULT(RL_TEMPORARY,RS_WOULDBLOCK,RM_APPLICATION,RD_TIMEOUT);
static int find(Handle h) {for(unsigned i=0;i<HANDLES;i++)if(tracked[i].handle==h)return (int)i;return -1;}
static void work(void *unused) {
    (void)unused;
    for(;;) {
        unsigned at=__atomic_load_n(&read_cursor,__ATOMIC_RELAXED);
        unsigned end=__atomic_load_n(&write_cursor,__ATOMIC_ACQUIRE);
        if(at==end) {
            if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))break;
            LightEvent_WaitTimeout(&wake,100000000);continue;
        }
        Record *r=&queue[at%SLOTS];u32 got=0;
        Result result=direct_write(r->handle,&got,r->offset,r->data,r->size,r->flags);
        if(R_FAILED(result) || got!=r->size) {
            LightLock_Lock(&lock);int slot=find(r->handle);
            if(slot>=0 && R_SUCCEEDED(tracked[slot].error))tracked[slot].error=R_FAILED(result)?result:io_error;
            errors++;LightLock_Unlock(&lock);
        }
        __atomic_store_n(&read_cursor,at+1,__ATOMIC_RELEASE);
    }
}
void fh_log_io_init(FhLogWrite write) {
    direct_write=write;LightLock_Init(&lock);LightEvent_Init(&wake,RESET_ONESHOT);
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    /* Same available application core as the producer, like Melee. FS IPC
     * yields it; no extra-core permission or console overclock is required. */
    worker=threadCreate(work,NULL,STACK,priority>0x18?priority-1:priority,-2,false);
}
void fh_log_io_track(Handle h,FS_Path path) {
    if(!worker || !h)return;
    const unsigned char *p=path.data;unsigned stride=path.type==PATH_UTF16?2:path.type==PATH_ASCII?1:0;
    if(!p || !stride || path.size%stride)return;
    static const char *names[]={"/3ds/foxhollow/native.log","/3ds/foxhollow/render.csv","/3ds/foxhollow/render-profile.csv","/3ds/foxhollow/sd-io.csv","/3ds/foxhollow/memory.jsonl","/3ds/foxhollow/audio.csv","/3ds/foxhollow/minimal-render.csv"};
    _Static_assert(sizeof(names)/sizeof(names[0])<=HANDLES,"Tracked diagnostic handle budget");
    unsigned match=0;
    for(unsigned n=0;n<sizeof(names)/sizeof(names[0]);n++) {
        unsigned size=strlen(names[n]);if(path.size!=(size+1)*stride)continue;
        unsigned i;for(i=0;i<=size;i++)if(p[i*stride]!=(unsigned char)names[n][i] || (stride==2 && p[i*stride+1]))break;
        if(i==size+1){match=1;break;}
    }
    if(!match)return;
    LightLock_Lock(&lock);
    for(unsigned i=0;i<HANDLES;i++)if(!tracked[i].handle){tracked[i]=(Tracked){h,0};break;}
    LightLock_Unlock(&lock);
}
int fh_log_io_write(Handle h,u32 *got,u64 offset,const void *data,u32 size,u32 flags,Result *result) {
    if(!worker)return 0;
    LightLock_Lock(&lock);int slot=find(h);
    if(slot<0){LightLock_Unlock(&lock);return 0;}
    *result=tracked[slot].error;LightLock_Unlock(&lock);
    if(got)*got=0;
    if(R_FAILED(*result))return 1;
    unsigned done=0;u64 began=svcGetSystemTick();
    while(done<size) {
        LightLock_Lock(&lock);
        unsigned end=__atomic_load_n(&write_cursor,__ATOMIC_RELAXED);
        unsigned used=end-__atomic_load_n(&read_cursor,__ATOMIC_ACQUIRE);
        if(used==SLOTS) {
            waits++;LightLock_Unlock(&lock);LightEvent_Signal(&wake);
            if(svcGetSystemTick()-began>10ULL*SYSCLOCK_ARM11){*result=timeout;return 1;}
            svcSleepThread(1000000);continue;
        }
        Record *r=&queue[end%SLOTS];unsigned count=size-done;if(count>CHUNK)count=CHUNK;
        r->handle=h;r->offset=offset+done;r->size=count;r->flags=flags;
        memcpy(r->data,(const unsigned char*)data+done,count);
        if(used+1>peak)peak=used+1;
        __atomic_store_n(&write_cursor,end+1,__ATOMIC_RELEASE);
        LightLock_Unlock(&lock);done+=count;if(got)*got=done;LightEvent_Signal(&wake);
    }
    return 1;
}
Result fh_log_io_drain(Handle h) {
    if(!worker)return 0;
    LightLock_Lock(&lock);int slot=h?find(h):0;LightLock_Unlock(&lock);
    if(slot<0)return 0;
    unsigned end=__atomic_load_n(&write_cursor,__ATOMIC_ACQUIRE);u64 began=svcGetSystemTick();
    LightEvent_Signal(&wake);
    while((int)(__atomic_load_n(&read_cursor,__ATOMIC_ACQUIRE)-end)<0) {
        if(svcGetSystemTick()-began>10ULL*SYSCLOCK_ARM11)return timeout;
        svcSleepThread(1000000);
    }
    Result result=0;LightLock_Lock(&lock);
    for(unsigned i=0;i<HANDLES;i++)if((!h || tracked[i].handle==h) && R_FAILED(tracked[i].error))result=tracked[i].error;
    LightLock_Unlock(&lock);return result;
}
void fh_log_io_forget(Handle h) {
    if(!worker)return;
    LightLock_Lock(&lock);int slot=find(h);if(slot>=0)tracked[slot]=(Tracked){0};LightLock_Unlock(&lock);
}
Result fh_log_io_shutdown(void) {
    if(!worker)return 0;
    Result result=fh_log_io_drain(0);
    __atomic_store_n(&stopping,1,__ATOMIC_RELEASE);LightEvent_Signal(&wake);
    Result joined=threadJoin(worker,10000000000ULL);if(R_FAILED(joined))return joined;
    threadFree(worker);worker=NULL;return result;
}
void fh_log_io_stats(FhLogIoStats *out) {
    LightLock_Lock(&lock);
    *out=(FhLogIoStats){worker!=NULL,__atomic_load_n(&write_cursor,__ATOMIC_RELAXED)-__atomic_load_n(&read_cursor,__ATOMIC_RELAXED),peak,waits,errors,worker?STACK:0};
    LightLock_Unlock(&lock);
}
