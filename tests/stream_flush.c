/* A host emulator often makes linear memory coherent. This test does not:
 * the GPU sees bytes only through the production helper's flush calls. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint64_t u64;
#define FH_EMULATOR_TEST 1
#define R_FAILED(x) ((x)<0)
static unsigned char cpu[4096] __attribute__((aligned(32))),gpu[4096];
static unsigned stream_flush_bytes,stream_flush_full_bytes,stream_flush_calls,stream_full_flush;
static u64 stream_flush_ticks;
static void check(int ok,const char *message){if(!ok){fprintf(stderr,"%s\n",message);exit(86);}}
static u64 svcGetSystemTick(void){static u64 t;return ++t;}
static void fh_platform_panic(const char *s){check(0,s);}
static int GSPGPU_FlushDataCache(void *p,unsigned size){
    size_t offset=(unsigned char*)p-cpu;
    check(!(offset&31) && !(size&31) && offset+size<=sizeof(cpu),"Invalid cache-line range");
    memcpy(gpu+offset,p,size);return 0;
}
#include "flush.inc"
static unsigned checks;
static void visible(unsigned bytes){check(!memcmp(cpu,gpu,bytes),"GPU saw stale bytes");checks+=bytes;}
int main(void){
    for(stream_full_flush=0;stream_full_flush<2;stream_full_flush++){
        unsigned cursor=0,end=0;
        memset(cpu,0,sizeof(cpu));memset(gpu,0,sizeof(gpu));
        for(unsigned step=0;step<12000;step++){
            if(step%71==0 || end>3800){
                /* Queue completion permits recycling the whole stream. */
                end=cursor=0;
            }else if(step%13==0){
                /* An unsubmitted speculative decode can be rolled back. */
                end/=2;if(cursor>end)cursor=end;
            }
            unsigned previous=end,n=1+(step*17)%79;
            for(unsigned i=0;i<n;i++)cpu[end++]=(unsigned char)(step+i+1);
            flush_stream(cpu,end,&cursor,stream_full_flush && step%3?previous:0);
            visible(end);
            unsigned calls=stream_flush_calls;
            flush_stream(cpu,end,&cursor,0);visible(end);
            if(!stream_full_flush)check(stream_flush_calls==calls,"Unchanged stream flushed twice");
        }
    }
    printf("Noncoherent stream visibility: %u byte comparisons; append, partial cache lines, rollback, reset and full reference passed\n",checks);
    return 0;
}
