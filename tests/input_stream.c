#include <3ds.h>
#include "fh_input_tape.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
void fh_platform_log(const char*s){fputs(s,stdout);}
int main(void){
    const char*path="stream.fht";assert(fh_tape_stream_open(path,1));
    struct stat info;assert(!stat(path,&info)&&info.st_size==64*1024*1024);
    FhTapeRing*r=fh_tape_stream_ring();
    for(unsigned i=0;i<10000;i++){
        FhTapeSample s={.tick=i,.ui=(int)i,.pad={.x=(int8_t)(i%127)},.phase=i?FH_TAPE_CLOCK:FH_TAPE_ANCHOR,.delta=i?1:0,.flags=i?0:FH_TAPE_CHECK};
        while(!fh_tape_push(r,&s)){assert(!__atomic_load_n(&r->error,__ATOMIC_ACQUIRE));svcSleepThread(100000);}
        fh_tape_stream_notify();
        if(!(i%1000))assert(!stat(path,&info)&&info.st_size==64*1024*1024);
    }
    assert(fh_tape_stream_close(10000));assert(!stat(path,&info)&&info.st_size==16+10001*64);assert(fh_tape_stream_open(path,0));r=fh_tape_stream_ring();
    for(unsigned i=0;i<10001;i++){
        FhTapeSample s;while(!fh_tape_peek(r,&s)){assert(!__atomic_load_n(&r->error,__ATOMIC_ACQUIRE));svcSleepThread(100000);}
        assert(s.tick==i);if(i<10000)assert(s.ui==(int)i&&s.pad.x==(int)(i%127));else assert(s.flags==FH_TAPE_END);fh_tape_pop(r);fh_tape_stream_notify();
    }
    assert(fh_tape_stream_close(0));
    FILE*f=fopen(path,"r+b");assert(f);fseek(f,16+100*64+16,SEEK_SET);int b=fgetc(f);fseek(f,-1,SEEK_CUR);fputc(b^1,f);fclose(f);
    assert(fh_tape_stream_open(path,0));r=fh_tape_stream_ring();
    for(unsigned guard=0;guard<20000&&!__atomic_load_n(&r->error,__ATOMIC_ACQUIRE);guard++){
        FhTapeSample s;if(fh_tape_peek(r,&s)){fh_tape_pop(r);fh_tape_stream_notify();}else svcSleepThread(100000);
    }
    assert(__atomic_load_n(&r->error,__ATOMIC_ACQUIRE));assert(!fh_tape_stream_close(0));
    /* Close immediately after publishing the final sample, including a worker
     * that has already observed an empty queue. Every sample must reach disk. */
    for(unsigned run=0;run<128;run++){
        assert(fh_tape_stream_open(path,1));r=fh_tape_stream_ring();
        FhTapeSample s={.tick=0,.ui=(int)run,.phase=FH_TAPE_ANCHOR,.flags=FH_TAPE_CHECK};
        assert(fh_tape_push(r,&s));
        if(run&1)svcSleepThread(100000);
        s.tick=1;s.ui++;s.phase=FH_TAPE_CLOCK;s.flags=0;s.delta=1;
        assert(fh_tape_push(r,&s));assert(fh_tape_stream_close(2));
        assert(fh_tape_stream_open(path,0));r=fh_tape_stream_ring();
        for(unsigned i=0;i<3;i++){
            assert(fh_tape_peek(r,&s));assert(s.tick==i);
            if(i<2)assert(s.ui==(int)(run+i));else assert(s.flags==FH_TAPE_END);
            fh_tape_pop(r);fh_tape_stream_notify();
        }
        assert(fh_tape_stream_close(0));
    }
    puts("PASS: actual asynchronous FILE owner, 10000 streamed packets, prefetch/close, full-stream CRC corruption rejection");
    puts("PASS: 128 immediate-stop drain checks preserve the last queued sample; startup reserves disk capacity, no file growth while streaming, final truncation preserves exact CRC-terminated length");
}
