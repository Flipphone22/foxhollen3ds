#include "fh_audio_sample_registry.h"
#include <string.h>

FhAudioSampleRegistry *fh_audio_native_samples;
static int overlaps(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y || (x<y+nb && y<x+na);
}
static int same(const SAMPLE_INFO *a,const SAMPLE_INFO *b) {
    return a->info==b->info && a->addr==b->addr && a->extraData==b->extraData &&
           a->offset==b->offset && a->length==b->length && a->loop==b->loop &&
           a->loopLength==b->loopLength && a->compType==b->compType;
}
int fh_audio_sample_registry_init(FhAudioSampleRegistry *r,FhAudioSamplePool *pool,FhAudioPoolFlush flush,void *context) {
    if(!r || !pool || !pool->arena || !flush ||
       overlaps(r,sizeof(*r),pool,sizeof(*pool)) ||
       overlaps(r,sizeof(*r),pool->arena,pool->capacity))return FH_AUDIO_POOL_INVALID;
    if(r->pool)return FH_AUDIO_POOL_BUSY;
    memset(r,0,sizeof(*r));r->pool=pool;r->flush=flush;r->flush_context=context;
    return FH_AUDIO_POOL_OK;
}
static int remove_entry(FhAudioSampleRegistry *r,FhAudioSampleRegistration *e) {
    int status=fh_audio_pool_bank_close(r->pool,e->bank);
    if(status){r->faults++;return status;}
    memset(e,0,sizeof(*e));return FH_AUDIO_POOL_OK;
}
int fh_audio_sample_registry_forget(FhAudioSampleRegistry *r,const void *key) {
    if(!r || !r->pool || !key)return FH_AUDIO_POOL_INVALID;
    for(unsigned i=0;i<FH_AUDIO_REGISTRY_ENTRIES;i++)if(r->entries[i].key==key) {
        int status=remove_entry(r,&r->entries[i]);if(!status)r->forgotten++;return status;
    }
    return FH_AUDIO_POOL_OK; /* Source may never have been admitted. */
}
int fh_audio_sample_registry_reset(FhAudioSampleRegistry *r) {
    if(!r || !r->pool)return FH_AUDIO_POOL_INVALID;
    for(unsigned i=0;i<FH_AUDIO_REGISTRY_ENTRIES;i++)if(r->entries[i].key) {
        int status=remove_entry(r,&r->entries[i]);if(status)return status;
    }
    return FH_AUDIO_POOL_OK;
}
int fh_audio_sample_registry_prepare(FhAudioSampleRegistry *r,const void *key,const SAMPLE_INFO *sample) {
    if(!r || !r->pool || !key || !sample)return FH_AUDIO_POOL_INVALID;
    FhAudioSampleRegistration *e=NULL,*free_slot=NULL,*victim=NULL;
    for(unsigned i=0;i<FH_AUDIO_REGISTRY_ENTRIES;i++) {
        FhAudioSampleRegistration *at=&r->entries[i];
        if(at->key==key)e=at;
        if(!at->key && !free_slot)free_slot=at;
        if(at->key && (!victim || at->use<victim->use))victim=at;
    }
    if(e) {
        /* Metadata changes while registered violate the immutable source
         * lifetime contract. Retire old copies, don't guess that they match. */
        if(!same(sample,&e->sample)) {
            int status=remove_entry(r,e);r->faults++;
            return status?status:FH_AUDIO_POOL_INVALID;
        }
        int status=fh_audio_pool_publish(r->pool,e->entry,r->flush,r->flush_context);
        if(!status){e->use=++r->clock;r->hits++;return FH_AUDIO_POOL_OK;}
        /* Pool eviction may have removed this unpinned compiled copy. Its bank
         * remains open; recompile from the still-live canonical source record. */
        if(status!=FH_AUDIO_POOL_INVALID){r->declined++;return status;}
    }
    if(sample->compType!=SAMPLE_TYPE_ADPCM || !sample->addr || !sample->length ||
       sample->loop>sample->length || sample->loopLength>sample->length-sample->loop ||
       (sample->loopLength && !sample->extraData)) {
        r->declined++;return FH_AUDIO_POOL_INVALID;
    }
    const DSPADPCMplusInfo *adpcm=sample->extraData;
    FhAudioSampleInput input={.encoded=sample->addr,
        .encoded_bytes=((uint64_t)sample->length+13)/14*8,
        .length=sample->length,.loop_start=sample->loop,.loop_length=sample->loopLength,
        .initial_ps=adpcm?adpcm->initialPS:*(const uint8_t *)sample->addr,
        .loop_ps=adpcm?adpcm->loopPS:0,.loop_y1=adpcm?adpcm->loopY1:0,.loop_y2=adpcm?adpcm->loopY0:0};
    if(adpcm)memcpy(input.coefficients,adpcm->coefTab,sizeof(input.coefficients));
    if(!e) {
        e=free_slot?free_slot:victim;
        if(e->key){int status=remove_entry(r,e);if(status)return status;r->evicted++;}
        FhAudioPoolToken bank;int status=fh_audio_pool_bank_open(r->pool,&bank);
        if(status){r->declined++;return status;}
        *e=(FhAudioSampleRegistration){.key=key,.sample=*sample,.bank=bank,.use=++r->clock};
    }
    int status=fh_audio_pool_prepare(r->pool,e->bank,0,&input,&e->entry);
    if(!status)status=fh_audio_pool_publish(r->pool,e->entry,r->flush,r->flush_context);
    if(status) {
        int cleanup=remove_entry(r,e);r->declined++;return cleanup?cleanup:status;
    }
    e->use=++r->clock;r->prepared++;return FH_AUDIO_POOL_OK;
}
int fh_audio_sample_registry_find(FhAudioSampleRegistry *r,const SAMPLE_INFO *sample,FhAudioPoolToken *bank) {
    if(!r || !r->pool || !sample || !bank)return FH_AUDIO_POOL_INVALID;
    for(unsigned i=0;i<FH_AUDIO_REGISTRY_ENTRIES;i++) {
        FhAudioSampleRegistration *e=&r->entries[i];
        if(e->key && same(&e->sample,sample)) {
            int status=fh_audio_pool_publish(r->pool,e->entry,r->flush,r->flush_context);
            if(status)return status;
            e->use=++r->clock;*bank=e->bank;return FH_AUDIO_POOL_OK;
        }
    }
    return FH_AUDIO_POOL_INVALID;
}
