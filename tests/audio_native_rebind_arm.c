/* Original source control and pinned SDK owner; replies are synthetic. */
#define run original_fixture_run
#include "audio_native_voice_arm.c"
#undef run
static FhAudioDspVoices saved_dsp;
static FhAudioNativeVoices saved_native;
static FhAudioSamplePool saved_pool;
static unsigned char saved_sdk[FH_SDK_CHANNEL_BYTES];
static int old_users[2],new_users[2];
static void old_handler(u8 reason,SND_AUX_INFO *data,void *user){(void)reason;(void)data;(void)user;}
static void new_handler(u8 reason,SND_AUX_INFO *data,void *user){(void)reason;(void)data;(void)user;}
static void snapshot(void) {
    saved_dsp=dsp;saved_native=native;saved_pool=pool;memcpy(saved_sdk,ndspChn,sizeof(saved_sdk));
}
static void unchanged_playback(void) {
    CHECK(!memcmp(&saved_dsp,&dsp,sizeof(dsp)) && !memcmp(&saved_pool,&pool,sizeof(pool)));
    CHECK(!memcmp(saved_sdk,ndspChn,sizeof(saved_sdk)));
}
static void refresh(void) {
    snapshot();CHECK(fh_audio_native_voices_refresh_aux(&native,0));unchanged_playback();
    saved_native.aux_handlers[0]=dspStudio[0].auxAHandler;saved_native.aux_handlers[1]=dspStudio[0].auxBHandler;
    saved_native.aux_users[0]=dspStudio[0].auxAUser;saved_native.aux_users[1]=dspStudio[0].auxBUser;
    CHECK(!memcmp(&saved_native,&native,sizeof(native)));
}
static void rejected(unsigned studio) {
    snapshot();CHECK(!fh_audio_native_voices_refresh_aux(&native,studio));unchanged_playback();
    CHECK(!memcmp(&saved_native,&native,sizeof(native)));
}
void run(void) {
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    CHECK(!fh_audio_native_voices_refresh_aux(NULL,0));
    for(unsigned mode=3;mode<=4;mode++) {
        memset(&pool,0,sizeof(pool));memset(&registry,0,sizeof(registry));
        memset(&dsp,0,sizeof(dsp));memset(&native,0,sizeof(native));
        CHECK(!fh_audio_pool_init(&pool,arena,sizeof(arena)));
        CHECK(!fh_audio_sample_registry_init(&registry,&pool,flush,NULL));
        CHECK(!fh_audio_dsp_voices_init(&dsp,&pool,silence,sizeof(silence),flush,NULL));
        CHECK(fh_audio_native_voices_init(&native,&dsp,&registry,1,mode));
        setup();dspStudio[0].type=mode==4?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
        dspStudio[0].auxAHandler=dspStudio[0].auxBHandler=old_handler;
        dspStudio[0].auxAUser=&old_users[0];dspStudio[0].auxBUser=&old_users[1];
        CHECK(fh_audio_native_voices_bind_aux(&native,0));
        CHECK(!fh_audio_sample_registry_prepare(&registry,&info,&source_voice.smp_info));
        source_voice.volLa=source_voice.volRa=8192;source_voice.volLb=source_voice.volRb=4096;
        fh_audio_source_bridge=&native.bridge;controls(0);s16 pcm[320];salCtrlDsp(pcm);
        CHECK(native.bridge.voices[0].token && native.voices[0].token && !resample_calls);
        fh_audio_dsp_voices_service_window(&dsp,1,0,32,1);CHECK(dsp.voices[0].live);
        /* A currently audible lease and queued old controls survive pointer changes. */
        dspStudio[0].auxAHandler=dspStudio[0].auxBHandler=new_handler;
        dspStudio[0].auxAUser=&new_users[0];dspStudio[0].auxBUser=&new_users[1];
        refresh();unsigned prior=native.segments;controls(1);salCtrlDsp(pcm);
        CHECK(native.segments>prior && !native.faults && !native.bridge.faults && !resample_calls);
        /* This API captures handler masks; actual game bus changes remain a
         * separate admission decision. Only future controls see the new mask. */
        dspStudio[0].auxAHandler=NULL;dspStudio[0].auxAUser=NULL;refresh();
        unsigned head=dsp.voices[0].head;controls(2);salCtrlDsp(pcm);CHECK(dsp.voices[0].head>head);
        for(unsigned at=head;at<dsp.voices[0].head;at++) {
            const FhAudioDspCommand *c=&dsp.voices[0].commands[at%FH_AUDIO_DSP_COMMANDS];
            CHECK(c->kind==4); /* CURVE, private enum in actual owner. */
            for(unsigned ch=3;ch<6;ch++)CHECK(!c->curve.volume_start[ch] && !c->curve.volume_end[ch]);
        }
        FhAudioNativeVoices good=native;DSPstudioinfo studio=dspStudio[0];
        for(unsigned i=0;i<11;i++) {
            if(i==0)native.initialized=0;
            if(i==1)native.experimental_direct_stereo=2;
            if(i==2)native.aux_bound=0;
            if(i==3)native.aux_epoch=2;
            if(i==4)native.aux_studio=1;
            if(i==5)dspStudio[0].state=DSP_STUDIO_STATE_INACTIVE;
            if(i==6)dspStudio[0].isMaster=0;
            if(i==7)dspStudio[0].type=mode==4?SND_STUDIO_TYPE_STD:SND_STUDIO_TYPE_DPL2;
            if(i==8)dspStudio[0].numInputs=1;
            if(i==9)native.voices[0].studio=1;
            rejected(i==10?8:0);native=good;dspStudio[0]=studio;
        }
        refresh();CHECK(!native.faults && !native.bridge.faults && !dsp.faults);
    }
    finished=1;done();
}
