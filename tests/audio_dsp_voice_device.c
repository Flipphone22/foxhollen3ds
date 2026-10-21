/* Asset-free NDSP HLE fixture, deliberately forbidden in deployable builds. */
#if !defined(FH_EMULATOR_TEST) || !defined(FH_AUDIO_HLE)
#error Audio playback fixture must never ship as a console game build
#endif
#include "fh_audio_dsp_voice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
enum { CAPTURE=8000 };
static FhAudioSamplePool pool;
static FhAudioDspVoices voices;
static ndspWaveBuf capture;
static s16 recorded[CAPTURE*2+320],snapshot[CAPTURE*2];
static unsigned request,ready;
#ifdef FH_DSP_TIMED_TEST
static unsigned clock_available=1,source_cursor,first_start_cursor;
enum { START_DUE=4017 };
#endif
static const u8 component[32]={0};
static FILE *progress;
static void fail(const char *what) {fprintf(progress,"FAIL %s\n",what);fflush(progress);exit(2);}
#define REQUIRE(x) do{if(!(x))fail(#x);}while(0)
static int flush(void *unused,const void *p,size_t n) {
    (void)unused;
    Result r=svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)(uintptr_t)p,n);
    if(R_FAILED(r))r=DSP_FlushDataCache(p,n);
    return R_SUCCEEDED(r);
}
static void callback(void *unused) {
    (void)unused;
#ifdef FH_DSP_TIMED_TEST
    unsigned cursor=__atomic_load_n(&source_cursor,__ATOMIC_RELAXED);
    unsigned available=__atomic_load_n(&clock_available,__ATOMIC_ACQUIRE);
    unsigned before=voices.starts;
    fh_audio_dsp_voices_service_at(&voices,1,cursor,available);
    if(voices.starts!=before)first_start_cursor=cursor;
    if(available)__atomic_store_n(&source_cursor,cursor+160,__ATOMIC_RELEASE);
#else
    fh_audio_dsp_voices_service(&voices);
#endif
    if(__atomic_load_n(&request,__ATOMIC_ACQUIRE)) {
        unsigned offset=capture.offset;
        if(offset>=CAPTURE)return;
        memcpy(snapshot,recorded+offset*2,(CAPTURE-offset)*4);
        memcpy(snapshot+(CAPTURE-offset)*2,recorded,offset*4);
        __atomic_store_n(&request,0,__ATOMIC_RELAXED);
        __atomic_store_n(&ready,1,__ATOMIC_RELEASE);
    }
}
static void settle(unsigned frames) {for(unsigned i=0;i<frames;i++)gspWaitForVBlank();}
static void control_pair(FhAudioPoolToken id[2],FhAudioDspControls controls[2]) {
#ifdef FH_DSP_TIMED_TEST
    uint64_t due=(uint64_t)__atomic_load_n(&source_cursor,__ATOMIC_ACQUIRE)+320;
    for(unsigned i=0;i<2;i++)REQUIRE(!fh_audio_dsp_voice_control_at(&voices,i+2,id[i],&controls[i],due));
#else
    for(unsigned i=0;i<2;i++)REQUIRE(!fh_audio_dsp_voice_control(&voices,i+2,id[i],&controls[i]));
#endif
}
static void save(unsigned phase) {
    __atomic_store_n(&ready,0,__ATOMIC_RELAXED);__atomic_store_n(&request,1,__ATOMIC_RELEASE);
    unsigned i;for(i=0;i<120 && !__atomic_load_n(&ready,__ATOMIC_ACQUIRE);i++)gspWaitForVBlank();
    REQUIRE(i<120);
    char path[128];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow-dsp-voice-test/phase-%u.pcm",phase);
    FILE *f=fopen(path,"wb");REQUIRE(f);
    REQUIRE(fwrite(snapshot,1,sizeof(snapshot),f)==sizeof(snapshot));REQUIRE(!fclose(f));
    fprintf(progress,"phase %u\n",phase);fflush(progress);
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-dsp-voice-test",0777);
    progress=fopen("sdmc:/3ds/foxhollow-dsp-voice-test/progress.log","wb");if(!progress)return 2;
    gfxInitDefault();ndspUseComponent(component,sizeof(component),0xff,0xff);
    REQUIRE(R_SUCCEEDED(ndspInit()));
    void *arena=linearMemAlign(4096,32);s16 *silence=linearMemAlign(320,32);
    REQUIRE(arena && silence);REQUIRE(!fh_audio_pool_init(&pool,arena,4096));
    REQUIRE(!fh_audio_dsp_voices_init(&voices,&pool,silence,320,flush,NULL));
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);ndspSetClippingMode(NDSP_CLIP_NORMAL);
    /* Nonzero loop history and coefficients exercise independent ADPCM reset.
     * The PCM plan differs only in its FIRST prefix predictor/scale; steady
     * loops are exactly the same source samples. All sample counts are even,
     * so this does not hide Azahar's separately recorded odd-length defect. */
    const u8 encoded[16]={0x0a,0x13,0x56,0x76,0x53,0x1f,0xba,0x98,0x0a,0x13,0x56,0x76,0x53,0x1f,0xba,0x98};
    FhAudioSampleInput in={.encoded=encoded,.encoded_bytes=16,.length=28,.loop_start=14,.loop_length=14,
        .coefficients={512,-256},.initial_ps=0x0a,.loop_ps=0x0a,.loop_y1=1234,.loop_y2=-731};
    FhAudioPoolToken bank,entry,id[2];REQUIRE(!fh_audio_pool_bank_open(&pool,&bank));
    FhAudioDspControls controls[2]={{.rate=28000,.mix={.25f,0},.interpolation=NDSP_INTERP_LINEAR},
                                {.rate=28000,.mix={0,.25f},.interpolation=NDSP_INTERP_LINEAR}};
    for(unsigned i=0;i<2;i++) {
        if(i)in.initial_ps=0x09;
        REQUIRE(!fh_audio_pool_prepare(&pool,bank,i,&in,&entry));
        REQUIRE(!fh_audio_pool_publish(&pool,entry,flush,NULL));
#ifdef FH_DSP_TIMED_TEST
        REQUIRE(!fh_audio_dsp_voice_start_at(&voices,i+2,bank,i,&controls[i],1,START_DUE,&id[i]));
#else
        REQUIRE(!fh_audio_dsp_voice_start(&voices,i+2,bank,i,&controls[i],&id[i]));
#endif
    }
    capture.data_pcm16=recorded;capture.nsamples=CAPTURE;
    memset(recorded+CAPTURE*2,0xa5,640);
    ndspSetCapture(&capture);ndspSetCallback(callback,NULL);
#ifdef FH_DSP_TIMED_TEST
    /* Hold the output clock before the deadline, despite elapsed wall time. */
    __atomic_store_n(&clock_available,0,__ATOMIC_RELEASE);
    settle(40);save(5);
    __atomic_store_n(&clock_available,1,__ATOMIC_RELEASE);
#endif
    settle(60);save(0);
    for(unsigned i=0;i<2;i++)controls[i].rate=14000;
    control_pair(id,controls);
    settle(40);save(1);
    for(unsigned i=0;i<2;i++)controls[i].paused=1;
    control_pair(id,controls);
    settle(40);save(2);
    for(unsigned i=0;i<2;i++)controls[i].paused=0;
    control_pair(id,controls);
    settle(40);save(3);
#ifdef FH_DSP_TIMED_TEST
    __atomic_store_n(&clock_available,0,__ATOMIC_RELEASE);
    settle(40);save(6);
    __atomic_store_n(&clock_available,1,__ATOMIC_RELEASE);
    settle(40);save(7);
#endif
    REQUIRE(!fh_audio_pool_bank_close(&pool,bank));
#ifdef FH_DSP_TIMED_TEST
    uint64_t due=(uint64_t)__atomic_load_n(&source_cursor,__ATOMIC_ACQUIRE)+320;
    for(unsigned i=0;i<2;i++)REQUIRE(!fh_audio_dsp_voice_stop_at(&voices,i+2,id[i],due));
#else
    for(unsigned i=0;i<2;i++)REQUIRE(!fh_audio_dsp_voice_stop(&voices,i+2,id[i]));
#endif
    uint32_t completed=0,bits=0;
    for(unsigned i=0;i<120 && completed!=12;i++) {
        REQUIRE(!fh_audio_dsp_voices_poll(&voices,&bits));completed|=bits;gspWaitForVBlank();
    }
    REQUIRE(completed==12 && !pool.used);
    settle(40);save(4);
    ndspSetCallback(NULL,NULL);ndspSetCapture(NULL);ndspExit();
#ifdef FH_DSP_TIMED_TEST
    REQUIRE(first_start_cursor>=START_DUE && first_start_cursor<START_DUE+160);
    REQUIRE(voices.held==2 && !voices.epoch_rejected && !voices.cancelled);
#endif
    REQUIRE(!voices.faults && voices.starts==2 && voices.controls==6 && voices.stops==2);
    for(unsigned i=0;i<320;i++)REQUIRE((u16)recorded[CAPTURE*2+i]==0xa5a5);
    REQUIRE(!fh_audio_pool_shutdown(&pool));linearFree(arena);linearFree(silence);
    FILE *result=fopen("sdmc:/3ds/foxhollow-dsp-voice-test/result.json","wb");REQUIRE(result);
    fprintf(result,"{\"pass\":true,\"starts\":%u,\"controls\":%u,\"stops\":%u,\"retired\":%u,\"state_bytes\":%u,\"held\":%u,\"late_samples\":%llu",
        voices.starts,voices.controls,voices.stops,voices.retirement.acknowledged,(unsigned)sizeof(voices),voices.held,(unsigned long long)voices.late_samples);
#ifdef FH_DSP_TIMED_TEST
    fprintf(result,",\"first_start_cursor\":%u,\"requested_start\":%u",first_start_cursor,START_DUE);
#endif
    fprintf(result,"}\n");
    fclose(result);fclose(progress);gfxExit();return 0;
}
