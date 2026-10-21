#ifdef FH_OLD_3DS
unsigned fh_hardware_large_memory=0;
#else
unsigned fh_hardware_large_memory=1;
#endif
#ifdef FH_TEST_DVD_ASYNC
#include <3ds.h>
u32 __ctru_heap_size=68*1024*1024,__stacksize__=512*1024;
aptHookCookie *test_hooks;
void (*test_apt_callback)(APT_HookType,void*);
static pthread_t main_thread;
#endif
#include <dolphin/os.h>
#include <dolphin/dvd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_platform.h"
#include "fh_game_memory.h"
#include "fh_movie_prefetch.h"
#ifdef FH_TEST_DVD_ASYNC
#include <unistd.h>
static unsigned delayed_read_us;
void fh_dvd_movie_prefetch(DVDFileInfo *,unsigned,unsigned);
size_t __real_fread(void *,size_t,size_t,FILE *);
size_t __wrap_fread(void *data,size_t size,size_t count,FILE *file){
    unsigned delay=__atomic_exchange_n(&delayed_read_us,0,__ATOMIC_ACQ_REL);
    if(delay)usleep(delay);
    return __real_fread(data,size,count,file);
}
#endif
/* This suite exercises synchronous DVD contracts. Worker ownership has its
 * separate pthread/sanitizer suite and native checked integration. */
#ifndef FH_TEST_DVD_ASYNC
void fh_movie_prefetch_open(FILE *f,unsigned c,unsigned n){(void)f;(void)c;(void)n;}
void fh_movie_prefetch_close(void){}
int fh_movie_prefetch_read(unsigned o,void *d,unsigned n,int *r){(void)o;(void)d;(void)n;(void)r;return 0;}
int fh_movie_prefetch_poll(unsigned o,void *d,unsigned n,int *r){(void)o;(void)d;(void)n;(void)r;return 0;}
void fh_movie_prefetch_hint(unsigned o,unsigned n){(void)o;(void)n;}
void fh_movie_prefetch_wait_pending(void){}

#endif

void *fh_platform_alloc(size_t size,size_t alignment) { void *p=NULL; if(posix_memalign(&p,alignment,size)) return NULL; return p; }
void fh_platform_free(void *p) { free(p); }
void fh_platform_panic(const char *message) { fprintf(stderr,"PANIC: %s\n",message); abort(); }
void fh_platform_cache(void *p,size_t n,int write) { (void)p;(void)n;(void)write; }
static unsigned completion_holds,completion_calls;
int fh_input_dvd_complete(unsigned file,unsigned offset,unsigned bytes,int result){
    assert(file&&offset<=0x7fffffff&&bytes<=0x7fffffff);
    assert(result==(int)bytes||result==-1||result==DVD_RESULT_CANCELED);
    completion_calls++;
    if(completion_holds){completion_holds--;return 0;}return 1;
}
static unsigned callback_count;
static int callback_result;
static void callback(s32 result,DVDFileInfo *file) {
#ifdef FH_TEST_DVD_ASYNC
    assert(pthread_equal(pthread_self(),main_thread));
#endif
 ++callback_count; callback_result=result; assert(file->cb.state!=DVD_STATE_BUSY); }

static void pump_request(DVDFileInfo *file){do{fh_dvd_pump();}while(file->cb.state==DVD_STATE_BUSY);}
int main(int argc,char **argv) {
#ifdef FH_TEST_DVD_ASYNC
    main_thread=pthread_self();fh_movie_prefetch_mode=1;
#endif

    assert(argc==3 || argc==5);
    if(argc==5)assert(!setenv("FOXHOLLOW_PREPARED_MOVIE",argv[3],1));
    OSInit();
    uintptr_t low=(uintptr_t)OSGetArenaLo(),high=(uintptr_t)OSGetArenaHi();
#ifdef FH_OLD_3DS
    assert(high-low==19*1024*1024-0x4000);
    assert(fh_game_region_reserve()==0x780000);
    assert(fh_game_region_size(1)==0x180000 && fh_game_region_size(2)==0x7ff40 && fh_game_region_size(3)==0x57ff40);
#else
    assert(high-low==26*1024*1024-0x4000);
    assert(fh_game_region_reserve()==0xd80000);
    assert(fh_game_region_size(1)==0x380000 && fh_game_region_size(2)==0x13ff40 && fh_game_region_size(3)==0x8bff40);
#endif
    assert(OSAllocFromArenaLo(77,64)==(void*)((low+63)&~(uintptr_t)63));
    assert(((uintptr_t)OSAllocFromArenaHi(101,128)&127)==0);
    assert(OSAllocFromArenaLo(UINT32_MAX,32)==NULL);
    void *begin=OSInitAlloc(OSGetArenaLo(),OSGetArenaHi(),2);
    int heap=OSCreateHeap(begin,OSGetArenaHi());
    assert(heap==0); OSSetCurrentHeap(heap);
    assert(OSCreateHeap(begin,OSGetArenaHi())==-1);
    s32 initial=OSCheckHeap(heap);
    void *allocations[512]={0}; size_t sizes[512]={0};
    uint32_t random=0x41a8c237;
    for(unsigned step=0;step<20000;++step) {
        random=random*1664525+1013904223;
        unsigned slot=random%512;
        if (allocations[slot]) {
            for(size_t i=0;i<sizes[slot];++i) assert(((unsigned char*)allocations[slot])[i]==(unsigned char)slot);
            OSFreeToHeap(heap,allocations[slot]); allocations[slot]=NULL;
        } else {
            sizes[slot]=1+(random>>10)%8192;
            allocations[slot]=OSAllocFromHeap(heap,sizes[slot]);
            assert(allocations[slot] && !((uintptr_t)allocations[slot]&31));
            assert(OSReferentSize(allocations[slot])>=sizes[slot]);
            memset(allocations[slot],slot,sizes[slot]);
        }
    }
    for(unsigned i=0;i<512;++i) if(allocations[i]) OSFreeToHeap(heap,allocations[i]);
    assert(OSCheckHeap(heap)==initial && fh_memory_used()==0 && fh_memory_peak()>0);
    assert(OSAllocFromHeap(heap,UINT32_MAX)==NULL);
    void *all=OSAllocFromHeap(heap,initial);
    assert(all && OSCheckHeap(heap)==0 && OSAllocFromHeap(heap,1)==NULL);
    OSFreeToHeap(heap,all); OSDestroyHeap(heap); fh_memory_shutdown();

    assert(!fh_dvd_mount("/does-not-exist.iso"));
    assert(fh_dvd_mount(argv[1]));
    assert(!memcmp(DVDGetCurrentDiskID()->gameName,"GSAP",4));
    assert(DVDGetCurrentDiskID()->gameVersion==1);
    DVDFileInfo file;
    assert(DVDOpen("/anim.tab",&file));
    unsigned char expected[128],actual[160];
    FILE *source=fopen(argv[2],"rb"); assert(source);
    assert(fread(expected,1,sizeof(expected),source)==sizeof(expected)); fclose(source);
    memset(actual,0xa5,sizeof(actual));
    assert(DVDReadPrio(&file,actual,128,0,2)==128);
    assert(!memcmp(expected,actual,128) && actual[128]==0xa5);
    assert(DVDReadPrio(&file,actual,32,file.length,2)==-1);
    assert(DVDReadPrio(&file,actual,31,file.length,2)==31);
    for(unsigned i=0;i<31;++i) assert(actual[i]==0);
    assert(DVDReadPrio(&file,actual,16,-1,2)==-1);
    assert(DVDReadPrio(&file,NULL,16,0,2)==-1);
    assert(DVDReadAsyncPrio(&file,actual,128,0,callback,2));
    assert(file.cb.addr==actual && file.cb.length==128 && file.cb.offset==file.startAddr && file.callback==callback);
    assert(file.cb.state==DVD_STATE_BUSY && callback_count==0);
    assert(!DVDReadAsyncPrio(&file,actual,8,0,callback,2));
    completion_holds=100;
    while(completion_holds){fh_dvd_pump();assert(file.cb.state==DVD_STATE_BUSY&&!file.cb.transferredSize&&!callback_count);}
    assert(completion_calls==100&&!memcmp(actual,expected,128));
    pump_request(&file);
    assert(completion_calls==101);
    assert(callback_count==1 && callback_result==128 && !memcmp(actual,expected,128));
    assert(file.cb.addr==actual && file.cb.transferredSize==128);
    assert(DVDReadAsyncPrio(&file,actual,8,0,callback,2));
    assert(DVDCancel(&file.cb)==0);
    assert(DVDClose(&file));
    assert(callback_count==2 && callback_result==DVD_RESULT_CANCELED);
    assert(DVDReadPrio(&file,actual,16,0,2)==-1);
#ifdef FH_TEST_DVD_ASYNC
    assert(DVDOpen("/anim.tab",&file));DVDFileInfo other;assert(DVDOpen("/anim.tab",&other));
    for(unsigned i=0;i<64;i++){
        unsigned n=1+i*2,before=callback_count;memset(actual,0xa5,sizeof(actual));
        assert(DVDReadAsyncPrio(&file,actual,n,0,callback,2));fh_dvd_pump();
        assert(file.cb.state==DVD_STATE_BUSY && callback_count==before);
        unsigned char parallel[128];assert(DVDReadPrio(&other,parallel,128,0,2)==128 && !memcmp(parallel,expected,128));
        if(i%4==0){assert(!DVDCancel(&file.cb));pump_request(&file);assert(callback_result==DVD_RESULT_CANCELED);}
        else{pump_request(&file);assert(callback_result==(int)n && !memcmp(actual,expected,n));}
        assert(callback_count==before+1 && actual[n]==0xa5);
    }
    assert(DVDReadAsyncPrio(&file,actual,31,file.length,callback,2));pump_request(&file);
    for(unsigned i=0;i<31;i++)assert(actual[i]==0);
    assert(DVDReadAsyncPrio(&file,actual,128,0,callback,2));fh_dvd_pump();assert(DVDClose(&file));
    assert(DVDClose(&other));
    DVDFileInfo burst[94];unsigned char data[94][128];unsigned before=callback_count;
    for(unsigned i=0;i<94;i++){
        assert(DVDOpen("/anim.tab",burst+i));
        assert(DVDReadAsyncPrio(burst+i,data[i],128,0,callback,2));
    }
    for(unsigned i=0;i<94;i++){
        pump_request(burst+i);assert(!memcmp(data[i],expected,128));assert(DVDClose(burst+i));
    }
    assert(callback_count==before+94);FhMoviePrefetchStats stats;fh_disc_async_stats(&stats);
    assert(stats.queued==160 && !stats.errors);puts("PASS: 160 worker reads including a 94-request boot burst, concurrent synchronous cursor, main callbacks, in-flight cancellation/close");
#endif

    assert(DVDChangeDir("/audio"));
    char path[128]; assert(DVDGetCurrentDir(path,sizeof(path)) && !strcasecmp(path,"/audio"));
    assert(DVDConvertPathToEntrynum("../ANIM.TAB")>=0);
    assert(DVDChangeDir("/") && DVDConvertPathToEntrynum("missing.bin")==-1);
    assert(!DVDOpen("/audio",&file));
    assert(DVDOpen("/starfox.thp",&file));
    assert(file.length==(argc==5?(unsigned)strtoul(argv[4],NULL,10):48931320u));
    assert(DVDReadPrio(&file,actual,128,0,2)==128 && !memcmp(actual,"THP\0",4));
    if(file.length!=48931320u){
        unsigned char tag[64];
#ifdef FH_TEST_DVD_ASYNC
        /* Reproduce physical SD latency beyond the old fatal three seconds.
         * The actual DVD queue must stay responsive, preserve main callbacks,
         * and never lend the cancelled destination to the movie worker. */
        memset(tag,0xa5,sizeof(tag));unsigned before=callback_count;
        __atomic_store_n(&delayed_read_us,6000000,__ATOMIC_RELEASE);
        fh_dvd_movie_prefetch(&file,64,128);
        assert(DVDReadAsyncPrio(&file,tag,64,128,callback,2));
        u64 start=svcGetSystemTick(),max_poll=0;unsigned polls=0;
        do{
            u64 t=svcGetSystemTick();fh_dvd_pump();u64 dt=svcGetSystemTick()-t;
            if(dt>max_poll)max_poll=dt;
            assert(file.cb.state==DVD_STATE_BUSY && callback_count==before);
            for(unsigned i=0;i<sizeof(tag);i++)assert(tag[i]==0xa5);
            polls++;usleep(1000);
        }while(svcGetSystemTick()-start<3400000000ULL);
        u64 t=svcGetSystemTick();assert(!DVDCancel(&file.cb));fh_dvd_pump();
        assert(svcGetSystemTick()-t<250000000ULL);
        assert(callback_count==before+1 && callback_result==DVD_RESULT_CANCELED);
        /* A different demand must wait for stale owned bytes and then reread. */
        unsigned char scratch[64];memset(scratch,0xa5,sizeof(scratch));
        assert(DVDReadAsyncPrio(&file,scratch,64,192,callback,2));
        while(file.cb.state==DVD_STATE_BUSY){
            t=svcGetSystemTick();fh_dvd_pump();u64 dt=svcGetSystemTick()-t;
            if(dt>max_poll)max_poll=dt;
            assert(svcGetSystemTick()-start<9000000000ULL);usleep(1000);
        }
        assert(max_poll<250000000ULL && polls>100 && callback_result==64);
        for(unsigned i=0;i<sizeof(tag);i++)assert(tag[i]==0xa5);
        FILE *reference=fopen(argv[3],"rb");assert(reference && !fseek(reference,192,SEEK_SET));
        unsigned char expected_movie[64];assert(fread(expected_movie,1,64,reference)==64);fclose(reference);
        assert(!memcmp(scratch,expected_movie,64));
        FhMoviePrefetchStats movie_stats;fh_movie_prefetch_stats(&movie_stats);
        assert(movie_stats.slow_reads==1 && movie_stats.busy_polls>100);
        printf("PASS: six-second movie delay, %u responsive polls, max poll %.3f ms, immediate cancel, untouched retired buffer, byte-exact replacement\n",polls,max_poll/1000000.0);
#endif
        assert(DVDReadAsyncPrio(&file,tag,64,128,callback,2));pump_request(&file);
        assert(callback_result==64 && !memcmp(tag,"FHPICA01",8));
        assert(DVDReadPrio(&file,tag,31,file.length,2)==31);
        for(unsigned i=0;i<31;i++)assert(!tag[i]);
    }
    assert(DVDClose(&file));
    fh_dvd_shutdown();
    puts("PASS: arena bounds, 20000 allocation operations, full coalescing, PAL ISO identity, file bounds, padded reads, deferred callbacks and cancellation");
    return 0;
}

void fh_platform_log(const char *message){(void)message;}
#ifndef FH_TEST_DVD_ASYNC
void fh_stream_prefetch_open(const char *p,unsigned b,unsigned n){(void)p;(void)b;(void)n;}
void fh_stream_prefetch_close(void){}
int fh_stream_prefetch_read(unsigned a,void *d,unsigned n,int *r){(void)a;(void)d;(void)n;(void)r;return 0;}
void fh_stream_prefetch_stats(FhMoviePrefetchStats *s){*s=(FhMoviePrefetchStats){0};}
void fh_disc_async_open(const char *p,unsigned n){(void)p;(void)n;}
void fh_disc_async_close(void){}
void fh_disc_async_join(void){}
int fh_disc_async_begin(unsigned a,void *d,unsigned n){(void)a;(void)d;(void)n;return 0;}
int fh_disc_async_poll(void){abort();}
void fh_disc_async_stats(FhMoviePrefetchStats *s){*s=(FhMoviePrefetchStats){0};}

#endif
