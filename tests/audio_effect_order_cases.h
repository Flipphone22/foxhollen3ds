static FhAudioPcmQueue order_before;
static int16_t order_storage[2*8*2048*2];
static FhAudioMap order_map(unsigned a,unsigned b) {
    FhAudioMap m={0};
    CHECK(fh_audio_map_append(&m,(FhAudioSpan){1000,a?71:0,0,80,a},0,80));
    CHECK(fh_audio_map_append(&m,(FhAudioSpan){1080,b?71:0,0,80,b},80,80));return m;
}
static void rejected_order(FhAudioMap *m,int expected) {
    order_before=queue;memcpy(order_storage,storage,sizeof(storage));unsigned old_flushes=flushes;
    CHECK(fh_audio_pcm_queue_mapped(&queue,0,source,160,32000,m)==expected);
    CHECK(!memcmp(&order_before,&queue,sizeof(queue)) && !memcmp(order_storage,storage,sizeof(storage)));
    CHECK(old_flushes==flushes);
}
static void effect_order_cases(void) {
    reset(0);FhAudioPcmChannel *p=&queue.channels[0];p->effect_order_guard=1;
    FhAudioMap m=order_map(1,1);CHECK(!fh_audio_pcm_queue_mapped(&queue,0,source,160,32000,&m));
    CHECK(p->queued_effect_token==1);
    m=order_map(1,2);CHECK(!fh_audio_pcm_queue_mapped(&queue,0,source,160,32000,&m));CHECK(p->queued_effect_token==2);
    m=order_map(2,1);rejected_order(&m,FH_AUDIO_POOL_INVALID);
    m=order_map(3,2);rejected_order(&m,FH_AUDIO_POOL_INVALID);
    m=order_map(0,0);CHECK(!fh_audio_pcm_queue_mapped(&queue,0,source,160,32000,&m));CHECK(p->queued_effect_token==2);
    m=order_map(1,1);rejected_order(&m,FH_AUDIO_POOL_INVALID);
    reset(0);CHECK(p->queued_effect_token==2);rejected_order(&m,FH_AUDIO_POOL_INVALID);
    m=order_map(3,3);flush_fail=1;
    CHECK(fh_audio_pcm_queue_mapped(&queue,0,source,160,32000,&m)==FH_AUDIO_POOL_FLUSH_FAILED);
    CHECK(p->queued_effect_token==2 && p->write_index==0);flush_fail=0;
    for(unsigned i=0;i<8;i++)CHECK(!fh_audio_pcm_queue_mapped(&queue,0,source,160,32000,&m));
    CHECK(p->queued_effect_token==3);m=order_map(4,4);rejected_order(&m,FH_AUDIO_POOL_BUSY);
    reset(0);CHECK(p->queued_effect_token==3);
    /* Opt-in only; ordinary unguarded channel behavior remains unchanged. */
    reset(1);m=order_map(7,6);CHECK(!fh_audio_pcm_queue_mapped(&queue,1,source,160,32000,&m));
    CHECK(!queue.channels[1].effect_order_guard);
}
