#ifndef FH_EMULATOR_TEST
#error Diagnostic only
#endif
#include <3ds.h>
#include "fh_audio_dsp_aux.h"
#include "fh_audio_aux_rate.h"
#include "musyx/musyx.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
void *salMalloc(size_t n){return calloc(1,n);}
void salFree(void *p){free(p);}
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static FhAudioDspAux aux;
static FhAudioAuxRate rate;
static _SND_REVSTD_WORK original,native;
static int32_t work[3][160];
static s16 captured[48000*2+320];
static ndspWaveBuf capture,a,b;
static unsigned started,public_calls,effect_faults;
static uint64_t cpu_ticks,max_ticks;
static FILE *logfile;
#define CHECK(x) do{if(!(x)){fprintf(logfile,"FAIL %u %s\n",__LINE__,#x);fflush(logfile);exit(2);}}while(0)
static void effect(void *context,int32_t *l,int32_t *r,int32_t *s,unsigned n) {
    (void)context;
#ifdef FH_AUX_REVERB
    if(!fh_audio_reverb_std_frames(l,r,s,n,&native))effect_faults++;
#else
    (void)l;(void)r;(void)s;(void)n;
#endif
}
static void process(void *unused,int32_t p[4][160]) {
    (void)unused;uint64_t t=svcGetSystemTick();
#ifndef FH_AUX_RAW
    if(!fh_audio_aux_rate_process(&rate,p,effect,NULL))effect_faults++;
#endif
    for(unsigned i=0;i<160;i++){p[1][i]=p[0][i];p[0][i]=0;}
    uint64_t ticks=svcGetSystemTick()-t;cpu_ticks+=ticks;if(ticks>max_ticks)max_ticks=ticks;
}
static void public_callback(void *unused,int n,void *p[4]) {
    (void)unused;(void)n;(void)p;public_calls++;
}
static void frame(void *unused) {
    (void)unused;
    if(!started) {
        ndspSetCapture(&capture);
        ndspChnWaveBufAdd(0,&a);ndspChnWaveBufAdd(2,&b);started=1;
    }
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-aux-test",0777);
    logfile=fopen("sdmc:/3ds/foxhollow-aux-test/progress.log","wb");if(!logfile)return 2;
    gfxInitDefault();
    s16 *input=linearAlloc(32000*2),*reference=linearAlloc(32000*2);CHECK(input && reference);
    CHECK(ReverbSTDCreate(&original,.6f,1.2f,.65f,.4f,.013f));
    CHECK(ReverbSTDCreate(&native,.6f,1.2f,.65f,.4f,.013f));
    for(unsigned i=0;i<32000;i++)input[i]=i<6400?(s16)(9000*sinf(2*3.141592653589793f*311*i/32000)):0;
    for(unsigned cycle=0;cycle<200;cycle++) {
        memset(work,0,sizeof(work));
        for(unsigned i=0;i<160;i++)work[0][i]=cycle?input[(cycle-1)*160+i]:0;
#ifdef FH_AUX_REVERB
        ReverbSTDCallback(work[0],work[1],work[2],&original);
#endif
        for(unsigned i=0;i<160;i++)reference[cycle*160+i]=work[0][i];
    }
    CHECK(R_SUCCEEDED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)input,64000)));
    CHECK(R_SUCCEEDED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)reference,64000)));
    aux.process[FH_AUX_BUS]=process;CHECK(fh_audio_dsp_aux_install(&aux));
    CHECK(R_SUCCEEDED(ndspInit()));ndspSetOutputMode(NDSP_OUTPUT_STEREO);ndspSetClippingMode(NDSP_CLIP_NORMAL);
    ndspAuxSetEnable(FH_AUX_BUS,true);ndspAuxSetFrontBypass(FH_AUX_BUS,false);ndspAuxSetVolume(FH_AUX_BUS,1);
    ndspAuxSetCallback(FH_AUX_BUS,public_callback,NULL);
    float mix[12]={0};mix[0]=1;ndspChnReset(0);ndspChnSetFormat(0,NDSP_FORMAT_MONO_PCM16);
    ndspChnSetRate(0,32000);ndspChnSetInterp(0,NDSP_INTERP_LINEAR);ndspChnSetMix(0,mix);
    memset(mix,0,sizeof(mix));mix[4+4*FH_AUX_BUS]=1;
    ndspChnReset(2);ndspChnSetFormat(2,NDSP_FORMAT_MONO_PCM16);ndspChnSetRate(2,32000);
    ndspChnSetInterp(2,NDSP_INTERP_LINEAR);ndspChnSetMix(2,mix);
    a.data_pcm16=reference;a.nsamples=32000;b.data_pcm16=input;b.nsamples=32000;
    capture.data_pcm16=captured;capture.nsamples=48000;memset(captured+96000,0x5a,640);
    ndspSetCallback(frame,NULL);
    for(unsigned i=0;i<120;i++)svcSleepThread(10000000);
    unsigned captured_frames=capture.offset,drops=ndspGetDroppedFrames();
    ndspExit();fh_audio_dsp_aux_uninstall();
    for(unsigned i=96000;i<96320;i++)CHECK(captured[i]==0x5a5a);
    FILE *f=fopen("sdmc:/3ds/foxhollow-aux-test/capture.pcm","wb");CHECK(f);
    CHECK(fwrite(captured,4,captured_frames,f)==captured_frames);CHECK(!fclose(f));
    f=fopen("sdmc:/3ds/foxhollow-aux-test/result.json","wb");CHECK(f);
    fprintf(f,"{\"calls\":[%u,%u],\"public_calls\":%u,\"faults\":%u,\"rate_faults\":%u,\"effect_faults\":%u,\"frames\":%u,\"drops\":%u,\"source_frames\":%llu,\"cpu_ticks\":%llu,\"max_ticks\":%llu,\"aux_bytes\":%u,\"rate_bytes\":%u}\n",
        aux.calls[0],aux.calls[1],public_calls,aux.faults,rate.faults,effect_faults,captured_frames,drops,
        (unsigned long long)rate.source_frames,(unsigned long long)cpu_ticks,(unsigned long long)max_ticks,
        (unsigned)sizeof(aux),(unsigned)sizeof(rate));CHECK(!fclose(f));
    ReverbSTDFree(&original);ReverbSTDFree(&native);linearFree(input);linearFree(reference);
    fclose(logfile);gfxExit();return 0;
}
