#if !defined(FH_EMULATOR_TEST)
#error Native voice integration fixture must not ship as a game build
#endif
#include "fh_audio_native_voice.h"
#include "fh_audio_pcm_queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
u32 __ctru_heap_size=4*1024*1024,__ctru_linear_heap_size=2*1024*1024;
static FILE *progress;
#define REQUIRE(x) do{if(!(x)){fprintf(progress,"FAIL %u %s\n",__LINE__,#x);fflush(progress);exit(2);}}while(0)
#define CHECK REQUIRE
#include "source-fixture.h"
#ifdef FH_NATIVE_AUX_TEST
#include "fh_audio_dsp_aux.h"
#include "fh_audio_aux_rate.h"
static FhAudioDspAux aux_owner;
static FhAudioAuxRate aux_rate;
static _SND_REVSTD_WORK source_reverb,native_reverb;
static unsigned aux_faults;
void *salMalloc(size_t bytes){return calloc(1,bytes);}
void salFree(void *p){free(p);}
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static void native_effect(void *unused,int32_t *l,int32_t *r,int32_t *s,unsigned frames) {
    (void)unused;
    if(!fh_audio_reverb_std_frames(l,r,s,frames,&native_reverb))aux_faults++;
#ifdef FH_NATIVE_DPL2_TEST
    memset(s,0,frames*sizeof(*s));
#endif
}
static void aux_process(void *unused,int32_t samples[4][160]) {
    (void)unused;if(!fh_audio_aux_rate_process(&aux_rate,samples,native_effect,NULL))aux_faults++;
}
static void source_effect(u8 reason,SND_AUX_INFO *info,void *context) {
    REQUIRE(reason==SND_AUX_REASON_BUFFERUPDATE);
    ReverbSTDCallback(info->data.bufferUpdate.left,info->data.bufferUpdate.right,
                     info->data.bufferUpdate.surround,context);
}
#endif
static FhAudioPcmQueue pcm;
#ifdef FH_NATIVE_AUX_TEST
enum { CAPTURE=64000 };
#else
enum { CAPTURE=32000 };
#endif
static s16 recorded[CAPTURE*2+320],snapshot[CAPTURE*2];
static ndspWaveBuf capture;
static unsigned request,ready;
#ifdef FH_AUDIO_HLE
static const u8 component[32]={0};
#endif
static unsigned cpu_calls[2],blocks[2],underruns[2];
static unsigned messages[2];
static unsigned starvations[2];
#ifdef FH_NATIVE_PAIRED_TEST
enum { REFERENCE_FRAMES=CAPTURE };
static s16 reference_pcm[REFERENCE_FRAMES];
#endif
#if defined(FH_NATIVE_CURVE_TEST) && !defined(FH_NATIVE_WINDOW_TEST)
static uint64_t window_samples;
static unsigned window_started;
#endif
static int flush(void *unused,const void *p,size_t bytes) {
    (void)unused;Result result=svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)(uintptr_t)p,bytes);
    if(R_FAILED(result))result=DSP_FlushDataCache(p,bytes);
    return R_SUCCEEDED(result);
}
static void callback(void *unused) {
    (void)unused;fh_audio_pcm_service(&pcm);
    FhAudioPcmCursor c=fh_audio_pcm_cursor(&pcm,0);
    /* Directly rendered fixture PCM has no DMA/movie delay. The real game's
     * provenance adapter is a separate path, not covered by this fixture. */
#ifdef FH_NATIVE_WINDOW_TEST
    FhAudioSourceWindow w=fh_audio_pcm_source_window(&pcm,0);
#ifdef FH_NATIVE_SUBBLOCK_TEST
    const FhAudioSpan *known=NULL;
    if(w.valid)for(unsigned i=0;i<w.count;i++)if(w.spans[i].generation) {
        REQUIRE(!known && (w.spans[i].generation==11 || w.spans[i].generation==17));known=&w.spans[i];
    }
    if(known) {
        REQUIRE(known->offset+known->frames==w.frames);
        fh_audio_dsp_voices_service_region(&dsp,known->generation,known->source_frame,
            known->source_frame+known->frames,known->offset,1);
    } else fh_audio_dsp_voices_service_region(&dsp,17,0,0,0,0);
#else
    if(w.valid && w.count==1 && w.spans[0].generation) {
        FhAudioSpan s=w.spans[0];
        fh_audio_dsp_voices_service_window(&dsp,s.generation,s.source_frame,s.source_frame+s.frames,1);
    } else fh_audio_dsp_voices_service_window(&dsp,17,0,0,0);
#endif
#elif defined(FH_NATIVE_CURVE_TEST)
    /* Diagnostic paired-start clock only. Counts DSP output blocks; no wall
     * clock/frame-rate multiplier. Game reset/underrun mapping remains open. */
    if(c.available) {
        if(!window_started){window_samples=c.frame*SYSCLOCK_SOC;window_started=1;}
        uint64_t first=window_samples/SYSCLOCK_SOC;
        window_samples+=UINT64_C(160)*32000*512;
        fh_audio_dsp_voices_service_window(&dsp,c.epoch,first,window_samples/SYSCLOCK_SOC,1);
    } else {
        window_started=0;
        fh_audio_dsp_voices_service_window(&dsp,c.epoch,c.frame,c.frame,0);
    }
#else
    fh_audio_dsp_voices_service_at(&dsp,c.epoch,c.frame,c.available);
#endif
    if(__atomic_load_n(&request,__ATOMIC_ACQUIRE)) {
        unsigned offset=capture.offset;if(offset>=CAPTURE)return;
        memcpy(snapshot,recorded+offset*2,(CAPTURE-offset)*4);
        memcpy(snapshot+(CAPTURE-offset)*2,recorded,offset*4);
        __atomic_store_n(&request,0,__ATOMIC_RELAXED);__atomic_store_n(&ready,1,__ATOMIC_RELEASE);
    }
}
static void save(unsigned phase) {
    __atomic_store_n(&ready,0,__ATOMIC_RELAXED);__atomic_store_n(&request,1,__ATOMIC_RELEASE);
    for(unsigned i=0;i<100 && !__atomic_load_n(&ready,__ATOMIC_ACQUIRE);i++)svcSleepThread(1000000);
    REQUIRE(__atomic_load_n(&ready,__ATOMIC_ACQUIRE));
    char path[128];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow-native-voice-test/phase-%u.pcm",phase);
    FILE *f=fopen(path,"wb");REQUIRE(f);REQUIRE(fwrite(snapshot,1,sizeof(snapshot),f)==sizeof(snapshot));REQUIRE(!fclose(f));
}
int main(void) {
    mkdir("sdmc:/3ds",0777);mkdir("sdmc:/3ds/foxhollow-native-voice-test",0777);
    progress=fopen("sdmc:/3ds/foxhollow-native-voice-test/progress.log","wb");if(!progress)return 2;
    gfxInitDefault();
#ifdef FH_NATIVE_AUX_TEST
    REQUIRE(ReverbSTDCreate(&native_reverb,.6f,1.2f,.65f,.4f,.013f));
    aux_owner.process[0]=aux_process;REQUIRE(fh_audio_dsp_aux_install(&aux_owner));
#endif
#ifdef FH_AUDIO_HLE
    ndspUseComponent(component,sizeof(component),0xff,0xff);
#endif
    REQUIRE(R_SUCCEEDED(ndspInit()));
#ifdef FH_NATIVE_AUX_TEST
    ndspAuxSetEnable(0,true);ndspAuxSetFrontBypass(0,false);ndspAuxSetVolume(0,1);
#endif
    void *arena=linearMemAlign(4096,32);s16 *voice_silence=linearMemAlign(2048,32);
    s16 *storage=linearMemAlign(131072,32),*pcm_silence=linearMemAlign(640,32);
    REQUIRE(arena && voice_silence && storage && pcm_silence);
    REQUIRE(!fh_audio_pool_init(&pool,arena,4096));
    REQUIRE(!fh_audio_sample_registry_init(&registry,&pool,flush,NULL));
    REQUIRE(!fh_audio_dsp_voices_init(&dsp,&pool,voice_silence,2048,flush,NULL));
    REQUIRE(!fh_audio_pcm_init(&pcm,storage,131072,pcm_silence,640,flush,NULL));
#ifdef FH_NATIVE_WINDOW_TEST
    pcm.channels[0].output_guard=1;
#endif
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);ndspSetClippingMode(NDSP_CLIP_NORMAL);
    setup();REQUIRE(!fh_audio_sample_registry_prepare(&registry,&info,&source_voice.smp_info));
    capture.data_pcm16=recorded;capture.nsamples=CAPTURE;memset(recorded+CAPTURE*2,0xa5,640);
    ndspSetCapture(&capture);ndspSetCallback(callback,NULL);
    for(unsigned phase=0;phase<2;phase++) {
        setup();unsigned cycle=0;
#ifdef FH_NATIVE_DPL2_TEST
        dspStudio[0].type=SND_STUDIO_TYPE_DPL2;
        source_voice.volS=10000;source_voice.volSa=8192;
        if(phase)source_voice.volRb=4096;else source_voice.volLb=4096;
#endif
#ifdef FH_NATIVE_AUX_TEST
        REQUIRE(ReverbSTDCreate(&source_reverb,.6f,1.2f,.65f,.4f,.013f));
        dspStudio[0].auxAHandler=source_effect;dspStudio[0].auxAUser=&source_reverb;
        if(phase)source_voice.volRa=12288;else source_voice.volLa=12288;
#endif
#ifdef FH_NATIVE_ENVELOPE_TEST
        source_voice.adsr.data.linear.aTime=40;source_voice.adsr.data.linear.dTime=20;
        source_voice.adsr.data.linear.sLevel=16384;source_voice.adsr.data.linear.rTime=20;
#endif
        if(phase) {
#ifdef FH_NATIVE_PAIRED_TEST
            /* Left is independently generated original CPU reference; right is
             * native DSP. Both are submitted together on the same output clock. */
            source_voice.volR=source_voice.volL;source_voice.volL=0;
#endif
            REQUIRE(fh_audio_native_voices_init(&native,&dsp,&registry,
#ifdef FH_NATIVE_WINDOW_TEST
                17,
#else
                pcm.channels[0].requested_epoch,
#endif
#ifdef FH_NATIVE_DPL2_TEST
                4
#elif defined(FH_NATIVE_AUX_TEST)
                3
#elif defined(FH_NATIVE_CURVE_TEST)
                2
#else
                1
#endif
            ));
#ifdef FH_NATIVE_AUX_TEST
            REQUIRE(fh_audio_native_voices_bind_aux(&native,0));
#endif
            fh_audio_source_bridge=&native.bridge;
        } else fh_audio_source_bridge=NULL;
        u64 start=svcGetSystemTick();s16 packet[640*2];
        while(svcGetSystemTick()-start<SYSCLOCK_ARM11*
#ifdef FH_NATIVE_AUX_TEST
            7
#else
            3
#endif
            /4) {
#ifdef FH_NATIVE_PAUSE_TEST
            u64 elapsed=svcGetSystemTick()-start;
            REQUIRE(!fh_audio_pcm_pause(&pcm,0,phase && elapsed>SYSCLOCK_ARM11/5 && elapsed<SYSCLOCK_ARM11/4));
#endif
            while(fh_audio_pcm_space(&pcm,0)) {
#ifdef FH_NATIVE_STARVE_TEST
                /* Real lack of producer data, not a caller-issued pause. The
                 * paired reference and DSP tone must hold/resume together. */
                u64 withheld=svcGetSystemTick()-start;
                if(phase && withheld>SYSCLOCK_ARM11/10 && withheld<SYSCLOCK_ARM11*9/25)break;
#endif
                for(unsigned slice=0;slice<4;slice++,cycle++) {
                    if(phase)REQUIRE(fh_audio_native_voices_available(&native));
#ifdef FH_NATIVE_ENVELOPE_TEST
                    source_voice.changed[0]=cycle==80?0x40:8;
#endif
                    salFrame=cycle&1;salAuxFrame=cycle%3;salPCBeginCycle(cycle*5);
                    salCtrlDsp(packet+slice*320);
                }
                if(phase)for(unsigned i=0;i<1280;i++)REQUIRE(!packet[i]);
#ifdef FH_NATIVE_PAIRED_TEST
                unsigned first=(cycle-4)*160;REQUIRE(first+640<=REFERENCE_FRAMES);
                for(unsigned i=0;i<640;i++) {
                    if(!phase)reference_pcm[first+i]=packet[i*2];
                    else {
#ifdef FH_NATIVE_SUBBLOCK_TEST
                        packet[i*2]=first+i<FH_NATIVE_SUBBLOCK_TEST?0:reference_pcm[first+i-FH_NATIVE_SUBBLOCK_TEST];
#else
                        packet[i*2]=reference_pcm[first+i];
#endif
                        packet[i*2+1]=0;
                    }
                }
#endif
#ifdef FH_NATIVE_WINDOW_TEST
                FhAudioMap map={0};
#ifdef FH_NATIVE_SUBBLOCK_TEST
                uint64_t origin=(uint64_t)(cycle-4)*160;
                unsigned prefix=phase && origin<FH_NATIVE_SUBBLOCK_TEST?
                    (FH_NATIVE_SUBBLOCK_TEST-origin>640?640:(unsigned)(FH_NATIVE_SUBBLOCK_TEST-origin)):0;
                if(prefix)REQUIRE(fh_audio_map_append(&map,(FhAudioSpan){0},0,prefix));
                if(prefix<640)REQUIRE(fh_audio_map_append(&map,(FhAudioSpan){
                    .source_frame=origin+prefix-(phase?FH_NATIVE_SUBBLOCK_TEST:0),
                    .generation=phase?17:11,.frames=640-prefix},prefix,640-prefix));
#else
                REQUIRE(fh_audio_map_append(&map,(FhAudioSpan){.source_frame=(uint64_t)(cycle-4)*160,
                    .generation=phase?17:11,.frames=640},0,640));
#endif
                REQUIRE(!fh_audio_pcm_queue_mapped(&pcm,0,packet,640,32000,&map));
#else
                REQUIRE(!fh_audio_pcm_queue(&pcm,0,packet,640,32000));
#endif
            }
            if(phase)REQUIRE(!fh_audio_native_voices_poll(&native));
            svcSleepThread(1000000);
        }
        cpu_calls[phase]=resample_calls;blocks[phase]=cycle;
        messages[phase]=source_messages;
        underruns[phase]=fh_audio_pcm_status(&pcm,0).underruns;
#ifdef FH_NATIVE_WINDOW_TEST
        starvations[phase]=pcm.channels[0].output_clock.starvations;
#endif
        save(phase);
        if(phase)REQUIRE(fh_audio_source_stop(0,(uint64_t)cycle*160));
        REQUIRE(!fh_audio_pcm_reset(&pcm,0));
        for(unsigned i=0;i<500;i++) {
            if(phase)REQUIRE(!fh_audio_native_voices_poll(&native));
            if(fh_audio_pcm_space(&pcm,0) && (!phase || !dsp.voices[0].lease))break;
            svcSleepThread(1000000);
        }
        REQUIRE(fh_audio_pcm_space(&pcm,0) && (!phase || !dsp.voices[0].lease));
        fh_audio_source_bridge=NULL;
#ifdef FH_NATIVE_AUX_TEST
        ReverbSTDFree(&source_reverb);
#endif
        /* Separate capture windows without modifying the DSP cursor. */
        for(unsigned i=0;i<70;i++)gspWaitForVBlank();
    }
    ndspSetCallback(NULL,NULL);ndspSetCapture(NULL);ndspExit();
#ifdef FH_NATIVE_STARVE_TEST
    REQUIRE(starvations[1]>starvations[0] && !underruns[1]);
#endif
#ifdef FH_NATIVE_AUX_TEST
    fh_audio_dsp_aux_uninstall();ReverbSTDFree(&native_reverb);
    REQUIRE(aux_owner.calls[0]>200 && !aux_faults && !aux_owner.faults && !aux_rate.faults);
#endif
    REQUIRE(cpu_calls[0]>0 && !cpu_calls[1] && dsp.starts==1 && native.admitted==1 && !native.faults && !dsp.faults);
    REQUIRE(!fh_audio_sample_registry_reset(&registry));REQUIRE(!fh_audio_pool_shutdown(&pool));
    for(unsigned i=0;i<320;i++)REQUIRE((u16)recorded[CAPTURE*2+i]==0xa5a5);
    linearFree(arena);linearFree(voice_silence);linearFree(storage);linearFree(pcm_silence);
    FILE *f=fopen("sdmc:/3ds/foxhollow-native-voice-test/result.json","wb");REQUIRE(f);
    fprintf(f,"{\"pass\":true,\"cpu_resample_calls\":[%u,%u],\"cycles\":[%u,%u],\"underruns\":[%u,%u],\"starts\":%u,\"source_segments\":%u,\"peak_pending\":%u,\"retained_controls\":%u,\"source_end_messages\":[%u,%u],\"prefix_starts\":%u,\"prefix_samples\":%u,\"prefix_faults\":%u,\"aux_calls\":%u}\n",
        cpu_calls[0],cpu_calls[1],blocks[0],blocks[1],underruns[0],underruns[1],dsp.starts,native.segments,native.peak_pending,dsp.retained_controls,messages[0],messages[1],dsp.prefix_starts,dsp.prefix_samples,dsp.prefix_faults,
#ifdef FH_NATIVE_AUX_TEST
        aux_owner.calls[0]
#else
        0u
#endif
    );
    fprintf(progress,"starvations=%u,%u\n",starvations[0],starvations[1]);
    fclose(f);fclose(progress);gfxExit();return 0;
}
