/* Asset-free output-cursor integration; no synthetic clock or game assets. */
#if !defined(FH_EMULATOR_TEST) || !defined(FH_AUDIO_HLE)
#error Emulator-only fixture
#endif
#include "fh_audio_pcm_queue.h"
#include "fh_audio_dsp_voice.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
#ifndef FH_PCM_TEST_DUE
#define FH_PCM_TEST_DUE 3217
#endif
enum {CAPTURE=32000,DUE=FH_PCM_TEST_DUE,FRAMES=640,TRACE=512};
static FhAudioPcmQueue pcm;
static FhAudioSamplePool pool;
static FhAudioDspVoices voices;
static ndspWaveBuf capture;
static s16 recorded[CAPTURE*2+320],snapshot[CAPTURE*2];
static unsigned request,ready,trace_count,first_start;
static const volatile unsigned requested_due=DUE;
static struct {unsigned epoch,frame,available,starts,position,sequence;} trace[TRACE];
static const u8 component[32]={0};
static FILE *progress;
#define REQUIRE(x) do{if(!(x)){fprintf(progress,"FAIL line %u: %s\n",__LINE__,#x);fflush(progress);exit(2);}}while(0)
static int flush(void *unused,const void *p,size_t n){
    (void)unused;Result r=svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)(uintptr_t)p,n);
    if(R_FAILED(r))r=DSP_FlushDataCache(p,n);
    return R_SUCCEEDED(r);
}
static void callback(void *unused){
    (void)unused;fh_audio_pcm_service(&pcm);FhAudioPcmCursor c=fh_audio_pcm_cursor(&pcm,0);
    unsigned prior=voices.starts;fh_audio_dsp_voices_service_at(&voices,c.epoch,c.frame,c.available);
    if(voices.starts!=prior)first_start=(unsigned)c.frame;
    if(trace_count<TRACE){
        trace[trace_count].epoch=c.epoch;trace[trace_count].frame=c.frame;
        trace[trace_count].available=c.available;trace[trace_count].starts=voices.starts;
        trace[trace_count].position=ndspChnGetSamplePos(0);trace[trace_count].sequence=ndspChnGetWaveBufSeq(0);trace_count++;
    }
    if(__atomic_load_n(&request,__ATOMIC_ACQUIRE)){
        unsigned offset=capture.offset;if(offset>=CAPTURE)return;
        memcpy(snapshot,recorded+offset*2,(CAPTURE-offset)*4);
        memcpy(snapshot+(CAPTURE-offset)*2,recorded,offset*4);
        __atomic_store_n(&request,0,__ATOMIC_RELAXED);__atomic_store_n(&ready,1,__ATOMIC_RELEASE);
    }
}
int main(void){
    unsigned due=requested_due;
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-pcm-test",0777);
    progress=fopen("sdmc:/3ds/foxhollow-pcm-test/progress.log","wb");if(!progress)return 2;
    gfxInitDefault();ndspUseComponent(component,sizeof(component),0xff,0xff);REQUIRE(R_SUCCEEDED(ndspInit()));
    void *arena=linearMemAlign(4096,32);s16 *voice_silence=linearMemAlign(320,32);
    s16 *storage=linearMemAlign(131072,32),*pcm_silence=linearMemAlign(640,32);
    REQUIRE(arena && voice_silence && storage && pcm_silence);
    REQUIRE(!fh_audio_pool_init(&pool,arena,4096));
    REQUIRE(!fh_audio_dsp_voices_init(&voices,&pool,voice_silence,320,flush,NULL));
    REQUIRE(!fh_audio_pcm_init(&pcm,storage,131072,pcm_silence,640,flush,NULL));
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);ndspSetClippingMode(NDSP_CLIP_NORMAL);
    const u8 encoded[16]={0x0a,0x13,0x56,0x76,0x53,0x1f,0xba,0x98,0x0a,0x13,0x56,0x76,0x53,0x1f,0xba,0x98};
    FhAudioSampleInput in={.encoded=encoded,.encoded_bytes=16,.length=28,.loop_start=14,.loop_length=14,
        .coefficients={512,-256},.initial_ps=0x09,.loop_ps=0x0a,.loop_y1=1234,.loop_y2=-731};
    FhAudioPoolToken bank,entry,id,view;REQUIRE(!fh_audio_pool_bank_open(&pool,&bank));
    REQUIRE(!fh_audio_pool_prepare(&pool,bank,0,&in,&entry));REQUIRE(!fh_audio_pool_publish(&pool,entry,flush,NULL));
    REQUIRE(!fh_audio_pool_acquire(&pool,bank,0,&view));const void *bytes;const FhAudioDspSamplePlan *plan;
    REQUIRE(!fh_audio_pool_view(&pool,view,&bytes,&plan));REQUIRE(plan->format==FH_AUDIO_DSP_PCM16);
    FhAudioDspControls control={.rate=32000,.mix={0,.25f},.interpolation=NDSP_INTERP_LINEAR};
    REQUIRE(!fh_audio_dsp_voice_start_at(&voices,2,bank,0,&control,1,due,&id));
    capture.data_pcm16=recorded;capture.nsamples=CAPTURE;memset(recorded+CAPTURE*2,0xa5,640);
    ndspSetCapture(&capture);ndspSetCallback(callback,NULL);
    unsigned submitted=0;s16 block[FRAMES*2];u64 start=svcGetSystemTick();
    /* Producer gets deliberately different service intervals; the clock comes
     * only from played PCM and must be independent of these sleeps. */
    while(svcGetSystemTick()-start<SYSCLOCK_ARM11/2){
        while(fh_audio_pcm_space(&pcm,0)){
            for(unsigned i=0;i<FRAMES;i++){
                unsigned at=submitted+i;s16 value=0;
                if(at>=due){
                    at-=due;
                    if(at<plan->first_samples)value=((const s16 *)bytes)[at];
                    else value=((const s16 *)((const u8 *)bytes+plan->loop_offset_bytes))[(at-plan->first_samples)%plan->loop_samples];
                }
                block[i*2]=value;block[i*2+1]=0;
            }
            REQUIRE(!fh_audio_pcm_queue(&pcm,0,block,FRAMES,32000));submitted+=FRAMES;
        }
        svcSleepThread(1000000);
    }
    __atomic_store_n(&request,1,__ATOMIC_RELEASE);
    for(unsigned i=0;i<100 && !__atomic_load_n(&ready,__ATOMIC_ACQUIRE);i++)svcSleepThread(1000000);
    REQUIRE(__atomic_load_n(&ready,__ATOMIC_ACQUIRE));
    /* Reset cancels the timed voice and blocks PCM reuse until silent ack. */
    REQUIRE(!fh_audio_pcm_reset(&pcm,0));REQUIRE(!fh_audio_pool_release(&pool,view));
    REQUIRE(!fh_audio_pool_bank_close(&pool,bank));uint32_t bits=0;unsigned retired=0;
    for(unsigned i=0;i<500 && (!retired || !fh_audio_pcm_space(&pcm,0));i++){
        REQUIRE(!fh_audio_dsp_voices_poll(&voices,&bits));retired|=bits;svcSleepThread(1000000);
    }
    REQUIRE(retired==4 && fh_audio_pcm_space(&pcm,0));
    ndspSetCallback(NULL,NULL);ndspSetCapture(NULL);ndspExit();
    REQUIRE(first_start>=due && first_start<due+320 && voices.starts==1 && !voices.faults);
    REQUIRE(!pcm.channels[0].faults && !pcm.channels[0].underruns);
    for(unsigned i=0;i<320;i++)REQUIRE((u16)recorded[CAPTURE*2+i]==0xa5a5);
    FILE *f=fopen("sdmc:/3ds/foxhollow-pcm-test/output.pcm","wb");REQUIRE(f);
    REQUIRE(fwrite(snapshot,1,sizeof(snapshot),f)==sizeof(snapshot));REQUIRE(!fclose(f));
    f=fopen("sdmc:/3ds/foxhollow-pcm-test/trace.csv","wb");REQUIRE(f);
    fprintf(f,"epoch,frame,available,starts,position,sequence\n");
    for(unsigned i=0;i<trace_count;i++)fprintf(f,"%u,%u,%u,%u,%u,%u\n",trace[i].epoch,trace[i].frame,trace[i].available,trace[i].starts,trace[i].position,trace[i].sequence);
    REQUIRE(!fclose(f));REQUIRE(!fh_audio_pool_shutdown(&pool));
    linearFree(arena);linearFree(voice_silence);linearFree(storage);linearFree(pcm_silence);
    f=fopen("sdmc:/3ds/foxhollow-pcm-test/result.json","wb");REQUIRE(f);
    fprintf(f,"{\"pass\":true,\"first_start\":%u,\"due\":%u,\"retired\":%u,\"pcm_owner_bytes\":%u,\"trace_count\":%u}\n",first_start,due,retired,(unsigned)sizeof(pcm),trace_count);
    fclose(f);fclose(progress);gfxExit();return 0;
}
