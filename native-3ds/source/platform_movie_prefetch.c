#include <3ds.h>
#include <malloc.h>
#include <stdlib.h>
#include <string.h>
#include "fh_movie_prefetch.h"
#include "fh_platform.h"

/* Adapt Melee's sleeping same-core file worker and joined close. One owned
 * CPU buffer, one producer and one request; no game callback on this thread.
 * Asynchronous movie requests poll privately owned bytes; synchronous callers
 * and close join before sharing/releasing the FILE, as in pinned Melee. */
enum { STACK=32768,MAX_BYTES=196608,IDLE=0,READING=1,READY=2 };
unsigned fh_movie_prefetch_mode;
typedef struct {
    FILE *file; Thread worker; LightEvent wake,done; aptHookCookie hook;
    unsigned char *buffer; void *destination;
    unsigned state,stopping,suspended,offset,length,file_bytes,accounted,mode;
    int result; u64 read_ticks; FhMoviePrefetchStats stats;
} ReadAhead;
static ReadAhead movie,stream,disc_reader;
static FILE *disc_file;
static FILE *stream_file;
static unsigned stream_base,stream_bytes;

static void join(ReadAhead *c){
    if(!c->worker)return;
    u64 began=svcGetSystemTick();
    while(__atomic_load_n(&c->state,__ATOMIC_ACQUIRE)==READING){
        LightEvent_WaitTimeout(&c->done,10000000);
    }
    c->stats.wait_ticks+=svcGetSystemTick()-began;
    if(__atomic_load_n(&c->state,__ATOMIC_ACQUIRE)==READY && !c->accounted){
        c->stats.read_ticks+=c->read_ticks;c->stats.errors+=c->result<0;
        if(c->read_ticks>c->stats.max_read_ticks)c->stats.max_read_ticks=c->read_ticks;
        c->stats.slow_reads+=c->read_ticks>3ULL*SYSCLOCK_ARM11;c->accounted=1;
    }
}
static void work(void *unused){
    ReadAhead *c=unused;
    for(;;){
        if(__atomic_load_n(&c->state,__ATOMIC_ACQUIRE)==READING){
            u64 start=svcGetSystemTick();
            c->result=fseek(c->file,c->offset,SEEK_SET) || fread(c->destination,1,c->length,c->file)!=c->length?-1:(int)c->length;
            c->read_ticks=svcGetSystemTick()-start;
            __atomic_store_n(&c->state,READY,__ATOMIC_RELEASE);LightEvent_Signal(&c->done);
        }
        if(__atomic_load_n(&c->stopping,__ATOMIC_ACQUIRE))break;
        LightEvent_WaitTimeout(&c->wake,100000000);
    }
}
static void lifecycle(APT_HookType type,void *unused){
    ReadAhead *c=unused;
    if(type==APTHOOK_ONSUSPEND || type==APTHOOK_ONSLEEP){join(c);c->suspended=1;}
    else if(type==APTHOOK_ONRESTORE || type==APTHOOK_ONWAKEUP)c->suspended=0;
}
static void close_reader(ReadAhead *c){
    if(c->worker){
        aptUnhook(&c->hook);join(c);__atomic_store_n(&c->stopping,1,__ATOMIC_RELEASE);LightEvent_Signal(&c->wake);
        if(R_FAILED(threadJoin(c->worker,UINT64_MAX)))fh_platform_panic("Background file shutdown timeout");
        threadFree(c->worker);c->worker=NULL;
    }
    free(c->buffer);c->buffer=NULL;c->file=NULL;c->state=c->stopping=c->suspended=0;c->stats.active=0;
}
static void open_reader(ReadAhead *c,FILE *source,unsigned capacity,unsigned bytes,unsigned mode){
    close_reader(c);
    if(!mode || !source || (!capacity && mode!=3) || capacity>MAX_BYTES)return;
    extern u32 __ctru_heap_size,__stacksize__;
    if(((u64)(unsigned)mallinfo().uordblks+__stacksize__+capacity+STACK+4096)*100>(u64)__ctru_heap_size*85)return;
    c->buffer=capacity?malloc(capacity):NULL;if(capacity && !c->buffer)return;
    c->file=source;c->file_bytes=bytes;c->mode=mode;c->stats=(FhMoviePrefetchStats){.capacity=capacity,.stack_bytes=STACK};
    LightEvent_Init(&c->wake,RESET_ONESHOT);LightEvent_Init(&c->done,RESET_ONESHOT);
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    c->worker=threadCreate(work,c,STACK,priority>0x18?priority-1:priority,-2,false);
    if(!c->worker){close_reader(c);return;}
    aptHook(&c->hook,lifecycle,c);c->stats.active=1;
}
static int read_reader(ReadAhead *c,unsigned at,void *destination,unsigned bytes,int *out){
    if(!c->worker)return 0;
    join(c);
    if(__atomic_load_n(&c->state,__ATOMIC_ACQUIRE)!=READY || at!=c->offset || bytes!=c->length){c->stats.misses++;return 0;}
    *out=c->result;
    if(c->result>=0){
        if(c->mode==2){
            if(fseek(c->file,at,SEEK_SET) || fread(destination,1,bytes,c->file)!=bytes || memcmp(destination,c->buffer,bytes))
                fh_platform_panic("Movie prefetch byte verification failed");
            c->stats.checks++;
        }else memcpy(destination,c->buffer,bytes);
        c->stats.hits++;
    }
    __atomic_store_n(&c->state,IDLE,__ATOMIC_RELEASE);return 1;
}
static void hint_reader(ReadAhead *c,unsigned at,unsigned bytes){
    if(!c->worker || c->suspended || !bytes || bytes>c->stats.capacity || at>c->file_bytes || bytes>c->file_bytes-at)return;
    if(__atomic_load_n(&c->state,__ATOMIC_ACQUIRE)==READING)return;
    join(c);c->offset=at;c->length=bytes;c->destination=c->buffer;c->accounted=0;c->stats.queued++;
    __atomic_store_n(&c->state,READING,__ATOMIC_RELEASE);LightEvent_Signal(&c->wake);
}

void fh_movie_prefetch_close(void){close_reader(&movie);}
void fh_movie_prefetch_open(FILE *file,unsigned capacity,unsigned bytes){open_reader(&movie,file,capacity,bytes,fh_movie_prefetch_mode);}
/* The larger lossless packets require read-ahead. Preserve explicit diagnostic
 * modes and ETC's existing default; buffer allocation, publication, suspend,
 * cancellation and joined retirement remain with the same reader owner. */
void fh_movie_prefetch_open_native(FILE *file,unsigned capacity,unsigned bytes,unsigned rgb8){
    unsigned mode=fh_movie_prefetch_mode?fh_movie_prefetch_mode:(rgb8?1:0);
    open_reader(&movie,file,capacity,bytes,mode);
}
int fh_movie_prefetch_read(unsigned at,void *destination,unsigned bytes,int *out){return read_reader(&movie,at,destination,bytes,out);}
/* Returns handled/BUSY without touching the caller buffer while SD is pending.
 * A stale speculative read must finish before this FILE/capacity is reused.
 * Cancellation can drop the DVD request immediately: the worker owns its buffer. */
int fh_movie_prefetch_poll(unsigned at,void *destination,unsigned bytes,int *out){
    ReadAhead *c=&movie;
    if(!c->worker)return 0;
    /* Never fall through to a synchronous read on the worker's shared FILE. */
    if(!destination || !bytes || bytes>c->stats.capacity || at>c->file_bytes || bytes>c->file_bytes-at){*out=-1;return 1;}
    unsigned state=__atomic_load_n(&c->state,__ATOMIC_ACQUIRE);
    if(state==READING){c->stats.busy_polls++;*out=FH_DISC_ASYNC_BUSY;return 1;}
    if(state==READY && at==c->offset && bytes==c->length){
        /* Checked-mode comparison belongs in the synchronous diagnostic API;
         * async completion only copies the already completed private bytes. */
        join(c);*out=c->result;
        if(*out>=0){memcpy(destination,c->buffer,bytes);c->stats.hits++;}
        __atomic_store_n(&c->state,IDLE,__ATOMIC_RELEASE);return 1;
    }
    c->stats.misses++;
    if(c->suspended){*out=FH_DISC_ASYNC_BUSY;return 1;}
    hint_reader(c,at,bytes);c->stats.busy_polls++;*out=FH_DISC_ASYNC_BUSY;return 1;
}
void fh_movie_prefetch_hint(unsigned at,unsigned bytes){hint_reader(&movie,at,bytes);}
void fh_movie_prefetch_wait_pending(void){
    ReadAhead *c=&movie;
    if(!c->worker || c->suspended ||
       __atomic_load_n(&c->state,__ATOMIC_ACQUIRE)!=READING)return;
    /* READY publication precedes the signal. A completion between this test
     * and waiting leaves the one-shot event signalled, so it is not lost.
     * Timeout services original input/reset logic; it never cancels the read. */
    u64 start=svcGetSystemTick();
    LightEvent_WaitTimeout(&c->done,1000000);
    c->stats.wait_ticks+=svcGetSystemTick()-start;
}
void fh_movie_prefetch_stats(FhMoviePrefetchStats *out){*out=movie.stats;}

/* The stream has its own cursor/FILE, unlike main's DVD disc cursor. Only
 * compressed CPU bytes cross threads; decode, count, volume and callbacks
 * remain in their original main-thread order. One chunk ahead (~597 ms at
 * 48043 Hz) suffices without increasing the NDSP queue or audio latency. */
void fh_stream_prefetch_close(void){
    close_reader(&stream);
    if(stream_file)fclose(stream_file);
    stream_file=NULL;
}
void fh_stream_prefetch_open(const char *path,unsigned base,unsigned bytes){
    fh_stream_prefetch_close();
    if(!path || !bytes || bytes>0x7fffffffU || bytes%32 || base>0x7fffffffU-bytes)return;
    stream_file=fopen(path,"rb");if(!stream_file)return;
    if(setvbuf(stream_file,NULL,_IONBF,0)){fh_stream_prefetch_close();return;}
    stream_base=base;stream_bytes=bytes;
    open_reader(&stream,stream_file,32768,base+bytes,1);
    hint_reader(&stream,base,bytes<32768?bytes:32768);
}
int fh_stream_prefetch_read(unsigned at,void *destination,unsigned bytes,int *out){
    if(!stream.worker || at>stream_bytes || bytes>stream_bytes-at)return 0;
    if(!read_reader(&stream,stream_base+at,destination,bytes,out)){
        /* Joined above, so a discontinuous demand safely uses this cursor. */
        *out=fseek(stream_file,stream_base+at,SEEK_SET) || fread(destination,1,bytes,stream_file)!=bytes?-1:(int)bytes;
    }
    if(*out>=0 && at+bytes<stream_bytes){
        unsigned next=stream_bytes-at-bytes;if(next>32768)next=32768;
        hint_reader(&stream,stream_base+at+bytes,next);
    }
    return 1;
}
void fh_stream_prefetch_stats(FhMoviePrefetchStats *out){*out=stream.stats;}

/* Ordinary DVD async requests borrow the caller's buffer only while BUSY.
 * Main joins before cancellation/close releases that buffer and alone publishes
 * completion/cache notifications. The worker has a separate disc cursor. */
void fh_disc_async_close(void){
    close_reader(&disc_reader);
    if(disc_file)fclose(disc_file);
    disc_file=NULL;
}
void fh_disc_async_open(const char *path,unsigned bytes){
    fh_disc_async_close();if(!path || !bytes)return;
    disc_file=fopen(path,"rb");if(!disc_file)return;
    if(setvbuf(disc_file,NULL,_IONBF,0)){fh_disc_async_close();return;}
    open_reader(&disc_reader,disc_file,0,bytes,3);
}
int fh_disc_async_begin(unsigned at,void *destination,unsigned bytes){
    ReadAhead *c=&disc_reader;
    if(!c->worker || c->suspended || !destination || !bytes ||
       at>c->file_bytes || bytes>c->file_bytes-at ||
       __atomic_load_n(&c->state,__ATOMIC_ACQUIRE)!=IDLE)return 0;
    c->offset=at;c->length=bytes;c->destination=destination;c->accounted=0;c->stats.queued++;
    __atomic_store_n(&c->state,READING,__ATOMIC_RELEASE);LightEvent_Signal(&c->wake);return 1;
}
int fh_disc_async_poll(void){
    ReadAhead *c=&disc_reader;
    if(__atomic_load_n(&c->state,__ATOMIC_ACQUIRE)!=READY)return FH_DISC_ASYNC_BUSY;
    join(c);int result=c->result;c->destination=NULL;
    __atomic_store_n(&c->state,IDLE,__ATOMIC_RELEASE);return result;
}
void fh_disc_async_join(void){join(&disc_reader);}
void fh_disc_async_stats(FhMoviePrefetchStats *out){*out=disc_reader.stats;}
