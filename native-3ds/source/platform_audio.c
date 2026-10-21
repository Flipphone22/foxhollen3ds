#include <3ds.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fh_platform.h"
#include "fh_audio_worker.h"
#include "fh_diagnostic_buffer.h"
#include "fh_perf_display.h"
#include "fh_audio_pcm_queue.h"
#include "fh_audio_game_dsp.h"

enum { CHANNELS=2, BLOCKS=8, MAX_FRAMES=2048 };
static FhAudioPcmQueue pcm_queue;
static unsigned pending_paused[CHANNELS];
static s16 *storage,*reset_silence;
static int initialized,direct_flush_failed;
static FILE *metrics;
static RecursiveLock io_lock;
static unsigned io_lock_ready;
static void io_enter(void) {
    /* Initialized on main before the optional worker is created. */
    if(!io_lock_ready){RecursiveLock_Init(&io_lock);io_lock_ready=1;}
    RecursiveLock_Lock(&io_lock);
}
static void io_leave(void) {RecursiveLock_Unlock(&io_lock);}
int fh_audio_display_snapshot(unsigned milliseconds[2],unsigned underruns[2]){
    if(!io_lock_ready || RecursiveLock_TryLock(&io_lock)!=0)return 0;
    if(!initialized){io_leave();return 0;}
    for(unsigned c=0;c<CHANNELS;c++){
        FhAudioPcmStatus status=fh_audio_pcm_status(&pcm_queue,c);
        milliseconds[c]=status.rate?(uint64_t)status.frames*1000/status.rate:0;
        underruns[c]=status.underruns;
    }
    io_leave();return 1;
}
#ifdef FH_EMULATOR_TEST
enum { CAPTURE_FRAMES=32000, CAPTURE_GUARD=160 };
_Static_assert(CAPTURE_FRAMES%160==0,"NDSP capture block alignment");
static ndspWaveBuf capture;
static s16 *capture_samples,*capture_snapshot;
static volatile unsigned capture_request,capture_ready;
static unsigned capture_retrace;
static void capture_callback(void *unused) {
    (void)unused;if(!capture_request)return;
    unsigned offset=capture.offset;
    if(offset>=32000)return;
    memcpy(capture_snapshot,capture_samples+offset*2,(32000-offset)*4);
    memcpy(capture_snapshot+(32000-offset)*2,capture_samples,offset*4);
    __sync_synchronize();capture_ready=1;capture_request=0;
}
#endif
#ifdef FH_AUDIO_HLE
#ifndef FH_EMULATOR_TEST
#error HLE audio fixture must never be present in a console build
#endif
static const u8 hle_component[32]={0};
#endif
static int flush_pcm(void *unused,const void *memory,size_t bytes) {
    (void)unused;Result result=-1;
    if(!direct_flush_failed) {
        result=svcFlushProcessDataCache(CUR_PROCESS_HANDLE,(u32)(uintptr_t)memory,bytes);
        if(R_FAILED(result)){direct_flush_failed=1;fh_platform_log("Audio cache flush using DSP service fallback\n");}
    }
    if(direct_flush_failed)result=DSP_FlushDataCache(memory,bytes);
    return R_SUCCEEDED(result);
}
static void audio_callback(void *unused) {
    (void)unused;fh_audio_pcm_service(&pcm_queue);
    fh_audio_game_dsp_service(&pcm_queue);
#ifdef FH_EMULATOR_TEST
    if(capture_samples)capture_callback(NULL);
#endif
}
static void initialize(void) {
    if(initialized) return;
#ifdef FH_AUDIO_HLE
    ndspUseComponent(hle_component,sizeof(hle_component),0xff,0xff);
    fh_platform_log("Emulator-only HLE audio component active\n");
#endif
    if(!fh_audio_game_dsp_preinit())fh_platform_panic("Native DSP hook installation failed");
    Result result=ndspInit();
    if(R_FAILED(result)) {
        char message[128];snprintf(message,sizeof(message),"NDSP initialization failed (%08lx); check sdmc:/3ds/dspfirm.cdc",(unsigned long)result);
        fh_platform_panic(message);
    }
    if(!fh_audio_game_dsp_initialize(flush_pcm,NULL))fh_platform_panic("Native DSP owner allocation failed");
    storage=linearMemAlign(CHANNELS*BLOCKS*MAX_FRAMES*4,32);
    reset_silence=linearMemAlign(FH_PCM_SILENCE*4,32);
    if(!storage || !reset_silence) {
        ndspExit();linearFree(storage);linearFree(reset_silence);
        fh_platform_panic("NDSP linear memory allocation failed");
    }
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    if(fh_audio_pcm_init(&pcm_queue,storage,CHANNELS*BLOCKS*MAX_FRAMES*4,
        reset_silence,FH_PCM_SILENCE*4,flush_pcm,NULL))fh_platform_panic("NDSP PCM owner initialization failed");
    pcm_queue.channels[0].output_guard=1;
    pcm_queue.channels[0].effect_order_guard=1;
    for(unsigned c=0;c<CHANNELS;c++)fh_audio_pcm_pause(&pcm_queue,c,pending_paused[c]);
    initialized=1;
    metrics=fopen("sdmc:/3ds/foxhollow/audio.csv","wb");
    if(fh_diagnostic_buffer(metrics))fh_platform_panic("Audio telemetry buffer allocation failed");
#ifdef FH_EMULATOR_TEST
    FILE *request=fopen("sdmc:/3ds/foxhollow/capture_audio.txt","rb");
    if(request) {
        fclose(request);capture_samples=calloc(CAPTURE_FRAMES+CAPTURE_GUARD,4);capture_snapshot=malloc(32000*4);
        if(!capture_samples || !capture_snapshot)fh_platform_panic("Audio capture allocation failed");
        memset(capture_samples+CAPTURE_FRAMES*2,0xa5,CAPTURE_GUARD*4);
        capture.data_pcm16=capture_samples;capture.nsamples=32000;
        ndspSetCapture(&capture);
    }
#endif
    __sync_synchronize();ndspSetCallback(audio_callback,NULL);
}
static int audio_space(unsigned channel) {
    if(channel>=CHANNELS)fh_platform_panic("Invalid native audio channel");
    initialize();return fh_audio_pcm_space(&pcm_queue,channel);
}
static unsigned audio_buffered(unsigned channel) {
    if(channel>=CHANNELS)fh_platform_panic("Invalid native audio buffer query");
    initialize();return fh_audio_pcm_buffered(&pcm_queue,channel);
}
static void audio_queue(unsigned channel,const int16_t *pcm,unsigned frames,unsigned rate) {
    if(channel>=CHANNELS)fh_platform_panic("Invalid native audio channel");
    initialize();
    if(fh_audio_pcm_queue(&pcm_queue,channel,pcm,frames,rate))fh_platform_panic("Native PCM queue submission failed");
}
void fh_platform_audio_queue_mapped(unsigned channel,const int16_t *pcm,unsigned frames,unsigned rate,const FhAudioMap *map) {
    io_enter();initialize();
    if(fh_audio_pcm_queue_mapped(&pcm_queue,channel,pcm,frames,rate,map))fh_platform_panic("Mapped PCM submission failed");
    io_leave();
}
static void audio_reset(unsigned channel) {
    if(channel>=CHANNELS)fh_platform_panic("Invalid native audio reset");
    if(initialized && fh_audio_pcm_reset(&pcm_queue,channel))fh_platform_panic("Native PCM reset rejected");
}
static void audio_pause(unsigned channel,int paused) {
    if(channel>=CHANNELS)fh_platform_panic("Invalid native audio pause");
    pending_paused[channel]=paused!=0;
    if(initialized)fh_audio_pcm_pause(&pcm_queue,channel,paused);
}
static void audio_finish(unsigned channel) {
    if(initialized && channel<CHANNELS)fh_audio_pcm_finish(&pcm_queue,channel);
}
unsigned fh_audio_resume_reset;
static void audio_observe(unsigned retrace) {
    if(!initialized)return;
    if(__atomic_exchange_n(&fh_audio_resume_reset,0,__ATOMIC_ACQ_REL)){
        for(unsigned c=0;c<CHANNELS;c++)fh_audio_pcm_reset(&pcm_queue,c);
        fh_platform_log("Audio output reset after resume\n");
    }
    if(retrace%30==0) {
        unsigned active,jobs,stack,core;fh_audio_worker_stats(&active,&jobs,&stack,&core);
        if(active){char line[144];snprintf(line,sizeof(line),"audio worker jobs=%u stack_peak_upper_bound=%u reported_core=%u\n",jobs,stack,core);fh_platform_log(line);}
        if(metrics)for(unsigned i=0;i<CHANNELS;i++) {
            FhAudioPcmStatus c=fh_audio_pcm_status(&pcm_queue,i);
            fprintf(metrics,"%u,%u,%u,%u,%u,%u,%d\n",retrace,i,c.rate,c.submitted,c.buffers,c.underruns,(int)c.paused);
        }
    }
#ifdef FH_EMULATOR_TEST
    if(capture_samples) {
        for(unsigned i=0;i<CAPTURE_GUARD*2;i++)if((u16)capture_samples[CAPTURE_FRAMES*2+i]!=0xa5a5)fh_platform_panic("NDSP capture guard overwritten");
        if(capture_ready) {
            char path[128];snprintf(path,sizeof(path),"sdmc:/3ds/foxhollow/audio-%06u.pcm",capture_retrace);
            FILE *file=fopen(path,"wb");if(!file || fwrite(capture_snapshot,4,32000,file)!=32000)fh_platform_panic("Audio capture write failed");
            fclose(file);capture_ready=0;
        }
        if(retrace%60==0 && !capture_request){capture_retrace=retrace;__sync_synchronize();capture_request=1;}
    }
#endif
}
static void audio_shutdown(void) {
    if(!initialized) return;
#ifdef FH_EMULATOR_TEST
    if(capture_samples)ndspSetCapture(NULL);
#endif
    ndspSetCallback(NULL,NULL);ndspExit();
    fh_audio_game_dsp_shutdown();
    linearFree(storage);linearFree(reset_silence);storage=reset_silence=NULL;
    initialized=0;memset(&pcm_queue,0,sizeof(pcm_queue));memset(pending_paused,0,sizeof(pending_paused));
    if(metrics){fclose(metrics);metrics=NULL;}
#ifdef FH_EMULATOR_TEST
    free(capture_samples);free(capture_snapshot);capture_samples=capture_snapshot=NULL;
#endif
}

/* Serialize our NDSP producer operations separately from synth computation.
 * The main renderer can service streams/observe queues without waiting for an
 * entire MusyX block; lock order is synth -> I/O, never the reverse. */
int fh_platform_audio_space(unsigned channel) {io_enter();int result=audio_space(channel);io_leave();return result;}
void fh_platform_audio_queue(unsigned channel,const int16_t *pcm,unsigned frames,unsigned rate) {io_enter();audio_queue(channel,pcm,frames,rate);io_leave();}
void fh_platform_audio_reset(unsigned channel) {io_enter();audio_reset(channel);io_leave();}
void fh_platform_audio_pause(unsigned channel,int paused) {io_enter();audio_pause(channel,paused);io_leave();}
void fh_platform_audio_finish(unsigned channel) {io_enter();audio_finish(channel);io_leave();}
unsigned fh_platform_audio_buffered(unsigned channel){io_enter();unsigned frames=audio_buffered(channel);io_leave();return frames;}
void fh_platform_audio_observe(unsigned retrace) {io_enter();audio_observe(retrace);io_leave();}
void fh_platform_audio_shutdown(void) {fh_audio_worker_stop();io_enter();audio_shutdown();io_leave();}

void fh_platform_audio_native_initialize(void) {io_enter();initialize();io_leave();}
