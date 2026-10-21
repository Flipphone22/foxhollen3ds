#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "fh_startup_runner.h"
enum { SD_BENCH_BUFFER=1024*1024, SD_BENCH_PATTERN=446 };
static const unsigned sd_bench_pattern[SD_BENCH_PATTERN][2]={{66000328,32768},{90762344,10912},{66033096,32768},{36204808,32768},{88094472,5440},{39392456,32768},{88352072,10912},{39425224,32768},{54039464,32768},{89672200,10912},{54072232,32768},{3878280,24576},{85139880,5376},{61171624,8192},{90331176,1344},{56416872,32768},{89894056,10912},{56449640,32768},{48078152,32768},{89075656,10912},{48110920,32768},{80439240,32768},{92086600,5376},{7997288,32768},{85454920,21824},{8030056,32768},{8062824,32768},{80414664,24576},{92081224,5376},{5182600,16384},{85258632,2720},{54039464,32768},{61171624,8192},{56416872,32768},{26405256,24576},{87225256,5440},{18203592,16384},{86474600,2720},{7997288,32768},{80414664,24576},{48078152,32768},{61171624,8192},{82645608,32768},{92286984,10912},{82678376,32768},{7997288,32768},{80414664,24576},{61171624,8192},{74628872,64},{54039464,32768},{36204808,32768},{7997288,32768},{80414664,24576},{5182600,16384},{48078152,32768},{61171624,8192},{18203592,16384},{82645608,32768},{54039464,32768},{66000328,32768},{80439240,32768},{7997288,32768},{80414664,24576},{61171624,8192},{36204808,32768},{56416872,32768},{54039464,32768},{66000328,32768},{5182600,16384},{7997288,32768},{80414664,24576},{48078152,32768},{79237288,16384},{91965704,2720},{28615816,64},{61171624,8192},{82645608,32768},{7997288,32768},{80414664,24576},{56416872,32768},{66000328,32768},{79237288,16384},{28615816,64},{61171624,8192},{74628872,64},{18203592,16384},{36204808,32768},{56416872,32768},{26405256,24576},{39392456,32768},{66000328,32768},{5182600,16384},{80439240,32768},{79237288,16384},{48078152,32768},{56416872,32768},{66582984,24576},{90826888,5376},{18203592,16384},{5182600,16384},{36204808,32768},{39392456,32768},{61171624,8192},{48078152,32768},{26405256,24576},{36204808,32768},{39392456,32768},{66000328,32768},{82645608,32768},{79237288,16384},{66582984,24576},{5182600,16384},{28615816,64},{61171624,8192},{82645608,32768},{26405256,24576},{36204808,32768},{5182600,16384},{54039464,32768},{66000328,32768},{56416872,32768},{7997288,32768},{80414664,24576},{28615816,64},{61171624,8192},{56416872,32768},{39392456,32768},{82645608,32768},{26405256,24576},{7997288,32768},{80414664,24576},{74628872,64},{66000328,32768},{36204808,32768},{7997288,32768},{80414664,24576},{80242408,12288},{92065096,2688},{79237288,16384},{61171624,8192},{56416872,32768},{66000328,32768},{7997288,32768},{80242408,12288},{66582984,24576},{79237288,16384},{5182600,16384},{48078152,32768},{56416872,32768},{66000328,32768},{82645608,32768},{66582984,24576},{28615816,64},{61171624,8192},{36204808,32768},{56416872,32768},{82645608,32768},{66000328,32768},{39392456,32768},{48078152,32768},{7997288,32768},{80414664,24576},{28615816,64},{29598824,32768},{87496968,5440},{61171624,8192},{36204808,32768},{54039464,32768},{56416872,32768},{39392456,32768},{7997288,32768},{80414664,24576},{3878280,24576},{79237288,16384},{61171624,8192},{29598824,32768},{74628872,64},{39392456,32768},{56416872,32768},{7997288,32768},{80414664,24576},{3878280,24576},{79237288,16384},{29598824,32768},{61171624,8192},{82645608,32768},{56416872,32768},{5182600,16384},{3878280,24576},{36204808,32768},{7997288,32768},{80414664,24576},{68102440,16384},{90984712,2560},{28615816,64},{29598824,32768},{61171624,8192},{56416872,32768},{7997288,32768},{3878280,24576},{80414664,24576},{28615816,64},{75937800,16384},{91693512,2720},{52154312,4096},{89463720,672},{72404776,4096},{91369672,672},{19473320,32768},{86597896,5440},{35660008,32768},{88033544,5440},{72419112,4096},{91371016,672},{14303720,32768},{14336488,10240},{40678920,4096},{4859272,32768},{85231400,10912},{4892040,32768},{21371624,6144},{86761736,1280},{2375880,6144},{85019240,1280},{48954376,6144},{89182664,1280},{14294056,4096},{86066088,672},{37833256,256},{52197864,1024},{89464392,160},{77606824,2048},{91836552,320},{35026088,32768},{35058856,32768},{35091624,32768},{35124392,32768},{35157160,32768},{35189928,18432},{16433800,32768},{16466568,32768},{80557640,32768},{92097416,5440},{14303720,32768},{21393512,16384},{86763688,2720},{28615880,8192},{87401832,1344},{45113000,4096},{88854952,640},{73673832,32768},{91497352,5440},{83043496,32768},{92325672,5440},{49589096,4096},{89226792,640},{61287336,64},{70667720,16384},{91237832,2720},{60827112,16384},{90311368,2720},{78952680,32768},{91934856,10880},{78985448,16384},{13559944,16384},{85969544,2560},{61127848,8192},{90325288,1344},{31464680,12288},{87653160,2688},{79847656,4096},{92022824,672},{65909608,32768},{90755624,5376},{37220872,16384},{88192424,2720},{11255752,4096},{85745352,672},{81900456,4096},{92239560,672},{16888744,4096},{86332072,640},{79903464,4096},{92030280,640},{23746216,8192},{86952712,1344},{9562760,32768},{85592904,10912},{9595528,32768},{66000328,32768},{36204808,32768},{48858280,16384},{89169768,2720},{61580936,16384},{90372648,2720},{82916712,32768},{92312648,10912},{82949480,32768},{35354856,6144},{88004744,1280},{78538984,6144},{91889896,1280},{34202248,6144},{87927752,1280},{26869416,6144},{87264872,1280},{3954184,6144},{85147528,1280},{75936776,1024},{91693352,160},{65326056,1024},{66966440,8192},{90865736,1344},{58495880,512},{90105960,64},{76255688,16384},{91721864,2720},{6369544,32768},{85331688,5440},{16606760,16384},{86305992,2688},{20619336,32768},{20652104,32768},{20684872,32768},{20717640,32768},{86685384,43680},{20750408,32768},{20783176,32768},{20815944,32768},{20848712,32768},{31354088,6144},{87630376,1280},{29433768,6144},{87486888,1280},{48018888,6144},{89072904,1280},{8277224,6144},{85493256,1280},{47203624,6144},{88995208,1280},{66099496,6144},{90775880,1280},{52278760,4096},{89475304,672},{78485224,32768},{91884456,5440},{16606760,16384},{49057000,32768},{89186536,5376},{18236360,32768},{86480040,10880},{18269128,16384},{6679880,32768},{6712648,32768},{6745416,32768},{6778184,32768},{85338472,43680},{6810952,32768},{6843720,32768},{6876488,32768},{6909256,32768},{75681160,1536},{91682856,256},{49722472,1024},{89232104,160},{20208712,4608},{86660808,1280},{37833256,256},{13338952,32768},{13371720,32768},{85941992,21824},{13404488,32768},{13437256,32768},{49102056,6144},{89193192,1280},{3941896,6144},{85146248,1280},{29551720,6144},{87492968,1280},{9979976,6144},{85630504,1280},{22614344,6144},{86867720,1280},{47334376,6144},{89003368,1280},{80327688,6144},{92071784,1280},{84114088,6144},{92414152,1280},{26091976,1024},{87199880,160},{13338952,32768},{13371720,32768},{7381832,24576},{85408520,5376},{82916712,32768},{35354856,6144},{78538984,6144},{34202248,6144},{26869416,6144},{3954184,6144},{75936776,1024},{5738536,12288},{72619784,32768},{91390760,10880},{72652552,32768},{29982024,16384},{87509160,2560},{23871496,16384},{86965608,2560},{64498536,16384},{90642728,2560},{33947976,16384},{87898760,2560},{54133544,2048},{89684392,320},{77245928,32768},{91797128,5440},{37093640,6144},{15110984,16384},{86174408,2720},{12454216,16384},{85887304,2560},{25630152,8192},{87135816,1344},{18899496,64},{54479752,64},{7290120,12288},{12733032,12288},{32969704,8192},{61628040,32768},{61660808,8064},{49120136,30720},{64594216,3584},{43748008,3584},{8156520,6144},{36245704,6144},{55529960,1024},{89819592,160},{26487176,512},{87236136,64},{33285096,32768},{87833032,5440},{65529288,1024},{90713384,160},{49536584,4096}};
typedef struct { FhStartupProgress progress; void *context; unsigned done,total,cancelled; FILE *out; unsigned char *buffer; u32 seed; } SdBench;
typedef struct { unsigned calls; u64 bytes,ticks,maximum; unsigned errors; } SdBenchResult;
static u32 sd_bench_random(SdBench *b){b->seed=b->seed*1664525u+1013904223u;return b->seed;}
static int sd_bench_step(SdBench *b,const char *phase){
    if(b->cancelled)return 0;
    if(!b->progress(b->context,phase,b->done<b->total?b->done:b->total,b->total)){b->cancelled=1;return 0;}
    b->done++;return 1;
}
static Handle sd_bench_open(const char *path){
    Handle h=0;
    if(R_FAILED(FSUSER_OpenFileDirectly(&h,ARCHIVE_SDMC,fsMakePath(PATH_EMPTY,""),fsMakePath(PATH_ASCII,path),FS_OPEN_READ,0)))return 0;
    return h;
}
static u64 sd_bench_size(Handle h){u64 size=0;return R_SUCCEEDED(FSFILE_GetSize(h,&size))?size:0;}
static void sd_bench_read(SdBenchResult *r,Handle h,u64 offset,u32 bytes,void *buffer){
    u32 got=0;u64 begin=svcGetSystemTick();Result rc=FSFILE_Read(h,&got,offset,buffer,bytes);
    u64 t=svcGetSystemTick()-begin;
    r->calls++;r->bytes+=got;r->ticks+=t;if(t>r->maximum)r->maximum=t;r->errors+=R_FAILED(rc)||got!=bytes;
}
static void sd_bench_report(SdBench *b,const char *name,const SdBenchResult *r,u64 wall){
    double ms=r->ticks*1000.0/SYSCLOCK_ARM11,wall_ms=(wall?wall:r->ticks)*1000.0/SYSCLOCK_ARM11;
    fprintf(b->out,"%s,%u,%llu,%.3f,%.3f,%.3f,%.3f,%u\n",name,r->calls,(unsigned long long)r->bytes,wall_ms,
        r->calls?ms/r->calls:0.0,r->maximum*1000.0/SYSCLOCK_ARM11,wall_ms>0?r->bytes/1048576.0/(wall_ms/1000.0):0.0,r->errors);
    fflush(b->out);
}
static void sd_bench_random_reads(SdBench *b,Handle h,u64 size,u32 bytes,unsigned calls,unsigned align,const char *name){
    SdBenchResult r={0};
    for(unsigned i=0;i<calls;i++){
        if(!sd_bench_step(b,name))return;
        u64 span=size-bytes-4096;u64 offset=((u64)sd_bench_random(b)<<16^sd_bench_random(b))%span;
        offset=align?offset&~(u64)(align-1):offset|1;
        sd_bench_read(&r,h,offset,bytes,b->buffer);
    }
    sd_bench_report(b,name,&r,0);
}
static void sd_bench_sequential(SdBench *b,Handle h,u64 start,u32 bytes,u32 total,const char *name){
    SdBenchResult r={0};
    for(u32 at=0;at<total;at+=bytes){if(!sd_bench_step(b,name))return;sd_bench_read(&r,h,start+at,bytes,b->buffer);}
    sd_bench_report(b,name,&r,0);
}
static void sd_bench_stdio(SdBench *b,const char *path,u64 size,u32 bytes,unsigned calls,const char *name){
    FILE *f=fopen(path,"rb");if(!f)return;
    SdBenchResult r={0};
    for(unsigned i=0;i<calls;i++){
        if(!sd_bench_step(b,name))break;
        u64 offset=(((u64)sd_bench_random(b)<<16^sd_bench_random(b))%(size-bytes-4096))|1;
        u64 begin=svcGetSystemTick();
        int ok=!fseek(f,(long)offset,SEEK_SET) && fread(b->buffer,1,bytes,f)==bytes;
        u64 t=svcGetSystemTick()-begin;r.calls++;r.bytes+=ok?bytes:0;r.ticks+=t;if(t>r.maximum)r.maximum=t;r.errors+=!ok;
    }
    fclose(f);sd_bench_report(b,name,&r,0);
}
typedef struct { Handle h; u64 size; u32 bytes,seed; unsigned calls; void *buffer; SdBenchResult r; } SdBenchWorker;
static void sd_bench_worker(void *arg){
    SdBenchWorker *w=arg;
    for(unsigned i=0;i<w->calls;i++){
        w->seed=w->seed*1664525u+1013904223u;u64 offset=(((u64)w->seed<<16)%(w->size-w->bytes-4096))&~511ull;
        sd_bench_read(&w->r,w->h,offset,w->bytes,w->buffer);
    }
}
static void sd_bench_parallel(SdBench *b,Handle h,u64 size,u32 bytes,unsigned calls,const char *name){
    if(!sd_bench_step(b,name))return;
    SdBenchWorker w[2]={{h,size,bytes,sd_bench_random(b),calls,b->buffer,{0}},{h,size,bytes,sd_bench_random(b),calls,b->buffer+SD_BENCH_BUFFER/2,{0}}};
    u64 begin=svcGetSystemTick();
    s32 priority=0x30;svcGetThreadPriority(&priority,CUR_THREAD_HANDLE);
    Thread t=threadCreate(sd_bench_worker,w+1,16*1024,priority,-2,false);
    sd_bench_worker(w);
    if(t){threadJoin(t,U64_MAX);threadFree(t);}else sd_bench_worker(w+1);
    SdBenchResult r={w[0].r.calls+w[1].r.calls,w[0].r.bytes+w[1].r.bytes,w[0].r.ticks+w[1].r.ticks,
        w[0].r.maximum>w[1].r.maximum?w[0].r.maximum:w[1].r.maximum,w[0].r.errors+w[1].r.errors};
    sd_bench_report(b,name,&r,svcGetSystemTick()-begin);
    b->done+=calls*2-1;
}
static int sd_bench_texture_path(char *out,size_t capacity){
    DIR *d=opendir("sdmc:/3ds/foxhollow/prepared/textures");if(!d)return 0;
    struct dirent *e;char best[64]={0};
    while((e=readdir(d)))if(e->d_name[0]=='g' && strlen(e->d_name)<sizeof(best) && strcmp(e->d_name,best)>0)strcpy(best,e->d_name);
    closedir(d);
    if(!best[0])return 0;
    snprintf(out,capacity,"/3ds/foxhollow/prepared/textures/%s/asset-1.bin",best);return 1;
}
static void sd_bench_replay(SdBench *b,Handle h,u64 size,unsigned merged,const char *name){
    SdBenchResult r={0};
    unsigned run_offset[8],run_bytes[8],runs=0;
    for(unsigned i=0;i<=SD_BENCH_PATTERN;i++){
        if(!sd_bench_step(b,name))return;
        if(i<SD_BENCH_PATTERN && sd_bench_pattern[i][0]+sd_bench_pattern[i][1]>size)continue;
        if(!merged){if(i<SD_BENCH_PATTERN)sd_bench_read(&r,h,sd_bench_pattern[i][0],sd_bench_pattern[i][1],b->buffer);continue;}
        if(i<SD_BENCH_PATTERN){
            unsigned joined=0;
            for(unsigned k=0;k<runs;k++)if(run_offset[k]+run_bytes[k]==sd_bench_pattern[i][0] && run_bytes[k]+sd_bench_pattern[i][1]<=SD_BENCH_BUFFER){
                run_bytes[k]+=sd_bench_pattern[i][1];joined=1;break;
            }
            if(joined)continue;
            if(runs<8){run_offset[runs]=sd_bench_pattern[i][0];run_bytes[runs++]=sd_bench_pattern[i][1];continue;}
        }
        for(unsigned k=0;k<runs;k++)sd_bench_read(&r,h,run_offset[k],run_bytes[k],b->buffer);
        runs=0;
        if(i<SD_BENCH_PATTERN){run_offset[0]=sd_bench_pattern[i][0];run_bytes[0]=sd_bench_pattern[i][1];runs=1;}
    }
    sd_bench_report(b,name,&r,0);
}
static int sd_bench_order(const void *a,const void *b){
    const unsigned *x=a,*y=b;return x[0]<y[0]?-1:x[0]>y[0];
}
static void sd_bench_sorted(SdBench *b,Handle h,u64 size,const char *name){
    unsigned (*order)[2]=malloc(sizeof(sd_bench_pattern));if(!order)return;
    memcpy(order,sd_bench_pattern,sizeof(sd_bench_pattern));qsort(order,SD_BENCH_PATTERN,sizeof(*order),sd_bench_order);
    SdBenchResult r={0};
    for(unsigned i=0;i<SD_BENCH_PATTERN;i++){
        if(!sd_bench_step(b,name))break;
        if(order[i][0]+order[i][1]<=size)sd_bench_read(&r,h,order[i][0],order[i][1],b->buffer);
    }
    free(order);sd_bench_report(b,name,&r,0);
}
static int sd_bench_run(void *unused,FhStartupProgress progress,void *context,char *message,size_t capacity){
    (void)unused;
    SdBench b={progress,context,0,0,0,NULL,NULL,0x2545f491u};
    b.buffer=memalign(64,SD_BENCH_BUFFER);
    if(!b.buffer){snprintf(message,capacity,"SD benchmark: no memory.");return 0;}
    Handle disc=sd_bench_open("/3ds/foxhollow/disc.iso");u64 disc_size=disc?sd_bench_size(disc):0;
    if(!disc || disc_size<256u*1024*1024){if(disc)FSFILE_Close(disc);free(b.buffer);snprintf(message,capacity,"SD benchmark: disc.iso missing.");return 0;}
    char texture_path[96];Handle texture=sd_bench_texture_path(texture_path,sizeof(texture_path))?sd_bench_open(texture_path):0;
    u64 texture_size=texture?sd_bench_size(texture):0;
    static const u32 sizes[]={4096,16384,32768,65536,131072,262144,524288,1048576};
    enum { CALLS=40 };
    b.total=8*CALLS+CALLS+CALLS+256+256+64+2*CALLS+(texture?3*(SD_BENCH_PATTERN+1):0);
    b.out=fopen("sdmc:/3ds/foxhollow/sd-bench.csv","wb");
    if(!b.out){FSFILE_Close(disc);if(texture)FSFILE_Close(texture);free(b.buffer);snprintf(message,capacity,"SD benchmark: cannot write results.");return 0;}
    fprintf(b.out,"# hardware=%s texture=%s texture_bytes=%llu pattern=%u\ntest,calls,bytes,wall_ms,ms_per_call,max_ms,mb_per_s,errors\n",
        fh_hardware_is_new?"new":"old",texture?texture_path:"none",(unsigned long long)texture_size,SD_BENCH_PATTERN);
    char name[48];
    for(unsigned i=0;i<8;i++){snprintf(name,sizeof(name),"random_aligned_%luk",(unsigned long)(sizes[i]/1024));sd_bench_random_reads(&b,disc,disc_size,sizes[i],CALLS,512,name);}
    sd_bench_random_reads(&b,disc,disc_size,32768,CALLS,0,"random_unaligned_32k");
    sd_bench_stdio(&b,"sdmc:/3ds/foxhollow/disc.iso",disc_size,32768,CALLS,"stdio_unaligned_32k");
    sd_bench_sequential(&b,disc,64ull*1024*1024,32768,8u*1024*1024,"sequential_32k");
    sd_bench_sequential(&b,disc,96ull*1024*1024,131072,32u*1024*1024,"sequential_128k");
    sd_bench_sequential(&b,disc,160ull*1024*1024,1048576,64u*1024*1024,"sequential_1m");
    sd_bench_parallel(&b,disc,disc_size,32768,CALLS,"parallel2_aligned_32k");
    if(texture){
        sd_bench_replay(&b,texture,texture_size,0,"texture_load_as_recorded");
        sd_bench_replay(&b,texture,texture_size,1,"texture_load_merged_runs");
        sd_bench_sorted(&b,texture,texture_size,"texture_load_offset_sorted");
    }
    fclose(b.out);FSFILE_Close(disc);if(texture)FSFILE_Close(texture);free(b.buffer);
    if(b.cancelled){snprintf(message,capacity,"SD benchmark cancelled. Partial results in sd-bench.csv.");return -1;}
    snprintf(message,capacity,"SD benchmark done: /3ds/foxhollow/sd-bench.csv");return 1;
}
static const FhStartupDiagnostic sd_bench_diagnostics[]={{"SD read benchmark",sd_bench_run,NULL}};
