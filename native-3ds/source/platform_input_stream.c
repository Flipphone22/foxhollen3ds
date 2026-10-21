#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include <unistd.h>
#include "fh_input_tape.h"
/* SDK thread on the default application affinity; no extra New-only core.
 * One file owner, release/acquire publication, no GPU access. */
static FhTapeRing *ring;
static Thread worker;
static FILE *file;
/* Reserve the file during recording startup, not by extending FAT clusters
 * amid asset streaming. Melee's file/log workers provide the ownership model;
 * preallocation is recording-specific. Keep wire format/CRC unchanged.
 * Larger bounded buffering absorbs SD latency; the producer still never waits. */
enum { TAPE_FILE_RESERVE=64*1024*1024 };
static char io_buffer[64*1024];
static unsigned writing,stopping,records;
static unsigned written_bytes;
static u64 reserve_ticks,finalize_ticks;
static double stop_tick;
static u64 io_ticks,max_io_ticks;
static LightEvent wake;
void fh_tape_stream_notify(void){
    if(!ring)return;
    unsigned used=__atomic_load_n(&ring->head,__ATOMIC_ACQUIRE)-__atomic_load_n(&ring->tail,__ATOMIC_ACQUIRE);
    unsigned progress=writing?used:FH_TAPE_RING-used;
    if((progress&63)==1||progress>=FH_TAPE_RING-64)LightEvent_Signal(&wake);
}
void fh_tape_stream_invalidate(void){if(ring)__atomic_store_n(&ring->error,1,__ATOMIC_RELEASE);}
static void error(void){__atomic_store_n(&ring->error,1,__ATOMIC_RELEASE);}
static int packet(unsigned char*b,unsigned write){
    if(write && written_bytes>TAPE_FILE_RESERVE-FH_TAPE_WIRE)return 0;
    u64 start=svcGetSystemTick();int ok=write?fwrite(b,1,FH_TAPE_WIRE,file)==FH_TAPE_WIRE:fread(b,1,FH_TAPE_WIRE,file)==FH_TAPE_WIRE;
    if(write && ok)written_bytes+=FH_TAPE_WIRE;
    u64 dt=svcGetSystemTick()-start;io_ticks+=dt;if(dt>max_io_ticks)max_io_ticks=dt;return ok;
}
static int ordered_sample(const FhTapeSample*s,unsigned count,double previous){
    if(!count)return s->phase==FH_TAPE_ANCHOR&&s->tick==0&&s->flags==FH_TAPE_CHECK;
    if(s->phase==FH_TAPE_CLOCK)return !s->flags&&s->tick==previous+(double)s->delta;
    if(s->phase==FH_TAPE_IO)return ring->version>=4&&!s->flags&&s->tick==previous;
    if(s->phase==FH_TAPE_TIME)return ring->version>=5&&!s->flags&&s->tick==previous;
    return s->phase==FH_TAPE_PAD&&s->tick==previous&&s->flags==FH_TAPE_CHECK;
}
static void stream_thread(void*unused){
    (void)unused;unsigned char bytes[FH_TAPE_WIRE];uLong crc=crc32(0,0,0);double last=-1;int ended=0;
    while(!__atomic_load_n(&ring->error,__ATOMIC_ACQUIRE)){
        FhTapeSample s;
        if(writing){
            if(!fh_tape_peek(ring,&s)){
                if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE)){
                    /* The producer may have queued its final sample between the
                     * empty observation and publishing stop. Acquire stop first,
                     * then recheck so that sample cannot be lost at shutdown. */
                    if(fh_tape_peek(ring,&s))continue;
                    s=(FhTapeSample){.tick=stop_tick,.flags=FH_TAPE_END};
                    if(stop_tick<last||!records||!fh_tape_encode(bytes,&s)){error();break;}
                    for(unsigned i=0;i<4;i++)bytes[60+i]=(unsigned char)(crc>>(i*8));
                    if(!packet(bytes,1))error();
                    ended=1;break;
                }
                LightEvent_WaitTimeout(&wake,50000000);continue;
            }
            if(!ordered_sample(&s,records,last)||!fh_tape_encode(bytes,&s)||!packet(bytes,1)){error();break;}
            crc=crc32(crc,bytes,sizeof(bytes));last=s.tick;records++;fh_tape_pop(ring);
        }else{
            if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))break;
            if(!packet(bytes,0)||!fh_tape_decode(&s,bytes)){error();break;}
            if(s.flags==FH_TAPE_END){
                unsigned stored=(unsigned)bytes[60]|(unsigned)bytes[61]<<8|(unsigned)bytes[62]<<16|(unsigned)bytes[63]<<24;
                if(!records||s.tick<last||stored!=(unsigned)crc||fgetc(file)!=EOF||ferror(file)){error();break;}ended=1;
            }else{
                if(ring->version>=3 ? !ordered_sample(&s,records,last) :
                   ((!records&&s.tick!=0)||s.tick<=last||s.phase||s.delta)){error();break;}
                crc=crc32(crc,bytes,sizeof(bytes));last=s.tick;records++;
            }
            while(!fh_tape_push(ring,&s)){
                if(__atomic_load_n(&stopping,__ATOMIC_ACQUIRE))goto done;
                LightEvent_WaitTimeout(&wake,50000000);
            }
            if(ended)break;
        }
    }
done:
    if(writing){
        u64 start=svcGetSystemTick();
        if(fflush(file) || ftruncate(fileno(file),written_bytes))error();
        finalize_ticks=svcGetSystemTick()-start;
    }
    if(fclose(file))error();
    file=NULL;
    __atomic_store_n(&ring->done,ended?1:2,__ATOMIC_RELEASE);
}
int fh_tape_stream_open(const char*path,unsigned write){
    if(worker||ring)return 0;
    ring=calloc(1,sizeof(*ring));if(!ring)return 0;
    writing=write;stopping=records=0;io_ticks=max_io_ticks=reserve_ticks=finalize_ticks=0;written_bytes=0;stop_tick=0;
    file=fopen(path,write?"wb":"rb");if(!file){free(ring);ring=NULL;return 0;}
    if(setvbuf(file,io_buffer,_IOFBF,sizeof(io_buffer))){fclose(file);file=NULL;free(ring);ring=NULL;return 0;}
    if(write){
        u64 start=svcGetSystemTick();int ok=!ftruncate(fileno(file),TAPE_FILE_RESERVE);
        reserve_ticks=svcGetSystemTick()-start;
        if(!ok){fclose(file);file=NULL;free(ring);ring=NULL;return 0;}
    }
    const char magic[16]="FHTAPE5\n64LE\n",legacy[16]="FHTAPE2\n64LE\n",ordered[16]="FHTAPE3\n64LE\n",io[16]="FHTAPE4\n64LE\n";char actual[16];
    ring->version=5;
    int ok;
    if(write){ok=fwrite(magic,1,16,file)==16;if(ok)written_bytes=16;}
    else {ok=fread(actual,1,16,file)==16;
        if(ok&&!memcmp(actual,legacy,16))ring->version=2;
        else if(ok&&!memcmp(actual,ordered,16))ring->version=3;
        else if(ok&&!memcmp(actual,io,16))ring->version=4;
        else if(ok&&memcmp(actual,magic,16))ok=0;}
    if(!ok){fclose(file);file=NULL;free(ring);ring=NULL;return 0;}
    /* Pinned Melee workers also own bounded stacks and are joined before free. */
    /* Pinned Melee file/log workers run one priority above the caller and
     * wake on published work. Polling below the producer can lose a startup
     * burst even when actual SD writes are fast. No New-only core is needed. */
    LightEvent_Init(&wake,RESET_ONESHOT);
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    worker=threadCreate(stream_thread,NULL,32*1024,priority>0x18?priority-1:priority,-2,false);
    if(!worker){fclose(file);file=NULL;free(ring);ring=NULL;return 0;}
    if(!write){
        u64 start=svcGetSystemTick();
        while(__atomic_load_n(&ring->head,__ATOMIC_ACQUIRE)<FH_TAPE_RING/2&&!__atomic_load_n(&ring->done,__ATOMIC_ACQUIRE)&&!__atomic_load_n(&ring->error,__ATOMIC_ACQUIRE)){
            if(svcGetSystemTick()-start>5ULL*268111856){error();break;}svcSleepThread(1000000);
        }
        if(__atomic_load_n(&ring->error,__ATOMIC_ACQUIRE)){fh_tape_stream_close(0);return 0;}
    }
    return 1;
}
FhTapeRing *fh_tape_stream_ring(void){return ring;}
int fh_tape_stream_close(double duration){
    if(!ring)return 1;
    stop_tick=duration;__atomic_store_n(&stopping,1,__ATOMIC_RELEASE);
    LightEvent_Signal(&wake);
    /* Only called at menu preparation failure, explicit stop or shutdown. */
    threadJoin(worker,U64_MAX);threadFree(worker);worker=NULL;
    int ok=!__atomic_load_n(&ring->error,__ATOMIC_ACQUIRE)&&(!writing||ring->done==1);
    char text[384];snprintf(text,sizeof(text),"Input stream: write=%u records=%u peak=%u/%u ram=%u stack=32768 io_buffer=%u io_ms=%.3f max_io_ms=%.3f reserve_ms=%.3f finalize_ms=%.3f written_bytes=%u status=%s\n",writing,records,ring->peak,FH_TAPE_RING,(unsigned)sizeof(*ring),(unsigned)sizeof(io_buffer),io_ticks*1000./268111856.,max_io_ticks*1000./268111856.,reserve_ticks*1000./268111856.,finalize_ticks*1000./268111856.,written_bytes,ok?"ok":"invalid");fh_platform_log(text);
    free(ring);ring=NULL;return ok;
}
