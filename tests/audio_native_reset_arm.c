/* Reuse original MusyX + actual SDK fixture without rerunning unrelated tests. */
#define run original_fixture_run
#include "audio_native_voice_arm.c"
#undef run
static FhAudioDspVoices saved_dsp;
static FhAudioNativeVoices saved_native;
static FhAudioSamplePool saved_pool;
static unsigned char saved_sdk[FH_SDK_CHANNEL_BYTES];
_Static_assert(FH_SDK_CHANNEL_BYTES==24*132,"Pinned SDK channel layout");
static u16 reset_sync(unsigned ch){u16 value;memcpy(&value,ndspChn+132*ch+8,2);return value;}
static void reset_reply(unsigned ch,u16 sync,u16 sequence) {
    unsigned char *p=shared[4+(ndspBufferId&1)]+12*ch;
    u16 flags=0x101,last=0;
    memcpy(p,&flags,2);memcpy(p+2,&sync,2);memcpy(p+8,&sequence,2);memcpy(p+10,&last,2);
    ndspiReadChnState();
}

static void snapshot(void) {
    saved_dsp=dsp;saved_native=native;saved_pool=pool;
    memcpy(saved_sdk,ndspChn,sizeof(saved_sdk));
}
static void unchanged_playback(void) {
    CHECK(!memcmp(&dsp,&saved_dsp,sizeof(dsp)));
    CHECK(!memcmp(&pool,&saved_pool,sizeof(pool)));
    CHECK(!memcmp(saved_sdk,ndspChn,sizeof(saved_sdk)));
}
static void source_reset(uint64_t epoch) {
    snapshot();CHECK(fh_audio_native_voices_reset(&native,epoch));
    unchanged_playback();CHECK(native.epoch==epoch);
    FhAudioSourceVoice empty={0};FhAudioNativeVoice empty_native={0};
    for(unsigned i=0;i<SYNTH_MAX_VOICES;i++)CHECK(!memcmp(&native.bridge.voices[i],&empty,sizeof(empty)));
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)CHECK(!memcmp(&native.voices[i],&empty_native,sizeof(empty_native)));
    CHECK(!native.aux_bound && !native.aux_epoch);
    for(unsigned i=0;i<2;i++)CHECK(!native.aux_handlers[i] && !native.aux_users[i]);
    CHECK(!memcmp(&saved_native.bridge.backend,&native.bridge.backend,sizeof(native.bridge.backend)));
    CHECK(native.admitted==saved_native.admitted && native.declined==saved_native.declined &&
          native.segments==saved_native.segments && native.stops==saved_native.stops &&
          native.faults==saved_native.faults && native.bridge.faults==saved_native.bridge.faults);
}
static void reset_aux(u8 reason,SND_AUX_INFO *data,void *user){(void)reason;(void)data;(void)user;}
void run(void) {
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    CHECK(!fh_audio_native_voices_reset(NULL,2));
    CHECK(!fh_audio_native_voices_reset(&native,2));
    CHECK(!fh_audio_pool_init(&pool,arena,sizeof(arena)));
    CHECK(!fh_audio_sample_registry_init(&registry,&pool,flush,NULL));
    CHECK(!fh_audio_dsp_voices_init(&dsp,&pool,silence,sizeof(silence),flush,NULL));
    CHECK(fh_audio_native_voices_init(&native,&dsp,&registry,1,2));
    setup();CHECK(!fh_audio_sample_registry_prepare(&registry,&info,&source_voice.smp_info));
    /* Reference first post-reset source cycle, including original ADSR/position. */
    controls(0);s16 pcm[320];salCtrlDsp(pcm);DSPvoice expected_first=source_voice;
    setup();fh_audio_source_bridge=&native.bridge;controls(0);salCtrlDsp(pcm);
    CHECK(!memcmp(&source_voice,&expected_first,sizeof(source_voice)) && !resample_calls);
    FhAudioPoolToken old=native.bridge.voices[0].token;CHECK(old && dsp.voices[0].lease==old);
    fh_audio_dsp_voices_service_window(&dsp,1,0,32,1);CHECK(dsp.voices[0].live);
    snapshot();
    CHECK(!fh_audio_native_voices_reset(&native,0));CHECK(!fh_audio_native_voices_reset(&native,1));
    CHECK(!memcmp(&native,&saved_native,sizeof(native)));unchanged_playback();
    source_reset(2);
    /* Resetting preparation must not mute the output generation still playing. */
    fh_audio_dsp_voices_service_window(&dsp,1,32,64,1);
    CHECK(dsp.voices[0].live && dsp.voices[0].lease==old && !ndspChnIsPaused(2));
    setup();controls(0);salCtrlDsp(pcm);
    CHECK(!memcmp(&source_voice,&expected_first,sizeof(source_voice)) && !resample_calls);
    FhAudioPoolToken next=native.bridge.voices[0].token;
    CHECK(next && next!=old && dsp.voices[1].lease==next);
    fh_audio_dsp_voices_service_window(&dsp,1,64,96,1);
    CHECK(dsp.voices[0].live && !dsp.voices[1].live);
    CHECK(dsp.voices[1].tail!=dsp.voices[1].head);
    fh_audio_dsp_voices_service_window(&dsp,2,0,32,1);
    CHECK(!dsp.voices[0].live && dsp.voices[1].live);
    CHECK(dsp.voices[0].lease==old && dsp.voices[1].lease==next);
    const void *data;const FhAudioDspSamplePlan *plan;
    CHECK(!fh_audio_pool_view(&pool,old,&data,&plan));
    uint32_t completed=0;CHECK(!fh_audio_dsp_voices_poll(&dsp,&completed));CHECK(!completed);
    /* Source generations can advance twice before callback playback catches up. */
    source_reset(3);setup();CHECK(fh_audio_source_begin(0,&source_voice,0,32000));
    FhAudioPoolToken third=native.bridge.voices[0].token;CHECK(third && third!=next);
    source_reset(4);setup();CHECK(fh_audio_source_begin(0,&source_voice,0,32000));
    FhAudioPoolToken fourth=native.bridge.voices[0].token;CHECK(fourth && fourth!=third);
    fh_audio_dsp_voices_service_window(&dsp,2,32,64,1);
    CHECK(dsp.voices[1].live && !dsp.voices[2].live && !dsp.voices[3].live);
    fh_audio_dsp_voices_service_window(&dsp,4,0,32,1);
    CHECK(!dsp.voices[1].live && !dsp.voices[2].live && dsp.voices[3].live);
    CHECK(dsp.voices[1].lease==next && dsp.voices[2].lease==third);
    /* Exhaust channels without callback acknowledgements. Reset still neither
     * waits nor frees samples; fresh unsupported starts remain CPU-owned. */
    for(unsigned i=4;i<FH_AUDIO_DSP_CHANNELS;i++) {
        CHECK(fh_audio_source_begin(i,&source_voice,0,32000));CHECK(native.bridge.voices[i].token);
    }
    source_reset(5);setup();CHECK(fh_audio_source_begin(0,&source_voice,0,32000));
    CHECK(!native.bridge.voices[0].token && native.declined);
    controls(0);salCtrlDsp(pcm);
    CHECK(!memcmp(&source_voice,&expected_first,sizeof(source_voice)) && resample_calls);
    /* AUX identity is not sufficient to reuse an old effect generation. */
    native.experimental_direct_stereo=4;native.aux_bound=1;native.aux_epoch=5;
    native.aux_handlers[0]=reset_aux;native.aux_users[0]=&info;
    source_reset(6);dspStudio[0].type=SND_STUDIO_TYPE_DPL2;
    dspStudio[0].auxAHandler=reset_aux;dspStudio[0].auxAUser=&info;
    snapshot();CHECK(fh_audio_native_voices_bind_aux(&native,0));unchanged_playback();
    CHECK(native.aux_bound && native.aux_epoch==6);
    /* Older leases remain protected. Invalid current/future/immediate leases
     * and residual source mappings may not be disguised as that old work. */
    for(unsigned scenario=0;scenario<5;scenario++) {
        unsigned timed=dsp.voices[0].timed;uint64_t epoch=dsp.voices[0].epoch;
        if(scenario==0)dsp.voices[0].epoch=6;
        if(scenario==1)dsp.voices[0].epoch=7;
        if(scenario==2)dsp.voices[0].timed=0;
        if(scenario==3)native.voices[0].token=old;
        if(scenario==4)native.bridge.voices[0].token=old;
        snapshot();CHECK(!fh_audio_native_voices_bind_aux(&native,0));
        CHECK(!memcmp(&native,&saved_native,sizeof(native)));unchanged_playback();
        dsp.voices[0].timed=timed;dsp.voices[0].epoch=epoch;
        native.voices[0].token=native.bridge.voices[0].token=0;
    }
    CHECK(fh_audio_source_begin(0,&source_voice,0,32000));CHECK(!native.bridge.voices[0].token);
    snapshot();CHECK(!fh_audio_native_voices_reset(&native,5));
    CHECK(!memcmp(&native,&saved_native,sizeof(native)));unchanged_playback();
    /* Only an actual SDK sync/sequence acknowledgement permits reuse, even
     * after several source resets. Neither DONE nor a stale reply suffices. */
    fh_audio_dsp_voices_service_window(&dsp,6,0,32,1);ndspiUpdateChn();
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++) {
        unsigned ch=i+FH_AUDIO_DSP_FIRST_CHANNEL;
        FhAudioPoolToken lease=dsp.voices[i].lease;CHECK(lease);
        FhAudioDspRetiringVoice *v=&dsp.retirement.voices[i];
        CHECK(v->sentinel.status==NDSP_WBUF_PLAYING && v->sentinel.sequence_id);
        reset_reply(ch,(u16)(reset_sync(ch)-1),v->sentinel.sequence_id);
        fh_audio_dsp_voices_service_window(&dsp,6,0,32,1);
        completed=0;CHECK(!fh_audio_dsp_voices_poll(&dsp,&completed) && !completed);
        CHECK(!fh_audio_pool_view(&pool,lease,&data,&plan));
        reset_reply(ch,reset_sync(ch),v->sentinel.sequence_id);
        fh_audio_dsp_voices_service_window(&dsp,6,0,32,1);
        CHECK(!fh_audio_pool_view(&pool,lease,&data,&plan));
        CHECK(!fh_audio_dsp_voices_poll(&dsp,&completed) && completed==(1u<<ch));
        CHECK(fh_audio_pool_view(&pool,lease,&data,&plan)==FH_AUDIO_POOL_INVALID);
        CHECK(!dsp.voices[i].lease);
    }
    CHECK(!fh_audio_dsp_retire_pending(&dsp.retirement));
    /* Future source starts can be admitted again. The voice which already
     * began in software is never silently promoted with stale histories. */
    setup();dspStudio[0].type=SND_STUDIO_TYPE_DPL2;
    dspStudio[0].auxAHandler=reset_aux;dspStudio[0].auxAUser=&info;
    CHECK(fh_audio_native_voices_bind_aux(&native,0));
    CHECK(native.aux_epoch==6 && native.aux_bound);
    CHECK(fh_audio_source_begin(0,&source_voice,0,32000));
    CHECK(native.bridge.voices[0].token && dsp.voices[0].epoch==6);
    CHECK(!native.faults && !native.bridge.faults && !dsp.faults);
    finished=1;done();
}
