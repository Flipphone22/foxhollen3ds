/* Actual original source control/mixer + real installed SDK command owner.
 * Synthetic DSP replies here do not execute DSP waveforms; the device fixture
 * separately checks audible output. No game assets. */
#include "fh_audio_native_voice.h"
#include "mixer.c"
#include <math.h>
extern unsigned char ndspChn[];
extern void ndspiUpdateChn(void),ndspiReadChnState(void);
void *ndspVars[16];u16 ndspBufferId,ndspFrameId;
static unsigned char shared[16][8192] __attribute__((aligned(32)));
static unsigned char arena[4096] __attribute__((aligned(32)));
static int16_t silence[160] __attribute__((aligned(32)));
static unsigned char encoded[16]={0x0a,0x77,0x77,0x77,0x79,0x99,0x99,0x99,0x0a,0x77,0x77,0x77,0x79,0x99,0x99,0x99};
DSPstudioinfo dspStudio[8];
static DSPvoice source_voice;DSPvoice *dspVoice=&source_voice;
u8 salMaxStudioNum=1,salNumVoices=1,salFrame,salAuxFrame;
static s32 main_pcm[2][3*SAL_PC_MAX_FRAMES],aux_a[3][3*SAL_PC_MAX_FRAMES],aux_b[3][3*SAL_PC_MAX_FRAMES];
static FhAudioSamplePool pool;
static FhAudioSampleRegistry registry;
static FhAudioDspVoices dsp;
static FhAudioNativeVoices native;
static DSPADPCMplusInfo info={.initialPS=0x0a,.loopPS=0x0a};
static DSPvoice expected[50];
volatile unsigned failure,checks,finished,resample_calls,protocol_retained,protocol_peak;
const volatile unsigned abi_sizes[]={sizeof(FhAudioDspVoices),sizeof(FhAudioNativeVoices),sizeof(FhAudioNativeVoice)};
__attribute__((noinline,noreturn)) void done(void){for(;;){}}
#define CHECK(x) do{++checks;if(!(x)){failure=__LINE__;done();}}while(0)
void LightLock_Lock(LightLock *p){(void)p;}
void LightLock_Unlock(LightLock *p){(void)p;}
u32 osConvertVirtToPhys(const void *p){return (uintptr_t)p;}
static int flush(void *p,const void *data,size_t bytes){(void)p;CHECK(data && bytes);return 1;}
u32 salPCMixRate(void){return 32000;}
u32 salPCChannels(void){return 2;}
u32 salSynthSendMessage(DSPvoice *v,u32 message){(void)v;(void)message;CHECK(0);return 0;}
void salPCConvertCycle(s32 *a,u32 b,u32 c,u64 d,const s32 *e,u32 f,u32 g,u64 h,s32 i[32]) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;CHECK(0);
}
void salPCConvertStudioCycle(s32 *a,u32 b,u32 c,u64 d,const s32 *e,u32 f,u32 g,u64 h,s32 i[2]) {
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;CHECK(0);
}
static void setup(void) {
    memset(&source_voice,0,sizeof(source_voice));memset(dspStudio,0,sizeof(dspStudio));
    memset(voiceResampler,0,sizeof(voiceResampler));memset(lastContribution,0,sizeof(lastContribution));
    memset(depop,0,sizeof(depop));memset(main_pcm,0,sizeof(main_pcm));
    memset(aux_a,0,sizeof(aux_a));memset(aux_b,0,sizeof(aux_b));memset(rearMain,0,sizeof(rearMain));
    depopCursor=0;resample_calls=0;
    DSPstudioinfo *s=&dspStudio[0];s->state=DSP_STUDIO_STATE_ACTIVE;s->isMaster=1;s->type=SND_STUDIO_TYPE_STD;
    for(unsigned i=0;i<2;i++)s->main[i]=main_pcm[i];
    for(unsigned i=0;i<3;i++){s->auxA[i]=aux_a[i];s->auxB[i]=aux_b[i];}
    s->voiceRoot=&source_voice;
    source_voice.state=DSP_VOICE_STATE_STARTING;source_voice.virtualSampleID=UINT32_MAX;
    source_voice.smp_info=(SAMPLE_INFO){.addr=encoded,.extraData=&info,.length=28,.loopLength=28,.compType=SAMPLE_TYPE_ADPCM};
    source_voice.srcTypeSelect=SAL_SRC_NONE;
    source_voice.adsr.mode=ADSR_MODE_LINEAR;source_voice.adsr.data.linear.aTime=1;
    source_voice.adsr.data.linear.dTime=1;source_voice.adsr.data.linear.sLevel=32767;
    source_voice.adsr.data.linear.rTime=100;
    source_voice.volL=16384;source_voice.volR=8192;
    for(unsigned i=0;i<5;i++){source_voice.changed[i]=8;source_voice.pitch[i]=65536;}
}
static void controls(unsigned cycle) {
    if(cycle) {
        source_voice.srcTypeSelect=cycle%3==0?SAL_SRC_NONE:SAL_SRC_LINEAR;
        source_voice.volL=(uint16_t)(cycle&1?-11000:20000);source_voice.volR=9000+cycle*30;
        for(unsigned i=0;i<5;i++) {
            source_voice.changed[i]=8;source_voice.pitch[i]=cycle%5==0?0:32768+((cycle+i)%5)*16384;
        }
    }
    salFrame=cycle&1;salAuxFrame=cycle%3;salPCBeginCycle(cycle*5);
}
static void curves(void) {
    /* Execute the actual original sample loop with unit input so its PCM
     * contribution equals the signed Q15 gain. Random gains include negatives
     * and a simultaneously moving envelope; no duplicated gain oracle. */
    uint32_t random=0x7135a963;cycleFrames=160;
    for(unsigned trial=0;trial<128;trial++) {
        FhAudioDspCurve c={.rate=32000,.frames=32,.cycle_frames=160,.offset=(trial%5)*32};
        for(unsigned ch=0;ch<9;ch++) {
            random=random*1664525u+1013904223u;c.volume_start[ch]=random>>16;
            random=random*1664525u+1013904223u;c.volume_end[ch]=random>>16;
        }
        random=random*1664525u+1013904223u;c.env_start=random>>16;
        random=random*1664525u+1013904223u;c.env_end=random>>16;
        s32 unit[32],original[9][32]={{0}};
        for(unsigned i=0;i<32;i++)unit[i]=32768;
        for(unsigned ch=0;ch<9;ch++)mixRampChannel(original[ch],unit,32,
            (s16)c.volume_start[ch],(s16)c.volume_end[ch],c.env_start,c.env_end,c.offset);
        /* Execute the source DPL2 stereo output functions, not a duplicate
         * renderer equation. Main S and B-surround are intentionally absent. */
        memset(mixBufferL,0,sizeof(mixBufferL));memset(mixBufferR,0,sizeof(mixBufferR));
        cycleFrames=32;mixStudioOutput(original[0],original[1],NULL);
        foldStereoToOutput(original[6],original[7]);cycleFrames=160;
        for(unsigned i=0;i<32;i++) {
            int32_t gain[9];CHECK(fh_audio_dsp_curve_gains(&c,i,gain));
            for(unsigned ch=0;ch<9;ch++)CHECK(gain[ch]==original[ch][i]);
            float mix[12];CHECK(fh_audio_dsp_curve_mix(&c,i,mix));
            CHECK(mix[0]==original[0][i]/32768.0f && mix[1]==original[1][i]/32768.0f);
            CHECK(mix[2]==original[2][i]*(23170.0f/(32768.0f*32768.0f)) && mix[3]==mix[2]);
            for(unsigned bus=0;bus<2;bus++)for(unsigned ch=0;ch<3;ch++)CHECK(mix[4+bus*4+ch]==original[3+bus*3+ch][i]/32768.0f);
            CHECK(mix[7]==0 && mix[11]==0);
            c.routing=FH_AUDIO_DSP_ROUTE_DPL2_STEREO;
            CHECK(fh_audio_dsp_curve_mix(&c,i,mix));
            CHECK(mix[0]==mixBufferL[i]/32768.0f && mix[1]==mixBufferR[i]/32768.0f);
            CHECK(!mix[2] && !mix[3] && !mix[8] && !mix[9] && !mix[10] && !mix[11]);
            for(unsigned ch=0;ch<3;ch++)CHECK(mix[4+ch]==original[3+ch][i]/32768.0f);
            c.routing=FH_AUDIO_DSP_ROUTE_STD;
        }
        int32_t guard[2]={123,456};CHECK(!fh_audio_dsp_curve_gain(&c,33,guard));
        CHECK(guard[0]==123 && guard[1]==456);
    }
    FhAudioPoolToken bank,token;CHECK(!fh_audio_sample_registry_find(&registry,&source_voice.smp_info,&bank));
    FhAudioDspControls initial={.rate=32000,.interpolation=NDSP_INTERP_LINEAR};
    CHECK(!fh_audio_dsp_voice_start_ramped_at(&dsp,3,bank,0,&initial,1,16000,&token));
    FhAudioDspCurve c={.rate=32000,.frames=32,.cycle_frames=160,.interpolation=NDSP_INTERP_LINEAR,
        .volume_start={16384,8192,4096,2048,512,1024,3000,4000,5000},
        .volume_end={16384,8192,4096,2048,512,1024,3000,4000,5000},.env_start=0,.env_end=16384};
    CHECK(!fh_audio_dsp_voice_curve_at(&dsp,3,token,&c,16000));
    c.offset=32;c.env_start=16384;c.env_end=32767;c.rate=16000;c.paused=1;
    CHECK(!fh_audio_dsp_voice_curve_at(&dsp,3,token,&c,16032));
    fh_audio_dsp_voices_service_window(&dsp,1,16000,16048,1);
    float mix[12];int32_t expected_gain[9];CHECK(fh_audio_dsp_curve_gains(&c,16,expected_gain));
    ndspChnGetMix(3,mix);CHECK(mix[0]==expected_gain[0]/32768.0f && mix[1]==expected_gain[1]/32768.0f);
    CHECK(mix[2]==expected_gain[2]*(23170.0f/(32768.0f*32768.0f)) && mix[3]==mix[2]);
    for(unsigned bus=0;bus<2;bus++)for(unsigned ch=0;ch<3;ch++)CHECK(mix[4+bus*4+ch]==expected_gain[3+bus*3+ch]/32768.0f);
    CHECK(mix[7]==0 && mix[11]==0);
    CHECK(!ndspChnIsPaused(3));CHECK(fabsf(ndspChnGetRate(3)-32000/(float)NDSP_SAMPLE_RATE)<.000001f);
    CHECK(dsp.voices[1].head-dsp.voices[1].tail==1); /* lookahead is not consumption */
    fh_audio_dsp_voices_service_window(&dsp,1,16032,16064,1);CHECK(ndspChnIsPaused(3));
    CHECK(!fh_audio_dsp_voice_stop_at(&dsp,3,token,16064));
    fh_audio_dsp_voices_service_window(&dsp,1,16064,16100,1);CHECK(!dsp.voices[1].live);
}
#ifdef FH_AUX_SOURCE_TEST
static void source_aux(u8 reason,SND_AUX_INFO *info,void *context) {
    CHECK(reason==SND_AUX_REASON_BUFFERUPDATE && info && context);
}
static unsigned aux_context[2];
static void aux_setup(void) {
    setup();dspStudio[0].auxAHandler=source_aux;dspStudio[0].auxAUser=&aux_context[0];
#ifdef FH_DPL2_SOURCE_TEST
    dspStudio[0].type=SND_STUDIO_TYPE_DPL2;
#endif
#ifndef FH_AUX_NULL_B
    dspStudio[0].auxBHandler=source_aux;dspStudio[0].auxBUser=&aux_context[1];
#endif
}
static void aux_controls(unsigned cycle) {
    controls(cycle);source_voice.volS=4000+cycle*17;
    source_voice.volLa=6000+cycle*19;source_voice.volRa=3000+cycle*23;source_voice.volSa=2000+cycle*29;
    source_voice.volLb=9000+cycle*31;source_voice.volRb=5000+cycle*37;source_voice.volSb=1000+cycle*41;
}
static void aux_test(void) {
    aux_setup();fh_audio_source_bridge=&native.bridge;
    CHECK(fh_audio_source_begin(0,&source_voice,0,32000));CHECK(!native.bridge.voices[0].token);
    CHECK(!fh_audio_native_voices_bind_aux(&native,8));
    CHECK(fh_audio_native_voices_bind_aux(&native,0));
    fh_audio_source_bridge=NULL;
    for(unsigned cycle=0;cycle<50;cycle++) {
        aux_controls(cycle);s16 output[320];salCtrlDsp(output);expected[cycle]=source_voice;
    }
    CHECK(resample_calls);aux_setup();fh_audio_source_bridge=&native.bridge;
    for(unsigned cycle=0;cycle<50;cycle++) {
        CHECK(fh_audio_native_voices_available(&native));aux_controls(cycle);
        s16 output[320];salCtrlDsp(output);CHECK(!memcmp(&source_voice,&expected[cycle],sizeof(source_voice)));
        CHECK(!resample_calls && !native.faults && !native.bridge.faults);
        for(unsigned i=0;i<320;i++)CHECK(!output[i]);
        FhAudioDspVoice *v=&dsp.voices[0];const FhAudioDspCurve *c=&v->commands[(v->head-1)%FH_AUDIO_DSP_COMMANDS].curve;
        CHECK(c->volume_end[2]==source_voice.volS && c->volume_end[3]==source_voice.volLa &&
              c->volume_end[4]==source_voice.volRa && c->volume_end[5]==source_voice.volSa);
#if defined(FH_AUX_NULL_B) && !defined(FH_DPL2_SOURCE_TEST)
        CHECK(!c->volume_start[6] && !c->volume_start[7] && !c->volume_start[8]);
        CHECK(!c->volume_end[6] && !c->volume_end[7] && !c->volume_end[8]);
#else
        CHECK(c->volume_end[6]==source_voice.volLb && c->volume_end[7]==source_voice.volRb && c->volume_end[8]==source_voice.volSb);
#endif
    }
    /* All250 source segments still execute; only identical flat controls may
     * share a queued command. The source-state and SDK output checks surround
     * this accounting check, so fewer commands alone cannot establish parity. */
    CHECK(native.admitted==1 && native.segments==250 &&
          native.peak_pending+dsp.retained_controls==251 && dsp.retained_controls>0);
    protocol_retained=dsp.retained_controls;protocol_peak=native.peak_pending;
    CHECK(!fh_audio_native_voices_bind_aux(&native,0));
    for(unsigned cycle=0;cycle<50;cycle++) {
        fh_audio_dsp_voices_service_window(&dsp,1,cycle*160+128,cycle*160+159,1);ndspiUpdateChn();
        float mix[12];ndspChnGetMix(2,mix);
#ifdef FH_DPL2_SOURCE_TEST
        CHECK(!mix[2] && !mix[3] && !mix[8] && !mix[9] && !mix[10]);
        CHECK(dsp.voices[0].curve.routing==FH_AUDIO_DSP_ROUTE_DPL2_STEREO);
#else
        CHECK(mix[4]!=0 && mix[5]!=0 && mix[6]!=0 && mix[2]!=0 && mix[3]==mix[2]);
#ifdef FH_AUX_NULL_B
        CHECK(!mix[8] && !mix[9] && !mix[10]);
#else
        CHECK(mix[8]!=0 && mix[9]!=0 && mix[10]!=0);
#endif
#endif
        CHECK(!mix[7] && !mix[11]);
    }
    FhAudioSourceVoice prior=native.bridge.voices[0];
    FhAudioSourceSegment seg={.source_frame=8000,.pitch=65536,.frames=32,.cycle_frames=160,.env_end=32767};
    dspStudio[0].auxAUser=&aux_context[1];CHECK(fh_audio_source_segment(0,&source_voice,&seg)==-2);
    CHECK(!memcmp(&prior,&native.bridge.voices[0],sizeof(prior)));dspStudio[0].auxAUser=&aux_context[0];
    dspStudio[0].auxAHandler=NULL;CHECK(fh_audio_source_segment(0,&source_voice,&seg)==-2);dspStudio[0].auxAHandler=source_aux;
    native.epoch=2;CHECK(fh_audio_source_segment(0,&source_voice,&seg)==-2);native.epoch=1;
    source_voice.volRear[0]=1;CHECK(fh_audio_source_segment(0,&source_voice,&seg)==-2);source_voice.volRear[0]=0;
    source_voice.flags|=0x80000000u;CHECK(fh_audio_source_segment(0,&source_voice,&seg)==-2);source_voice.flags&=~0x80000000u;
    CHECK(!memcmp(&prior,&native.bridge.voices[0],sizeof(prior)));
    CHECK(fh_audio_source_stop(0,8000));fh_audio_dsp_voices_service_at(&dsp,1,8000,1);CHECK(dsp.stops==1);
    finished=1;done();
}
#endif
void run(void) {
    protocol_retained=protocol_peak=0;
    CHECK(abi_sizes[0]==sizeof(dsp));
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    CHECK(!fh_audio_pool_init(&pool,arena,sizeof(arena)));
    CHECK(!fh_audio_sample_registry_init(&registry,&pool,flush,NULL));
    CHECK(!fh_audio_dsp_voices_init(&dsp,&pool,silence,sizeof(silence),flush,NULL));
    CHECK(fh_audio_native_voices_init(&native,&dsp,&registry,1,
#ifdef FH_DPL2_SOURCE_TEST
        4
#elif defined(FH_AUX_SOURCE_TEST)
        3
#else
        1
#endif
    ));
    setup();CHECK(!fh_audio_sample_registry_prepare(&registry,&info,&source_voice.smp_info));
#ifdef FH_AUX_SOURCE_TEST
    aux_test();
#endif
    /* The default-disabled backend must not consume a channel. */
    native.experimental_direct_stereo=0;fh_audio_source_bridge=&native.bridge;
    CHECK(fh_audio_source_begin(0,&source_voice,0,32000));CHECK(!native.bridge.voices[0].token && !dsp.voices[0].lease);
    native.experimental_direct_stereo=1;
    fh_audio_source_bridge=NULL;unsigned nonzero=0;
    for(unsigned cycle=0;cycle<50;cycle++) {
        controls(cycle);s16 output[320];salCtrlDsp(output);expected[cycle]=source_voice;
        for(unsigned i=0;i<320;i++)nonzero+=output[i]!=0;
    }
    CHECK(nonzero && resample_calls);
    setup();fh_audio_source_bridge=&native.bridge;
    for(unsigned cycle=0;cycle<50;cycle++) {
        CHECK(fh_audio_native_voices_available(&native));
        controls(cycle);s16 output[320];salCtrlDsp(output);
        CHECK(!memcmp(&expected[cycle],&source_voice,sizeof(source_voice)));
        CHECK(!resample_calls && !native.faults && !native.bridge.faults);
        for(unsigned i=0;i<320;i++)CHECK(!output[i]);
    }
    CHECK(native.admitted==1 && native.segments==250 && native.peak_pending==251);
    CHECK(!fh_audio_native_voices_available(&native));
    /* Original source position/ADSR are preserved with NO CPU resampler calls;
     * SDK submission only starts when this source timeline is played. */
    fh_audio_dsp_voices_service_at(&dsp,1,0,0);CHECK(!dsp.starts);
    FhAudioDspVoice *v=&dsp.voices[0];
    for(unsigned cycle=0;cycle<50;cycle++) {
        unsigned count=0;FhAudioDspControls last={0};
        for(unsigned i=v->tail;i<v->head;i++) {
            const FhAudioDspCommand *c=&v->commands[i%FH_AUDIO_DSP_COMMANDS];
            if(c->due>cycle*160+128)break;
            last=c->controls;count++;
        }
        CHECK(count>=5);fh_audio_dsp_voices_service_at(&dsp,1,cycle*160+128,1);
        float mix[12];ndspChnGetMix(2,mix);
        CHECK(!memcmp(mix,last.mix,sizeof(mix)) && ndspChnIsPaused(2)==(last.paused!=0));
        CHECK(fabsf(ndspChnGetRate(2)-last.rate/(float)NDSP_SAMPLE_RATE)<0.000001f);
        if(cycle%3==0)CHECK(last.rate==32000 && last.interpolation==NDSP_INTERP_LINEAR);
        ndspiUpdateChn();
    }
    CHECK(dsp.starts==1 && dsp.controls==250 && !dsp.faults);
    CHECK(fh_audio_native_voices_available(&native));
    /* Unsupported route after admission is explicit and preserves source state. */
    FhAudioSourceVoice prior=native.bridge.voices[0];DSPvoice old=source_voice;
    source_voice.volLa=123;FhAudioSourceSegment segment={.source_frame=8000,.pitch=65536,.frames=32,.cycle_frames=160,.env_end=32767};
    CHECK(fh_audio_source_segment(0,&source_voice,&segment)==-2);
    CHECK(!memcmp(&prior,&native.bridge.voices[0],sizeof(prior)));
    source_voice=old;CHECK(fh_audio_source_stop(0,8000));
    fh_audio_dsp_voices_service_at(&dsp,1,7999,1);CHECK(!dsp.stops);
    fh_audio_dsp_voices_service_at(&dsp,1,8000,1);CHECK(dsp.stops==1 && v->lease);
    curves();
    /* Source stop is not a retirement fence. Source registry can close safely. */
    FhAudioPoolToken lease=v->lease;CHECK(!fh_audio_sample_registry_forget(&registry,&info));
    const void *samples;const FhAudioDspSamplePlan *plan;
    CHECK(!fh_audio_pool_view(&pool,lease,&samples,&plan));
    CHECK(fh_audio_pool_shutdown(&pool)==FH_AUDIO_POOL_BUSY);
    finished=1;done();
}
