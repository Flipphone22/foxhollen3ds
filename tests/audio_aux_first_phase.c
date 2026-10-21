/* Included after the established native AUX routing test helpers. */
static void first_phase(void) {
    FhAudioEffectRoutes owner={0};uint32_t token;
    CHECK(fh_audio_effect_routes_init(&owner,create,destroy,NULL));
    FhAudioEffectKey keys[2]={ {.source_token=1,.parameters={.5f,3,.9f,.5f,.01f}},
                             {.source_token=3,.parameters={.5f,3,.9f,.5f,.01f}} };
    CHECK(!fh_audio_effect_routes_prepare(&owner,keys,&token)&&token==1);
    keys[0].source_token=2;keys[1].source_token=4;
    CHECK(!fh_audio_effect_routes_prepare(&owner,keys,&token)&&token==2);
    for(unsigned n=0;n<4096;n++) {
        unsigned phase=n==0?0:n==1?1174855:n==2?16756990:random_word()%16756991;
        unsigned split=random_word()%160,unknown=n&1;
        FhAudioAuxWindow w=window(1095,phase,split,unknown);
        FhAudioAuxStream stream={0};int32_t pcm[4][160];memset(pcm,0x39,sizeof pcm);
        CHECK(fh_audio_aux_native_process(&stream,&owner,&w,0,1,pcm,admit,apply,NULL)==FH_AUDIO_AUX_PROCESSED);
        CHECK(stream.origin==1095&&stream.next_input==1095+w.source.frames);
        CHECK(stream.next_phase==(phase+UINT64_C(160)*32000*512)%16756991);
        for(unsigned j=0;j<160;j++) {
            unsigned at=(phase+(uint64_t)j*32000*512)/16756991;
            unsigned id=at<split?(unknown?0:1):2;
            CHECK(pcm[0][j]==(int32_t)id*100&&pcm[1][j]==(int32_t)id*200);
            CHECK(!pcm[2][j]&&!pcm[3][j]);
        }
        FhAudioAuxStream before=stream;int32_t prior[4][160];memcpy(prior,pcm,sizeof pcm);
        unsigned wrong=(stream.next_phase+1)%16756991;
        w=window(stream.next_input,wrong,80,0);
        CHECK(!fh_audio_aux_native_process(&stream,&owner,&w,0,1,pcm,admit,apply,NULL));
        CHECK(!memcmp(&before,&stream,sizeof stream)&&!memcmp(prior,pcm,sizeof pcm));
        w=window(stream.next_input,stream.next_phase,80,0);
        CHECK(fh_audio_aux_native_process(&stream,&owner,&w,0,1,pcm,admit,apply,NULL)==FH_AUDIO_AUX_PROCESSED);
    }
    /* Reproduce the captured first-window metadata, preserving queued future
     * raw input. PCM contents were not captured; use an explicit zero fixture. */
    FhAudioAuxInput input={0};size_t bytes=fh_audio_aux_input_bytes(2);
    int32_t *storage=calloc(1,bytes),zero[160]={0};CHECK(storage);
    CHECK(fh_audio_aux_input_init(&input,storage,bytes,2));
    for(unsigned i=0;i<4;i++)CHECK(fh_audio_aux_input_push(&input,1,1,5120+i*160,zero,zero,NULL)==FH_AUX_INPUT_OK);
    FhAudioAuxInput before_input=input;FhAudioAuxStream stream={0};int32_t pcm[4][160]={{0}};
    FhAudioAuxWindow w=window(1095,1174855,160,1);w.queue_epoch=1;
    CHECK(fh_audio_aux_native_shared(&stream,&owner,&w,0,1,&input,pcm,admit,apply,NULL)==FH_AUDIO_AUX_PROCESSED);
    CHECK(!memcmp(&input,&before_input,sizeof input)&&stream.initialized&&stream.origin==1095);
    free(storage);
    stream=(FhAudioAuxStream){0};FhAudioAuxStream before=stream;
    w=window(1095,0,80,0);w.phase=16756991;
    int32_t prior[4][160];memcpy(prior,pcm,sizeof pcm);
    CHECK(!fh_audio_aux_native_process(&stream,&owner,&w,0,1,pcm,admit,apply,NULL));
    CHECK(!memcmp(&stream,&before,sizeof stream)&&!memcmp(prior,pcm,sizeof pcm));
    fh_audio_effect_routes_shutdown(&owner);
}
