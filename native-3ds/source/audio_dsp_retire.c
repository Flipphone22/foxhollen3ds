#include "fh_audio_dsp_retire.h"
#include <string.h>
static int overlaps(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y || (x<y+nb && y<x+na);
}

int fh_audio_dsp_retire_init(FhAudioDspRetirement *r,FhAudioSamplePool *pool,int16_t *silence,
                            size_t bytes,FhAudioPoolFlush flush,void *context) {
    if(!r || !pool || !pool->arena || !silence || ((uintptr_t)silence&31) ||
       bytes<FH_AUDIO_DSP_SILENCE_SAMPLES*2 || !flush ||
       overlaps(silence,FH_AUDIO_DSP_SILENCE_SAMPLES*2,pool->arena,pool->capacity) ||
       overlaps(silence,FH_AUDIO_DSP_SILENCE_SAMPLES*2,pool,sizeof(*pool)) ||
       overlaps(silence,FH_AUDIO_DSP_SILENCE_SAMPLES*2,r,sizeof(*r)) ||
       overlaps(r,sizeof(*r),pool,sizeof(*pool)) ||
       overlaps(r,sizeof(*r),pool->arena,pool->capacity))return FH_AUDIO_POOL_INVALID;
    if(r->initialized)return FH_AUDIO_POOL_BUSY;
    memset(silence,0,FH_AUDIO_DSP_SILENCE_SAMPLES*2);
    if(!flush(context,silence,FH_AUDIO_DSP_SILENCE_SAMPLES*2))return FH_AUDIO_POOL_FLUSH_FAILED;
    memset(r,0,sizeof(*r));r->pool=pool;r->silence=silence;r->initialized=1;
    return FH_AUDIO_POOL_OK;
}
enum { FREE=0,REQUESTED=1,WAITING=2,ACKNOWLEDGED=3,ARMED=4 };
int fh_audio_dsp_retire_arm(FhAudioDspRetirement *r,unsigned channel,FhAudioPoolToken id) {
    if(!r || !r->initialized || channel<FH_AUDIO_DSP_FIRST_CHANNEL || channel>=24)return FH_AUDIO_POOL_INVALID;
    FhAudioDspRetiringVoice *v=&r->voices[channel-FH_AUDIO_DSP_FIRST_CHANNEL];
    if(__atomic_load_n(&v->state,__ATOMIC_ACQUIRE)!=FREE)return FH_AUDIO_POOL_BUSY;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(r->voices[i].lease==id)return FH_AUDIO_POOL_INVALID;
    const void *data;const FhAudioDspSamplePlan *plan;
    if(fh_audio_pool_view(r->pool,id,&data,&plan)!=FH_AUDIO_POOL_OK)return FH_AUDIO_POOL_INVALID;
    (void)data;(void)plan;
    v->lease=id;
    __atomic_store_n(&v->state,ARMED,__ATOMIC_RELEASE);
    return FH_AUDIO_POOL_OK;
}
int fh_audio_dsp_retire_begin(FhAudioDspRetirement *r,unsigned channel) {
    if(!r || !r->initialized || channel<FH_AUDIO_DSP_FIRST_CHANNEL || channel>=24)return FH_AUDIO_POOL_INVALID;
    FhAudioDspRetiringVoice *v=&r->voices[channel-FH_AUDIO_DSP_FIRST_CHANNEL];
    unsigned expected=ARMED;
    if(!__atomic_compare_exchange_n(&v->state,&expected,REQUESTED,0,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return FH_AUDIO_POOL_BUSY;
    __atomic_add_fetch(&r->requests,1,__ATOMIC_RELAXED);return FH_AUDIO_POOL_OK;
}
int fh_audio_dsp_retire_request(FhAudioDspRetirement *r,unsigned channel,FhAudioPoolToken id) {
    int status=fh_audio_dsp_retire_arm(r,channel,id);
    return status==FH_AUDIO_POOL_OK?fh_audio_dsp_retire_begin(r,channel):status;
}
static void submit(FhAudioDspRetirement *r,unsigned channel,FhAudioDspRetiringVoice *v) {
    /* Clear marks old wave buffers DONE in software. Keep their lease until a
     * NEW looping sentinel's sequence is returned by the sync-matched reader.
     * PLAYING alone is also speculative: SDK command publication sets it early.
     * A stale old DSP reply with the same sequence is rejected there by sync.
     * This requires the documented NDSP-callback ownership: ReadChnState's
     * sync check/cache write must not race this clear on another CPU thread. */
    ndspChnWaveBufClear(channel);
    ndspChnSetFormat(channel,NDSP_FORMAT_MONO_PCM16);
    ndspChnSetInterp(channel,NDSP_INTERP_NONE);ndspChnSetRate(channel,32000.f);
    float mute[12]={0};ndspChnSetMix(channel,mute);
    memset(&v->sentinel,0,sizeof(v->sentinel));
    v->sentinel.data_pcm16=r->silence;v->sentinel.nsamples=FH_AUDIO_DSP_SILENCE_SAMPLES;
    v->sentinel.looping=true;
    ndspChnWaveBufAdd(channel,&v->sentinel);ndspChnSetPaused(channel,false);
}
void fh_audio_dsp_retire_service(FhAudioDspRetirement *r) {
    if(!r || !r->initialized)return;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++) {
        FhAudioDspRetiringVoice *v=&r->voices[i];unsigned channel=i+FH_AUDIO_DSP_FIRST_CHANNEL;
        unsigned state=__atomic_load_n(&v->state,__ATOMIC_ACQUIRE);
        if(state==REQUESTED) {
            submit(r,channel,v);__atomic_store_n(&v->state,WAITING,__ATOMIC_RELEASE);continue;
        }
        if(state!=WAITING)continue;
        if(__atomic_load_n(&v->sentinel.status,__ATOMIC_ACQUIRE)!=NDSP_WBUF_PLAYING ||
           !v->sentinel.sequence_id || ndspChnGetWaveBufSeq(channel)!=v->sentinel.sequence_id)continue;
        /* The permanently retained silence may still be read after this clear.
         * Never free it merely because no application sample remains pinned. */
        ndspChnWaveBufClear(channel);ndspChnSetPaused(channel,true);
        __atomic_add_fetch(&r->acknowledged,1,__ATOMIC_RELAXED);
        __atomic_store_n(&v->state,ACKNOWLEDGED,__ATOMIC_RELEASE);
    }
}
int fh_audio_dsp_retire_drain(FhAudioDspRetirement *r,uint32_t *completed) {
    if(!r || !r->initialized || !completed)return FH_AUDIO_POOL_INVALID;
    *completed=0;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++) {
        FhAudioDspRetiringVoice *v=&r->voices[i];
        if(__atomic_load_n(&v->state,__ATOMIC_ACQUIRE)!=ACKNOWLEDGED)continue;
        if(fh_audio_pool_release(r->pool,v->lease)!=FH_AUDIO_POOL_OK) {
            ++r->faults;return FH_AUDIO_POOL_INVALID;
        }
        v->lease=0;__atomic_store_n(&v->state,FREE,__ATOMIC_RELEASE);
        *completed|=1u<<(i+FH_AUDIO_DSP_FIRST_CHANNEL);
    }
    return FH_AUDIO_POOL_OK;
}
uint32_t fh_audio_dsp_retire_pending(const FhAudioDspRetirement *r) {
    if(!r || !r->initialized)return 0;
    uint32_t mask=0;
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(__atomic_load_n(&r->voices[i].state,__ATOMIC_ACQUIRE)!=FREE)mask|=1u<<(i+FH_AUDIO_DSP_FIRST_CHANNEL);
    return mask;
}
