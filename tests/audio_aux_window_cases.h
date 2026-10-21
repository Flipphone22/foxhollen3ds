/* Invoked after the pinned SDK queue/reset fixture. Synthetic DSP replies;
 * metadata is mutated only to test rejection, never by production callers. */
static FhAudioPcmQueue aux_before;
static FhAudioAuxWindow aux_read(void) {
    aux_before=queue;
    FhAudioAuxWindow a=fh_audio_pcm_aux_window(&queue,0);
    CHECK(!memcmp(&aux_before,&queue,sizeof(queue)));
    if(a.source.valid) {
        CHECK(!memcmp(&a.source,&queue.channels[0].source_window,sizeof(a.source)));
        CHECK(a.rate==32000 && a.queue_epoch==queue.channels[0].active_epoch);
    }
    return a;
}
static void aux_window_cases(void) {
    FhAudioPcmChannel *p=&queue.channels[0];
    CHECK(!fh_audio_pcm_aux_window(NULL,0).source.valid);
    CHECK(!fh_audio_pcm_aux_window(&queue,2).source.valid);
    reset(0);CHECK(!aux_read().source.valid);
    FhAudioMap map={0};
    unsigned lengths[4]={156,320,400,400};
    for(unsigned i=0;i<4;i++) {
        map.count=0;
        CHECK(fh_audio_map_append(&map,(FhAudioSpan){5000+4000*i,71+i,0,lengths[i],3+i},0,lengths[i]));
        CHECK(!fh_audio_pcm_queue_mapped(&queue,0,source,lengths[i],32000,&map));
        memset(&map,0xcd,sizeof(map));
    }
    fh_audio_pcm_service(&queue);ndspiUpdateChn();
    FhAudioAuxWindow a=aux_read();
    CHECK(a.source.valid && a.source.frames==156 && a.phase==0 && a.has_lookahead);
    CHECK(a.source.spans[0].generation==71 && a.source.spans[0].effect_token==3);
    CHECK(a.lookahead.generation==72 && a.lookahead.effect_token==4 && a.lookahead.source_frame==9000);
    CHECK(a.lookahead.offset==156 && a.lookahead.frames==1);
    FhAudioPcmSlot saved=p->slots[1];
    for(unsigned state=0;state<2;state++) {
        p->slots[1].state=state;a=aux_read();
        CHECK(a.source.valid && !a.has_lookahead);
    }
    p->slots[1]=saved;p->slots[1].map.count=0;a=aux_read();
    CHECK(a.source.valid && a.has_lookahead && !a.lookahead.generation && !a.lookahead.effect_token);
    CHECK(a.lookahead.source_frame==0 && a.lookahead.offset==156 && a.lookahead.frames==1);
    p->slots[1]=saved;p->slots[1].map.count=5;CHECK(!aux_read().source.valid);
    p->slots[1]=saved;p->slots[1].map.spans[1]=p->slots[1].map.spans[0];
    p->slots[1].map.count=2;CHECK(!aux_read().source.valid);
    p->slots[1]=saved;p->slots[7]=saved;CHECK(!aux_read().source.valid);
    memset(&p->slots[7],0,sizeof(p->slots[7]));
    /* Keep the real queue storage address intact for later resets/submission. */
    p->slots[7].wave.data_vaddr=storage+7*2048*2;
    p->source_window.frames--;CHECK(!aux_read().source.valid);p->source_window.frames++;
    p->output_clock.valid=0;CHECK(!aux_read().source.valid);p->output_clock.valid=1;
    p->rate=48000;CHECK(!aux_read().source.valid);p->rate=32000;
    CHECK(!fh_audio_pcm_pause(&queue,0,1));fh_audio_pcm_service(&queue);
    CHECK(!aux_read().source.valid);
    CHECK(!fh_audio_pcm_pause(&queue,0,0));fh_audio_pcm_service(&queue);
    a=aux_read();CHECK(a.source.valid && a.has_lookahead);
    CHECK(a.lookahead.source_frame==9156 && a.lookahead.generation==72 && a.lookahead.effect_token==4);
    dropped++;fh_audio_pcm_service(&queue);CHECK(!aux_read().source.valid);
    reset(0);
    for(unsigned i=0;i<4;i++) {
        map.count=0;
        CHECK(fh_audio_map_append(&map,(FhAudioSpan){1000+i*1024,90,0,1024,12},0,1024));
        CHECK(!fh_audio_pcm_queue_mapped(&queue,0,source,1024,32000,&map));
    }
    uint64_t total=0;
    for(unsigned i=0;i<20;i++) {
        unsigned phase=total%SYSCLOCK_SOC;
        fh_audio_pcm_service(&queue);a=aux_read();
        CHECK(a.source.valid && a.phase==phase && a.has_lookahead);
        total+=UINT64_C(160)*32000*512;
        CHECK(a.source.input_first+a.source.frames==total/SYSCLOCK_SOC);
        CHECK(a.lookahead.source_frame==1000+total/SYSCLOCK_SOC);
        CHECK(a.lookahead.generation==90 && a.lookahead.effect_token==12);
    }
    CHECK(!fh_audio_pcm_reset(&queue,0));fh_audio_pcm_service(&queue);
    CHECK(!aux_read().source.valid);
}
