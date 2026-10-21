/* Included by the real pinned-SDK protocol fixture. */
static void retained_controls(FhAudioPoolToken bank) {
    FhAudioDspControls initial={.rate=32000,.interpolation=NDSP_INTERP_LINEAR};
    FhAudioPoolToken id;
    CHECK(!fh_audio_dsp_voice_start_ramped_at(&voices,2,bank,7,&initial,500,1000,&id));
    FhAudioDspVoice *v=&voices.voices[0];
    FhAudioDspCurve c={.rate=32000,.frames=32,.cycle_frames=160,.interpolation=NDSP_INTERP_LINEAR,
        .env_start=32767,.env_end=32767,.volume_start={16384,8192,4096,2048,512,1024,3000,4000,5000},
        .volume_end={16384,8192,4096,2048,512,1024,3000,4000,5000}};
    CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,1000));
    unsigned head=v->head,retained=voices.retained_controls,shortages=voices.curve_shortages;
    uint64_t due=1000;
    for(unsigned i=1;i<=10000;i++) {
        due+=32;c.offset=(i%5)*32;
        CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));
        CHECK(v->head==head && v->last_due==due && v->curve_end==due+32);
    }
    CHECK(voices.retained_controls==retained+10000);
    fh_audio_dsp_voices_service_window(&voices,500,1000,1156,1);
    float expected[12],actual[12];CHECK(fh_audio_dsp_curve_mix(&c,32,expected));
    ndspChnGetMix(2,actual);CHECK(!memcmp(expected,actual,sizeof(actual)));
    CHECK(voices.curve_shortages==shortages && !ndspChnIsPaused(2));
    fh_audio_dsp_voices_service_window(&voices,500,due,due+156,1);
    ndspChnGetMix(2,actual);CHECK(!memcmp(expected,actual,sizeof(actual)));
    CHECK(voices.curve_shortages==shortages);
    CHECK(fh_audio_dsp_voice_stop_at(&voices,2,id,due-1)==FH_AUDIO_POOL_INVALID);
    /* A constant control cannot hide even one changed property. Each change
     * is kept at its original deadline, including across a depleted queue. */
    for(unsigned change=0;change<7;change++) {
        due+=32;head=v->head;
        if(change==0)c.rate=16000;
        if(change==1)c.interpolation=NDSP_INTERP_NONE;
        if(change==2)c.paused=1;
        if(change==3)c.routing=FH_AUDIO_DSP_ROUTE_DPL2_STEREO;
        if(change==4)c.env_start=c.env_end=10000;
        if(change==5)c.volume_start[8]=c.volume_end[8]=2345;
        if(change==6)c.volume_start[0]=c.volume_end[0]=(uint16_t)-12345;
        CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));CHECK(v->head==head+1);
        unsigned tail=v->tail;
        fh_audio_dsp_voices_service_window(&voices,500,due-1,due+16,1);
        CHECK(v->tail==tail); /* gain lookahead does not apply pitch/pause early */
        fh_audio_dsp_voices_service_window(&voices,500,due,due+32,1);
        CHECK(v->tail==v->head && ndspChnIsPaused(2)==(c.paused!=0));
        CHECK(fabsf(ndspChnGetRate(2)-c.rate/(float)NDSP_SAMPLE_RATE)<.000001f);
        CHECK(fh_audio_dsp_curve_mix(&c,32,expected));ndspChnGetMix(2,actual);
        CHECK(!memcmp(expected,actual,sizeof(actual)));
        due+=32;head=v->head;
        CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));CHECK(v->head==head);
    }
    /* A gap requires publication, and an identical repeated ramp must never
     * be mistaken for constant state (its envelope starts again). */
    due+=64;head=v->head;CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));CHECK(v->head==head+1);
    fh_audio_dsp_voices_service_at(&voices,500,due,1);
    c.env_start=1000;c.env_end=2000;
    for(unsigned i=0;i<2;i++) {
        due+=32;head=v->head;CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));CHECK(v->head==head+1);
        fh_audio_dsp_voices_service_at(&voices,500,due,1);
    }
    c.env_start=c.env_end=32767;c.volume_start[0]=1000;c.volume_end[0]=2000;
    for(unsigned i=0;i<2;i++) {
        due+=32;head=v->head;CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));CHECK(v->head==head+1);
        fh_audio_dsp_voices_service_at(&voices,500,due,1);
    }
    c.volume_start[0]=c.volume_end[0]=2000;
    for(unsigned i=0;i<FH_AUDIO_DSP_COMMANDS;i++) {
        c.env_start=c.env_end=(uint16_t)(20000+i);due+=32;
        CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));
    }
    CHECK(v->head-v->tail==FH_AUDIO_DSP_COMMANDS);head=v->head;due+=32;
    CHECK(!fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due));CHECK(v->head==head);
    c.env_start=c.env_end=12345;
    CHECK(fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due+32)==FH_AUDIO_POOL_BUSY);
    CHECK(v->last_due==due && v->head==head);
    fh_audio_dsp_voices_service_at(&voices,500,due,1);
    CHECK(!fh_audio_dsp_voice_stop_at(&voices,2,id,due+32));
    CHECK(fh_audio_dsp_voice_curve_at(&voices,2,id,&c,due+32)==FH_AUDIO_POOL_BUSY);
    fh_audio_dsp_voices_service_at(&voices,500,due+31,1);CHECK(v->live);
    fh_audio_dsp_voices_service_at(&voices,500,due+32,1);CHECK(!v->live);
    acknowledge(2,id);
}
