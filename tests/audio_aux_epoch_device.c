/* Private LLE fixture: identify which published block produced AUX input.
 * No game data, IO, allocation or source-state mutation in either callback. */
#ifndef FH_EMULATOR_TEST
#error Diagnostic only
#endif
#include <3ds.h>
#include "fh_audio_dsp_aux.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
extern u16 ndspFrameId,ndspBufferId,ndspBufferCurId;
enum { ROWS=160, END=144 };
typedef struct {
    unsigned serial,tick,drops,frame,read,write,seen[2],token[2];
    int32_t minimum[2][2],maximum[2][2],first[2][2],last[2][2];
} Row;
static Row rows[ROWS];
static FhAudioDspAux aux;
static ndspWaveBuf wave;
static unsigned count,complete,overflow;
static unsigned marker(unsigned serial,unsigned lane) {
    return 1+(serial*(lane?37:1))%97;
}
static void frame(void *unused) {
    (void)unused;
    unsigned serial=++count;
    if(serial>=ROWS){overflow=1;return;}
    Row *r=&rows[serial];
    r->serial=serial;r->tick=ndspGetFrameCount();r->drops=ndspGetDroppedFrames();
    r->frame=ndspFrameId;r->read=ndspBufferId;r->write=ndspBufferCurId;
    float mix[12]={0};
    for(unsigned bus=0;bus<2;bus++)for(unsigned lane=0;lane<2;lane++)
        mix[4+4*bus+lane]=(float)marker(serial,lane)/128;
    ndspChnSetMix(2,mix);
    fh_audio_dsp_aux_tag(&aux,serial);
    if(serial==1)ndspChnWaveBufAdd(2,&wave);
#ifdef FH_AUX_GATES
    if(serial==32)ndspAuxSetEnable(1,false);
    if(serial==36)ndspAuxSetEnable(1,true);
#endif
#ifdef FH_AUX_DELAY
    if(serial==64)svcSleepThread(30000000);
#endif
}
static void process(void *context,int32_t p[4][160],uint32_t token) {
    unsigned bus=(uintptr_t)context,serial=count;
    if(!serial || serial>=ROWS){overflow=1;return;}
    Row *r=&rows[serial];r->seen[bus]++;r->token[bus]=token;
    for(unsigned lane=0;lane<2;lane++) {
        int32_t lo=p[lane][0],hi=lo;
        r->first[bus][lane]=lo;r->last[bus][lane]=p[lane][159];
        for(unsigned i=1;i<160;i++) {
            if(p[lane][i]<lo)lo=p[lane][i];
            if(p[lane][i]>hi)hi=p[lane][i];
        }
        r->minimum[bus][lane]=lo;r->maximum[bus][lane]=hi;
    }
    /* Keep returns silent; measured input is independently generated upstream. */
    memset(p,0,4*160*sizeof(int32_t));
    if(serial>=END && bus==1)__atomic_store_n(&complete,1,__ATOMIC_RELEASE);
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-aux-epoch-test",0777);
    s16 *input=linearMemAlign(320,32);if(!input)return 2;
    for(unsigned i=0;i<160;i++)input[i]=16384;
    if(R_FAILED(svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)input,320)))return 3;
    for(unsigned bus=0;bus<2;bus++){aux.tagged_process[bus]=process;aux.context[bus]=(void *)(uintptr_t)bus;}
    if(!fh_audio_dsp_aux_install(&aux) || R_FAILED(ndspInit()))return 4;
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);ndspSetClippingMode(NDSP_CLIP_NORMAL);
    ndspChnReset(2);ndspChnSetFormat(2,NDSP_FORMAT_MONO_PCM16);
    ndspChnSetRate(2,16756991.0f/512);ndspChnSetInterp(2,NDSP_INTERP_NONE);
    wave.data_pcm16=input;wave.nsamples=160;wave.looping=true;
    for(unsigned bus=0;bus<2;bus++) {
        ndspAuxSetFrontBypass(bus,false);ndspAuxSetVolume(bus,1);ndspAuxSetEnable(bus,true);
    }
    ndspSetCallback(frame,NULL);
    for(unsigned wait=0;wait<500 && !__atomic_load_n(&complete,__ATOMIC_ACQUIRE);wait++)svcSleepThread(10000000);
    ndspExit();fh_audio_dsp_aux_uninstall();
    FILE *f=fopen("sdmc:/3ds/foxhollow-aux-epoch-test/rows.csv","wb");if(!f)return 5;
    fprintf(f,"serial,tick,drops,frame,read,write,bus,seen,min0,max0,first0,last0,min1,max1,first1,last1,token\n");
    for(unsigned n=1;n<=count && n<ROWS;n++)for(unsigned bus=0;bus<2;bus++) {
        Row *r=&rows[n];
        fprintf(f,"%u,%u,%u,%u,%u,%u,%u,%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%u\n",
            r->serial,r->tick,r->drops,r->frame,r->read,r->write,bus,r->seen[bus],
            (long)r->minimum[bus][0],(long)r->maximum[bus][0],(long)r->first[bus][0],(long)r->last[bus][0],
            (long)r->minimum[bus][1],(long)r->maximum[bus][1],(long)r->first[bus][1],(long)r->last[bus][1],r->token[bus]);
    }
    fclose(f);
    f=fopen("sdmc:/3ds/foxhollow-aux-epoch-test/result.json","wb");if(!f)return 6;
    fprintf(f,"{\"complete\":%u,\"overflow\":%u,\"faults\":%u,\"rows\":%u,\"calls\":[%u,%u]}\n",
        complete,overflow,aux.faults,count,aux.calls[0],aux.calls[1]);fclose(f);
    linearFree(input);return 0;
}
