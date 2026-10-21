#include "fh_audio_dsp_voice.h"
#include <math.h>
#include <string.h>
extern unsigned char ndspChn[];
extern void ndspiUpdateChn(void),ndspiReadChnState(void);
void *ndspVars[16];u16 ndspBufferId,ndspFrameId;
static unsigned char shared[16][8192] __attribute__((aligned(32)));
static unsigned char arena[4096] __attribute__((aligned(32)));
static int16_t silence[160] __attribute__((aligned(32)));
static unsigned char encoded[16]={0x11,0x23,0x65,0x43,0x12,0,0,0,0x23,0x18};
static FhAudioSamplePool pool;
static FhAudioDspVoices voices;
volatile unsigned failure,checks,finished;
const volatile unsigned abi_sizes[]={sizeof(FhAudioDspVoices),sizeof(FhAudioDspVoice),sizeof(ndspWaveBuf)};
_Static_assert(sizeof(ndspWaveBuf)==24 && offsetof(ndspWaveBuf,status)==17 && offsetof(ndspWaveBuf,sequence_id)==18,"Installed SDK wave ABI");
void LightLock_Lock(LightLock *p){(void)p;}
void LightLock_Unlock(LightLock *p){(void)p;}
u32 osConvertVirtToPhys(const void *p){return (uintptr_t)p;}
static int flush(void *p,const void *data,size_t n){(void)p;(void)data;(void)n;return 1;}
__attribute__((noinline,noreturn)) void done(void){for(;;){}}
#define CHECK(x) do {++checks;if(!(x)){failure=__LINE__;done();}}while(0)
static unsigned inject_abort;
static FhAudioPoolToken inject_bank,inject_voice;
void fh_test_after_head(FhAudioDspVoices *r,unsigned index) {
    if(!inject_abort || index)return;
    inject_abort=0;
    FhAudioDspControls c={.rate=32000,.interpolation=NDSP_INTERP_LINEAR};
    CHECK(fh_audio_dsp_voice_start_at(r,2,inject_bank,7,&c,201,10000,&inject_voice)==0);
    CHECK(fh_audio_dsp_voice_abort(r,2,inject_voice)==0);
}
static u16 local_sync(unsigned ch){u16 s;memcpy(&s,ndspChn+132*ch+8,2);return s;}
static void reply(unsigned ch,u16 sync,u16 sequence,u16 last) {
    unsigned char *p=shared[4+(ndspBufferId&1)]+12*ch;
    u16 flags=sequence?0x101:0x100;
    memcpy(p,&flags,2);memcpy(p+2,&sync,2);memcpy(p+8,&sequence,2);memcpy(p+10,&last,2);
    ndspiReadChnState();
}
static void acknowledge(unsigned ch,FhAudioPoolToken id) {
    uint32_t bits;const void *samples;const FhAudioDspSamplePlan *plan;
    FhAudioDspRetiringVoice *v=&voices.retirement.voices[ch-2];
    ndspiUpdateChn();
    CHECK(v->sentinel.status==NDSP_WBUF_PLAYING);
    fh_audio_dsp_voices_service(&voices);
    CHECK(fh_audio_dsp_voices_poll(&voices,&bits)==0 && !bits);
    CHECK(fh_audio_pool_view(&pool,id,&samples,&plan)==0);
    reply(ch,(u16)(local_sync(ch)-1),v->sentinel.sequence_id,0);
    fh_audio_dsp_voices_service(&voices);
    CHECK(fh_audio_dsp_voices_poll(&voices,&bits)==0 && !bits);
    reply(ch,local_sync(ch),v->sentinel.sequence_id,0);
    fh_audio_dsp_voices_service(&voices);
    CHECK(fh_audio_pool_view(&pool,id,&samples,&plan)==0);
    CHECK(fh_audio_dsp_voices_poll(&voices,&bits)==0 && bits==(1u<<ch));
    CHECK(fh_audio_pool_view(&pool,id,&samples,&plan)==FH_AUDIO_POOL_INVALID);
}
#ifdef FH_RETAINED_TEST
#include "audio_retained_controls.inc"
#endif
void run(void) {
    CHECK(abi_sizes[2]==24);
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    CHECK(fh_audio_pool_init(&pool,arena,sizeof(arena))==0);
    CHECK(fh_audio_dsp_voices_init(&voices,&pool,silence,sizeof(silence),flush,NULL)==0);
    FhAudioDspControls c={.rate=32000,.mix={.5f,.25f,0,0,.125f,.125f},.interpolation=NDSP_INTERP_LINEAR};
    FhAudioPoolToken bank,e,id;
    /* Every hardware channel, both SDK shared buffers, raw ADPCM and exact PCM,
     * first/loop histories, natural ends, pause/resume/rate/mix controls. */
    for(unsigned mode=0;mode<4;mode++)for(unsigned ch=2;ch<24;ch++) {
        ndspBufferId=ndspFrameId=mode&1;
        FhAudioSampleInput in={.encoded=encoded,.encoded_bytes=sizeof(encoded),.length=28,
            .initial_ps=(mode&2)?0x10:0x11,.loop_start=(mode&1)?14:0,
            .loop_length=(mode&1)?14:0,.loop_ps=0x23,.loop_y1=173,.loop_y2=-409};
        CHECK(fh_audio_pool_bank_open(&pool,&bank)==0);
        CHECK(fh_audio_pool_prepare(&pool,bank,9,&in,&e)==0);
        CHECK(fh_audio_dsp_voice_start(&voices,ch,bank,9,&c,&id)==FH_AUDIO_POOL_UNPUBLISHED);
        CHECK(fh_audio_pool_publish(&pool,e,flush,NULL)==0);
        FhAudioDspVoice *v=&voices.voices[ch-2];
        if(!mode)v->head=v->tail=UINT32_MAX-2; /* Before publication, test-only. */
        CHECK(fh_audio_dsp_voice_start(&voices,ch,bank,9,&c,&id)==0);
        CHECK(fh_audio_dsp_voice_start(&voices,ch,bank,9,&c,&e)==FH_AUDIO_POOL_BUSY);
        unsigned starts=voices.starts;
        /* Arming a lease must not cancel it before START is consumed. */
        fh_audio_dsp_retire_service(&voices.retirement);
        CHECK(voices.retirement.requests==voices.retirement.acknowledged);
        fh_audio_dsp_voices_service(&voices);
        CHECK(voices.starts==starts+1 && v->live);
        CHECK(ndspChnGetFormat(ch)==((mode&2)?NDSP_FORMAT_MONO_PCM16:NDSP_FORMAT_MONO_ADPCM));
        CHECK(ndspChnGetInterp(ch)==NDSP_INTERP_LINEAR);
        CHECK(fabsf(ndspChnGetRate(ch)*(float)NDSP_SAMPLE_RATE-32000)<.01f);
        CHECK(!ndspChnIsPaused(ch));
        CHECK(v->first.nsamples==28 && !v->first.looping);
        CHECK(v->first.data_vaddr==v->samples);
        CHECK(v->loop.nsamples==in.loop_length);
        if(!(mode&2)) {
            CHECK(v->first.adpcm_data==&v->first_history && v->first_history.index==0x11);
            CHECK(!v->first_history.history0 && !v->first_history.history1);
            if(mode&1)CHECK(v->loop.adpcm_data==&v->loop_history && v->loop_history.index==0x23 &&
                v->loop_history.history0==173 && v->loop_history.history1==-409 && v->loop.data_adpcm==(const u8 *)v->samples+8);
        } else CHECK(!v->first.adpcm_data && !v->loop.adpcm_data);
        FhAudioDspControls next=c;next.rate=16000;next.paused=1;next.mix[0]=.75f;next.interpolation=NDSP_INTERP_NONE;
        CHECK(fh_audio_dsp_voice_control(&voices,ch,id,&next)==0);
        fh_audio_dsp_voices_service(&voices);
        float mix[12];ndspChnGetMix(ch,mix);
        CHECK(!memcmp(mix,next.mix,sizeof(mix)) && ndspChnIsPaused(ch));
        CHECK(fabsf(ndspChnGetRate(ch)*(float)NDSP_SAMPLE_RATE-16000)<.01f && ndspChnGetInterp(ch)==NDSP_INTERP_NONE);
        CHECK(fh_audio_dsp_voice_control(&voices,ch,id,&c)==0);
        fh_audio_dsp_voices_service(&voices);ndspiUpdateChn();
        CHECK(!ndspChnIsPaused(ch));
        reply(ch,local_sync(ch),v->first.sequence_id,0);
        CHECK(v->first.status==NDSP_WBUF_PLAYING);
        CHECK(fh_audio_pool_bank_close(&pool,bank)==0 && pool.used);
        if(mode&1) {
            reply(ch,local_sync(ch),v->loop.sequence_id,v->first.sequence_id);
            CHECK(v->first.status==NDSP_WBUF_DONE && v->loop.status==NDSP_WBUF_PLAYING);
            fh_audio_dsp_voices_service(&voices);CHECK(v->live);
            CHECK(fh_audio_dsp_voice_stop(&voices,ch,id)==0);
            CHECK(fh_audio_dsp_voice_control(&voices,ch,id,&c)==FH_AUDIO_POOL_BUSY);
        } else {
            reply(ch,local_sync(ch),0,v->first.sequence_id);
            CHECK(v->first.status==NDSP_WBUF_DONE);
        }
        fh_audio_dsp_voices_service(&voices);CHECK(!v->live && pool.used);
        acknowledge(ch,id);CHECK(!pool.used);
        CHECK(fh_audio_dsp_voice_control(&voices,ch,id,&c)==FH_AUDIO_POOL_INVALID);
    }
    CHECK(voices.starts==88 && voices.natural_ends==44 && voices.stops==44 && !voices.faults);
    /* Bounded queue backpressure and STOP before first device publication. */
    FhAudioSampleInput in={.encoded=encoded,.encoded_bytes=sizeof(encoded),.length=28,.initial_ps=0x11};
    CHECK(fh_audio_pool_bank_open(&pool,&bank)==0);
    CHECK(fh_audio_pool_prepare(&pool,bank,7,&in,&e)==0);
    CHECK(fh_audio_pool_publish(&pool,e,flush,NULL)==0);
    CHECK(fh_audio_dsp_voice_start(&voices,2,bank,7,&c,&id)==0);
    for(unsigned i=0;i<FH_AUDIO_DSP_COMMANDS-1;i++){c.rate=8000+(i%3)*1000;CHECK(fh_audio_dsp_voice_control(&voices,2,id,&c)==0);}
    unsigned head=voices.voices[0].head;
    CHECK(fh_audio_dsp_voice_stop(&voices,2,id)==FH_AUDIO_POOL_BUSY);
    CHECK(fh_audio_dsp_voice_control(&voices,2,id,&c)==FH_AUDIO_POOL_BUSY);
    CHECK(voices.voices[0].head==head);
    fh_audio_dsp_voices_service(&voices);
    CHECK(fabsf(ndspChnGetRate(2)*(float)NDSP_SAMPLE_RATE-c.rate)<.01f);
    CHECK(fh_audio_dsp_voice_stop(&voices,2,id)==0);
    fh_audio_dsp_voices_service(&voices);acknowledge(2,id);
    CHECK(fh_audio_dsp_voice_start(&voices,2,bank,7,&c,&id)==0);
    CHECK(fh_audio_dsp_voice_stop(&voices,2,id)==0);
    fh_audio_dsp_voices_service(&voices);
    CHECK(!voices.voices[0].live && voices.voices[0].first.status==NDSP_WBUF_DONE);
    acknowledge(2,id);
    FhAudioDspControls bad=c;bad.rate=NAN;
    CHECK(fh_audio_dsp_voice_start(&voices,2,bank,7,&bad,&id)==FH_AUDIO_POOL_INVALID);
    bad=c;bad.mix[11]=INFINITY;
    CHECK(fh_audio_dsp_voice_start(&voices,2,bank,7,&bad,&id)==FH_AUDIO_POOL_INVALID);
    CHECK(fh_audio_dsp_voice_start(&voices,1,bank,7,&c,&id)==FH_AUDIO_POOL_INVALID);
    CHECK(fh_audio_dsp_voice_start(&voices,24,bank,7,&c,&id)==FH_AUDIO_POOL_INVALID);
    /* Future playback is tied to a supplied source-output epoch/cursor, not CPU
     * preparation time. Hold/resume must preserve the caller's pause setting. */
    unsigned prior_starts=voices.starts;uint32_t bits;
    c.rate=32000;c.paused=0;
    CHECK(fh_audio_dsp_voice_start_at(&voices,2,bank,7,&c,99,1000,&id)==0);
    FhAudioDspControls change=c;change.rate=16000;change.paused=1;
    CHECK(fh_audio_dsp_voice_control(&voices,2,id,&change)==FH_AUDIO_POOL_INVALID);
    CHECK(fh_audio_dsp_voice_control_at(&voices,2,id,&change,1200)==0);
    CHECK(fh_audio_dsp_voice_control_at(&voices,2,id,&change,1199)==FH_AUDIO_POOL_INVALID);
    CHECK(fh_audio_dsp_voice_stop_at(&voices,2,id,1600)==0);
    fh_audio_dsp_voices_service_at(&voices,99,999,1);CHECK(voices.starts==prior_starts);
    fh_audio_dsp_voices_service_at(&voices,99,1007,0);CHECK(voices.starts==prior_starts);
    fh_audio_dsp_voices_service_at(&voices,99,1007,1);
    CHECK(voices.starts==prior_starts+1 && voices.late_samples==7 && !ndspChnIsPaused(2));
    fh_audio_dsp_voices_service_at(&voices,99,1007,0);CHECK(ndspChnIsPaused(2));
    fh_audio_dsp_voices_service_at(&voices,99,1007,1);CHECK(!ndspChnIsPaused(2));
    fh_audio_dsp_voices_service_at(&voices,99,1200,1);CHECK(ndspChnIsPaused(2));
    fh_audio_dsp_voices_service_at(&voices,99,1200,0);
    fh_audio_dsp_voices_service_at(&voices,99,1200,1);CHECK(ndspChnIsPaused(2));
    fh_audio_dsp_voices_service_at(&voices,99,1599,1);CHECK(voices.voices[0].live);
    fh_audio_dsp_voices_service_at(&voices,99,1600,1);CHECK(!voices.voices[0].live);
    acknowledge(2,id);
    /* A reset cancels future starts; it must not play them on a new epoch. */
    prior_starts=voices.starts;
    CHECK(fh_audio_dsp_voice_start_at(&voices,2,bank,7,&c,100,UINT64_C(1)<<40,&id)==0);
    fh_audio_dsp_voices_service_at(&voices,101,0,0);
    CHECK(voices.starts==prior_starts && voices.epoch_rejected==1);
    acknowledge(2,id);
    /* Abort bypasses a full future queue, without early pool release. */
    CHECK(fh_audio_dsp_voice_start_at(&voices,2,bank,7,&c,101,1000,&id)==0);
    for(unsigned i=0;i<FH_AUDIO_DSP_COMMANDS-1;i++)CHECK(fh_audio_dsp_voice_control_at(&voices,2,id,&c,1000+i)==0);
    CHECK(fh_audio_dsp_voice_stop_at(&voices,2,id,2000)==FH_AUDIO_POOL_BUSY);
    CHECK(fh_audio_dsp_voice_abort(&voices,2,id)==0);
    CHECK(fh_audio_dsp_voices_poll(&voices,&bits)==0 && !bits);
    fh_audio_dsp_voices_service_at(&voices,101,0,0);
    CHECK(voices.starts==prior_starts && voices.cancelled==1);
    acknowledge(2,id);
    /* An abort posted after natural retirement ACK must be consumed before
     * producer poll permits another generation on that channel. */
    CHECK(fh_audio_dsp_voice_start(&voices,2,bank,7,&c,&id)==0);
    fh_audio_dsp_voices_service(&voices);ndspiUpdateChn();
    CHECK(fh_audio_dsp_voice_stop(&voices,2,id)==0);
    fh_audio_dsp_voices_service(&voices);ndspiUpdateChn();
    reply(2,local_sync(2),voices.retirement.voices[0].sentinel.sequence_id,0);
    fh_audio_dsp_voices_service(&voices);
    CHECK(fh_audio_dsp_voice_abort(&voices,2,id)==0);
    CHECK(fh_audio_dsp_voices_poll(&voices,&bits)==0 && !bits);
    CHECK(fh_audio_dsp_voice_start(&voices,2,bank,7,&c,&e)==FH_AUDIO_POOL_BUSY);
    fh_audio_dsp_voices_service(&voices);
    CHECK(fh_audio_dsp_voices_poll(&voices,&bits)==0 && bits==4);
    /* Test-only boundary injection executes the producer after callback's first
     * head snapshot but before its acquire of cancellation. No SDK writes from
     * the injected producer; this is one explicit interleaving, not thread stress. */
    inject_bank=bank;inject_abort=1;
    fh_audio_dsp_voices_service_at(&voices,201,0,0);
    CHECK(!inject_abort && voices.voices[0].head==voices.voices[0].tail);
    acknowledge(2,inject_voice);
    /* A later source start inside this block gets silence at its INITIAL
     * curve pitch. Prefix, sample and loop descriptors remain separately owned. */
    c.rate=32000;c.paused=0;memset(c.mix,0,sizeof(c.mix));
    CHECK(!fh_audio_dsp_voice_start_ramped_at(&voices,2,bank,7,&c,300,1032,&id));
    prior_starts=voices.starts;head=voices.voices[0].tail;
    fh_audio_dsp_voices_service_region(&voices,300,1000,1156,0,1);
    CHECK(voices.starts==prior_starts && voices.voices[0].tail==head && !voices.voices[0].live);
    FhAudioDspCurve curve={.rate=48000,.frames=32,.cycle_frames=160,.interpolation=NDSP_INTERP_LINEAR,
        .volume_start={16384,8192},.volume_end={16384,8192},.env_start=16000,.env_end=20000};
    CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&curve,1032));
    fh_audio_dsp_voices_service_region(&voices,300,1000,1156,0,1);
    FhAudioDspVoice *delayed=&voices.voices[0];
    CHECK(delayed->live && delayed->delay.nsamples==48 && voices.prefix_starts==1);
    CHECK(delayed->delay.data_vaddr==silence && delayed->first.data_vaddr==delayed->samples);
    CHECK(fabsf(ndspChnGetRate(2)*(float)NDSP_SAMPLE_RATE-48000)<.01f);
    ndspiUpdateChn();CHECK(delayed->delay.sequence_id && delayed->delay.sequence_id!=delayed->first.sequence_id);
    CHECK(!fh_audio_dsp_voice_stop_at(&voices,2,id,1160));
    fh_audio_dsp_voices_service_at(&voices,300,1160,1);acknowledge(2,id);
    /* Small retained silence cannot cover a 192kHz prefix. Fail without SDK
     * publication or releasing its lease; explicit cancellation still works. */
    prior_starts=voices.starts;unsigned faults=voices.prefix_faults;
    CHECK(!fh_audio_dsp_voice_start_ramped_at(&voices,2,bank,7,&c,301,1150,&id));
    curve.rate=192000;CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&curve,1150));
    head=delayed->tail;fh_audio_dsp_voices_service_region(&voices,301,1000,1156,0,1);
    CHECK(voices.starts==prior_starts && voices.prefix_faults==faults+1 && !delayed->live && delayed->tail==head);
    CHECK(delayed->lease==id);CHECK(!fh_audio_dsp_voice_abort(&voices,2,id));
    fh_audio_dsp_voices_service_at(&voices,301,1000,0);acknowledge(2,id);
    /* Preparing the next source generation must not cancel it while older
     * PCM is still playing. Neither gap cursors nor stale replies free it. */
    FhAudioPoolToken next_id;
    CHECK(!fh_audio_dsp_voice_start_at(&voices,2,bank,7,&c,400,0,&id));
    fh_audio_dsp_voices_service_at(&voices,400,0,1);
    CHECK(voices.voices[0].live);
    CHECK(!fh_audio_dsp_voice_start_ramped_at(&voices,3,bank,7,&c,401,32,&next_id));
    curve.rate=32000;CHECK(!fh_audio_dsp_voice_curve_at(&voices,3,next_id,&curve,32));
    unsigned next_tail=voices.voices[1].tail;
    prior_starts=voices.starts;unsigned rejected=voices.epoch_rejected;
    for(unsigned repeat=0;repeat<8;repeat++){
        fh_audio_dsp_voices_service_at(&voices,400,1000+repeat*160,1);
        CHECK(voices.voices[0].live && !ndspChnIsPaused(2));
        CHECK(!voices.voices[1].live && voices.voices[1].tail==next_tail);
        CHECK(voices.voices[1].lease==next_id && voices.starts==prior_starts);
        CHECK(voices.epoch_rejected==rejected);
    }
    fh_audio_dsp_voices_service_at(&voices,0,0,0);
    CHECK(voices.voices[0].live && ndspChnIsPaused(2));
    CHECK(voices.voices[1].tail==next_tail && voices.epoch_rejected==rejected);
    fh_audio_dsp_voices_service_at(&voices,400,2300,1);CHECK(!ndspChnIsPaused(2));
    faults=voices.prefix_faults;
    fh_audio_dsp_voices_service_region(&voices,401,0,156,4,1);
    CHECK(!voices.voices[0].live && voices.epoch_rejected==rejected+1);
    CHECK(voices.voices[1].live && voices.starts==prior_starts+1);
    CHECK(voices.voices[1].delay.nsamples==36 && voices.prefix_faults==faults);
    CHECK(voices.voices[0].lease==id);acknowledge(2,id);
    fh_audio_dsp_voices_service_at(&voices,401,156,1);
    CHECK(!fh_audio_dsp_voice_stop_at(&voices,3,next_id,200));
    fh_audio_dsp_voices_service_at(&voices,401,200,1);acknowledge(3,next_id);
    /* Advancing over an unplayed epoch retires its queued START exactly once. */
    CHECK(!fh_audio_dsp_voice_start_at(&voices,2,bank,7,&c,402,0,&id));
    rejected=voices.epoch_rejected;prior_starts=voices.starts;
    fh_audio_dsp_voices_service_at(&voices,401,0,0);
    CHECK(voices.epoch_rejected==rejected && voices.starts==prior_starts);
    fh_audio_dsp_voices_service_at(&voices,403,0,0);
    CHECK(voices.epoch_rejected==rejected+1 && voices.starts==prior_starts);
    acknowledge(2,id);
#ifdef FH_RETAINED_TEST
    retained_controls(bank);
#endif
    CHECK(fh_audio_pool_bank_close(&pool,bank)==0);
    CHECK(fh_audio_pool_shutdown(&pool)==0 && !voices.faults);
    finished=1;done();
}
