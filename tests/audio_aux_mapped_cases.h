static FhAudioAuxWindow mapped_return[2];
static FhAudioAuxWindow mapped_value(unsigned id) {
    FhAudioAuxWindow w;memset(&w,0,sizeof(w));
    w.source.input_first=(uint64_t)id*160;w.source.frames=156;w.source.count=2;w.source.valid=1;
    w.source.spans[0]=(FhAudioSpan){10000+id,21,0,70,id};
    w.source.spans[1]=(FhAudioSpan){20000+id,22,70,86,id+100};
    w.lookahead=(FhAudioSpan){30000+id,23,156,1,id+200};
    w.queue_epoch=id/10+1;w.phase=id*137;w.rate=32000;w.has_lookahead=1;return w;
}
static void mapped(void *context,int32_t samples[4][160],uint32_t token,const FhAudioAuxWindow *window) {
    (void)samples;unsigned bus=(uintptr_t)context;returned[bus]=token;
    memset(&mapped_return[bus],0,sizeof(mapped_return[bus]));
    if(window)mapped_return[bus]=*window;
}
static void mapped_step(unsigned frame,unsigned publish,unsigned expected) {
    ndspFrameId=frame;ndspBufferId=ndspBufferCurId=frame&1;
    if(publish) {
        FhAudioAuxWindow w=mapped_value(publish);
        fh_audio_dsp_aux_tag_mapped(&owner,publish,&w);
        memset(&w,0xcc,sizeof(w)); /* Caller lifetime ends at tag return. */
    }
    returned[0]=returned[1]=~0u;fh_audio_dsp_aux_service();
    for(unsigned bus=0;bus<2;bus++) {
        CHECK(returned[bus]==expected);
        if(expected) {FhAudioAuxWindow wanted=mapped_value(expected);CHECK(!memcmp(&wanted,&mapped_return[bus],sizeof(wanted)));}
        else CHECK(!mapped_return[bus].source.valid);
    }
}
static void mapped_cases(void) {
    memset(&owner,0,sizeof(owner));
    owner.mapped_process[0]=owner.mapped_process[1]=mapped;owner.context[1]=(void *)1;
    CHECK(fh_audio_dsp_aux_install(&owner));
    ndspAuxSetEnable(0,true);ndspAuxSetEnable(1,true);
    for(unsigned i=0;i<100;i++)mapped_step(100+i,1000+i,i<2?0:998+i);
    mapped_step(201,2000,0);mapped_step(202,2001,0);mapped_step(203,2002,2000);
    mapped_step(204,0,2001);mapped_step(205,2004,2002);mapped_step(206,2005,0);
    mapped_step(65534,3000,0);mapped_step(65535,3001,0);mapped_step(2,3002,3000);
    ndspAuxSetEnable(1,false);
    ndspFrameId=3;ndspBufferId=ndspBufferCurId=1;
    FhAudioAuxWindow w=mapped_value(4000);fh_audio_dsp_aux_tag_mapped(&owner,4000,&w);
    returned[1]=~0u;fh_audio_dsp_aux_service();CHECK(returned[0]==3001 && returned[1]==~0u);
    ndspAuxSetEnable(1,true);mapped_step(4,4001,3002);
    ndspFrameId=5;ndspBufferId=ndspBufferCurId=1;
    w=mapped_value(4002);fh_audio_dsp_aux_tag_mapped(&owner,4002,&w);fh_audio_dsp_aux_service();
    CHECK(returned[0]==4000 && mapped_return[0].source.valid);
    CHECK(returned[1]==0 && !mapped_return[1].source.valid);
    mapped_step(6,4003,4001);
    fh_audio_dsp_aux_uninstall();CHECK(fh_audio_dsp_aux_install(&owner));
    mapped_step(7,5000,0);mapped_step(8,5001,0);mapped_step(9,5002,5000);
    fh_audio_dsp_aux_uninstall();
}
