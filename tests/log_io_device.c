#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "fh_log_io.h"
#include "fh_diagnostic_buffer.h"
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=1024*1024,__stacksize__=64*1024;
static unsigned checks,calls,stdio_writes,stdio_flags,observe_stdio;
static unsigned char payload[70001];
Result __real_FSFILE_Write(Handle,u32*,u64,const void*,u32,u32);
Result __wrap_FSFILE_Write(Handle h,u32 *got,u64 offset,const void *data,u32 size,u32 flags) {
    if(observe_stdio){stdio_writes++;stdio_flags|=flags;}
    return __real_FSFILE_Write(h,got,offset,data,size,flags);
}
static void check(int condition) {
    checks++;
    if(!condition){FILE *f=fopen("sdmc:/3ds/foxhollow-log-test/failure.txt","wb");if(f){fprintf(f,"check %u",checks);fclose(f);}exit(1);}
}
static Result write_delayed(Handle h,u32 *got,u64 offset,const void *data,u32 size,u32 flags) {
    ++calls;svcSleepThread(2000000); /* Force queue saturation without long boots. */
    if(h==0x12345678){*got=size-1;return 0;} /* Short-write error propagation. */
    if(h==0x12345679){*got=0;return MAKERESULT(RL_PERMANENT,RS_INTERNAL,RM_FS,RD_NO_DATA);}
    return FSFILE_Write(h,got,offset,data,size,flags);
}
static void enqueue_fixture(Handle h,u64 offset,unsigned count,unsigned seed) {
    for(unsigned i=0;i<count;i++)payload[i]=(unsigned char)(i*37+seed);
    u32 got=0;Result result=0;
    check(fh_log_io_write(h,&got,offset,payload,count,FS_WRITE_FLUSH,&result));check(R_SUCCEEDED(result));check(got==count);
    memset(payload,0xcc,count); /* Producer storage can immediately change. */
}
static void verify(Handle h,u64 offset,unsigned count,unsigned seed) {
    u32 got=0;check(R_SUCCEEDED(FSFILE_Read(h,&got,offset,payload,count)));check(got==count);
    for(unsigned i=0;i<count;i++)check(payload[i]==(unsigned char)(i*37+seed));
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-log-test",0777);gfxInitDefault();
    /* Real newlib/libctru stdio, not just the mock queue callback: all five
     * streams must retain a short run without filesystem write requests. */
    FILE *streams[5];observe_stdio=1;
    for(unsigned i=0;i<5;i++){
        char path[128];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow-log-test/stdio-%u.bin",i);
        streams[i]=fopen(path,"wb");check(streams[i]!=NULL);check(!fh_diagnostic_buffer(streams[i]));
        for(unsigned j=0;j<49152;j++)check(fputc((j*37+i)&255,streams[i])!=EOF);
    }
    check(stdio_writes==0);
    for(unsigned i=0;i<5;i++)check(!fclose(streams[i]));
    check(stdio_writes>=5);check(stdio_flags==0);observe_stdio=0;
    fh_log_io_init(write_delayed);FhLogIoStats stats;fh_log_io_stats(&stats);check(stats.active);
    Handle a=0,b=0;check(R_SUCCEEDED(FSUSER_OpenFileDirectly(&a,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""),fsMakePath(PATH_ASCII,"/3ds/foxhollow-log-test/a.bin"),FS_OPEN_READ|FS_OPEN_WRITE|FS_OPEN_CREATE,0)));
    check(R_SUCCEEDED(FSUSER_OpenFileDirectly(&b,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""),fsMakePath(PATH_ASCII,"/3ds/foxhollow-log-test/b.bin"),FS_OPEN_READ|FS_OPEN_WRITE|FS_OPEN_CREATE,0)));
    fh_log_io_track(a,fsMakePath(PATH_ASCII,"/3ds/foxhollow/minimal-render.csv"));
    const u16 name[]={'/','3','d','s','/','f','o','x','h','o','l','l','o','w','/','r','e','n','d','e','r','.','c','s','v',0};
    fh_log_io_track(b,fsMakePath(PATH_UTF16,name));
    enqueue_fixture(a,0,70001,3);enqueue_fixture(b,0,37003,9);enqueue_fixture(a,70001,17005,51);enqueue_fixture(b,37003,70001,121);
    check(R_SUCCEEDED(fh_log_io_drain(a)));
    verify(a,0,70001,3);verify(a,70001,17005,51);verify(b,0,37003,9);verify(b,37003,70001,121);
    fh_log_io_forget(a);fh_log_io_forget(b);check(R_SUCCEEDED(FSFILE_Close(a)));check(R_SUCCEEDED(FSFILE_Close(b)));
    u32 got=0;Result result=0;Handle excluded=0x12345670;
    const char *private_paths[]={"/3ds/foxhollow/save.gci","/3ds/foxhollow/disc.iso","/other/native.log","/3ds/foxhollow/native.log.bak"};
    for(unsigned i=0;i<4;i++){fh_log_io_track(excluded,fsMakePath(PATH_ASCII,private_paths[i]));check(!fh_log_io_write(excluded,&got,0,payload,1,0,&result));}
    fh_log_io_track(0x12345678,fsMakePath(PATH_ASCII,"/3ds/foxhollow/native.log"));enqueue_fixture(0x12345678,0,31,0);check(R_FAILED(fh_log_io_drain(0x12345678)));fh_log_io_forget(0x12345678);
    fh_log_io_track(0x12345679,fsMakePath(PATH_ASCII,"/3ds/foxhollow/native.log"));enqueue_fixture(0x12345679,0,31,0);check(R_FAILED(fh_log_io_drain(0x12345679)));fh_log_io_forget(0x12345679);
    check(R_SUCCEEDED(fh_log_io_shutdown()));fh_log_io_stats(&stats);check(!stats.active && !stats.queued);check(stats.errors==2);check(stats.peak==1 && stats.waits);check(calls==8);
    FILE *f=fopen("sdmc:/3ds/foxhollow-log-test/result.json","wb");if(f){fprintf(f,"{\"checks\":%u,\"writes\":%u,\"peak_records\":%u,\"backpressure_waits\":%u,\"injected_errors\":%u,\"joined\":true}\n",checks,calls,stats.peak,stats.waits,stats.errors);fclose(f);}
    gfxExit();return 0;
}
