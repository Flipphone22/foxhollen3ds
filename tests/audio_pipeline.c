/* Compare complete production DMA output across delayed worker schedules.
 * No game callback, ring rotation, sample or synth mutation may disappear. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../native-3ds/source/audio.c"
volatile u32 fh_audio_voice_starts,fh_audio_voice_stops;
static unsigned active,available,packets,generated,wrapped,revision,mode,lock_depth;
static unsigned test_schedule,callback_hash,render_hash;
unsigned fh_platform_audio_buffered(unsigned channel){assert(channel==0);return (8-available)*640;}
static AIDCallback previous;
static s16 movie[2][320],*delayed_source;
enum { PACKETS=128, SAMPLES=PACKETS*640*2 };
static s16 trace[SAMPLES],reference[SAMPLES];
void fh_platform_log(const char *s){(void)s;}
void fh_platform_panic(const char *s){fprintf(stderr,"%s\n",s);abort();}
uint64_t fh_platform_ticks(void){static uint64_t t;return ++t;}
void fh_platform_audio_pause(unsigned c,int p){(void)c;(void)p;}
void fh_platform_audio_reset(unsigned c){(void)c;}
void fh_platform_audio_finish(unsigned c){assert(c==1);}
int fh_platform_audio_space(unsigned c){return c==0 && available && packets<PACKETS;}
void fh_platform_audio_queue(unsigned c,const int16_t *p,unsigned n,unsigned r){
    assert(c==0 && n==640 && r==32000 && available && packets<PACKETS);
    memcpy(trace+packets*1280,p,2560);packets++;available--;
}
bool sndPCRender(s16 *pcm,size_t frames){
    assert(lock_depth && frames==160);
    /* Real sndPCRender recursively enters the configured synth lock. */
    enter();render_hash=render_hash*33+revision;
    for(unsigned i=0;i<frames;i++,generated++){
        pcm[2*i]=(s16)(1000+generated*13+revision*7);
        pcm[2*i+1]=(s16)(-2000-generated*3+revision*11);
    }
    leave();return true;
}
s32 DVDReadPrio(DVDFileInfo*f,void*d,s32 n,s32 o,s32 p){(void)f;(void)d;(void)n;(void)o;(void)p;abort();}
void fh_audio_lock(void){lock_depth++;}
void fh_audio_unlock(void){assert(lock_depth);lock_depth--;}
int fh_audio_try_lock(void){fh_audio_lock();return 1;}
int fh_audio_worker_active(void){return active;}
void fh_audio_worker_start(int (*step)(void)){assert(step==worker_step);}
void fh_audio_worker_wake(void){}
static void movie_mix(short *out,short *source,unsigned samples){
    assert(samples==160);
    for(unsigned i=0;i<320;i++){
        int value=(source?source[i]:0)+(int)((wrapped+i)%101)-50;
        out[i]=value<-32768?-32768:value>32767?32767:value;
    }
}
void AttractMovieAudio_DmaCallback(void){
    s16 *source=delayed_source;
    previous();delayed_source=(s16*)fhAIGetDMAStartAddr();
    if(mode==2)source=delayed_source;
    s16 *out=movie[wrapped&1];
    fh_audio_movie_mix(out,source,160,movie_mix);
    wrapped++;callback_hash=callback_hash*33+wrapped;
    AIInitDMA((uintptr_t)out,640);
    if(!(wrapped%11)){enter();revision++;leave();}
}
static void run(unsigned enabled,unsigned schedule){
    test_schedule=schedule;
    active=1;available=packets=generated=wrapped=revision=callback_hash=render_hash=0;
    delayed_source=NULL;memset(movie,0,sizeof(movie));memset(trace,0,sizeof(trace));
    pending_synth=NULL;dma_pcm_frames=dma_pcm_slices=0;critical=pumping=0;
    fh_audio_synth_jobs_enabled=0;fh_audio_pipeline_enabled=enabled;
    assert(pipeline_read==pipeline_write && !pipeline_reserved);
    fh_audio_pipeline_worker=fh_audio_pipeline_inline=fh_audio_pipeline_packets=fh_audio_pipeline_peak=0;
    fh_audio_synth_jobs_queued=fh_audio_synth_jobs_worker=fh_audio_synth_jobs_inline=0;
    fh_audio_synth_jobs_urgent=0;
    AISetDSPSampleRate(AI_SAMPLERATE_32KHZ);fhMusyxStartOutput();
    previous=AIRegisterDMACallback(AttractMovieAudio_DmaCallback);
    unsigned iteration=0;
    while(packets<PACKETS){
        assert(++iteration<PACKETS*40);available=PACKETS-packets<3?PACKETS-packets:3;fh_audio_pump();
        /* Include repeated producer polls while its job is still pending. */
        if(enabled && iteration%(schedule+2)){
            if(iteration%7==0)fh_audio_drain_pending();
            else {fh_audio_lock();worker_step();fh_audio_unlock();}
        }
    }
    AIRegisterDMACallback(NULL);AIStopDMA();
    assert(!pending_synth && !dma_pcm_frames && !dma_pcm_slices && !lock_depth);
    assert(generated==PACKETS*640 && wrapped==PACKETS*4);
    if(enabled)assert(fh_audio_pipeline_worker && fh_audio_pipeline_packets==PACKETS && pipeline_read==pipeline_write && !pipeline_reserved);

}
static void plain_reset(void){
    active=1;available=1;packets=generated=wrapped=revision=callback_hash=render_hash=0;
    critical=pumping=0;fh_audio_pipeline_enabled=fh_audio_synth_jobs_enabled=0;
    assert(!pending_synth && !dma_pcm_slices && pipeline_read==pipeline_write);
    memset(trace,0,sizeof(trace));fhMusyxStartOutput();
}
static void plain_run(unsigned sliced){
    plain_reset();
    while(packets<PACKETS){
        available=1;unsigned old=generated,count=packets;
        fh_audio_lock();
        if(sliced)worker_step();else mix_pending(1,~0u);
        fh_audio_unlock();
        assert(generated-old==(sliced?160:640));
        if(sliced)assert(packets-count==(generated%640==0));
    }
    AIRegisterDMACallback(NULL);AIStopDMA();
    assert(generated==PACKETS*640 && !lock_depth && !dma_pcm_slices);
}
int main(void){
    plain_run(0);memcpy(reference,trace,sizeof(trace));
    plain_run(1);assert(!memcmp(reference,trace,sizeof(trace)));
    for(unsigned slices=1;slices<4;slices++){
        plain_reset();
        for(unsigned i=0;i<slices;i++){fh_audio_lock();worker_step();fh_audio_unlock();}
        assert(!packets && dma_pcm_slices==slices);
        previous=AIRegisterDMACallback(AttractMovieAudio_DmaCallback);
        assert(packets==1 && !dma_pcm_slices && previous==mix_callback);
        assert(!memcmp(reference,trace,640*2*sizeof(s16)));
        AIRegisterDMACallback(NULL);AIStopDMA();
        plain_reset();
        for(unsigned i=0;i<slices;i++){fh_audio_lock();worker_step();fh_audio_unlock();}
        AIStopDMA();assert(!dma_pcm_slices && !dma_pcm_frames && !packets);
        AIRegisterDMACallback(NULL);
    }
    puts("PASS: sliced worker produces identical 128 stereo packets; one synth block per lock; all partial callback handoffs and stops");
    for(mode=1;mode<=2;mode++){
        run(0,0);memcpy(reference,trace,sizeof(trace));unsigned cb=callback_hash,render=render_hash;
        for(unsigned schedule=0;schedule<8;schedule++){
            run(1,schedule);assert(!memcmp(reference,trace,sizeof(trace)));
            assert(cb==callback_hash && render==render_hash);
        }
    }
    puts("PASS: 16 delayed schedules; byte-identical stereo output, callback and synth-mutation order; 640-frame packets; bounded ordered PCM pipeline drained");
}

void fh_dvd_stream_prepare(DVDFileInfo *file){(void)file;}

