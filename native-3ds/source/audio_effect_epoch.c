#include "fh_audio_effect_epoch.h"
#include <stddef.h>

enum { FREE=0,PUBLISHED=1,RETIRED=2 };
int fh_audio_effect_publish(FhAudioEffectEpochs *p,uint32_t token,void *payload) {
    if(!p || !payload || !token || token<=p->published_token ||
       token<__atomic_load_n(&p->return_floor,__ATOMIC_ACQUIRE))return FH_AUDIO_EFFECT_INVALID;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SLOTS;i++)
        if(__atomic_load_n(&p->slots[i].state,__ATOMIC_ACQUIRE)!=FREE &&
           p->slots[i].payload==payload)return FH_AUDIO_EFFECT_INVALID;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SLOTS;i++) {
        FhAudioEffectSlot *s=&p->slots[i];
        if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=FREE)continue;
        s->payload=payload;s->token=token;
        __atomic_store_n(&s->state,PUBLISHED,__ATOMIC_RELEASE);
        p->published_token=token;return FH_AUDIO_EFFECT_OK;
    }
    return FH_AUDIO_EFFECT_BUSY;
}
int fh_audio_effect_collect(FhAudioEffectEpochs *p,uint32_t *token,void **payload) {
    if(!p || !token || !payload)return 0;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SLOTS;i++) {
        FhAudioEffectSlot *s=&p->slots[i];
        if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=RETIRED)continue;
        *token=s->token;*payload=s->payload;
        s->payload=NULL;s->token=0;
        __atomic_store_n(&s->state,FREE,__ATOMIC_RELEASE);return 1;
    }
    return 0;
}
void *fh_audio_effect_begin(FhAudioEffectEpochs *p,uint32_t token) {
    if(!p || !token || p->active_slot ||
       token<__atomic_load_n(&p->return_floor,__ATOMIC_RELAXED))return NULL;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SLOTS;i++) {
        FhAudioEffectSlot *s=&p->slots[i];
        if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)!=PUBLISHED || s->token!=token)continue;
        p->active_slot=i+1;return s->payload;
    }
    return NULL;
}
int fh_audio_effect_end(FhAudioEffectEpochs *p,uint32_t token) {
    if(!p || !p->active_slot || p->slots[p->active_slot-1].token!=token)return 0;
    p->active_slot=0;return 1;
}
int fh_audio_effect_retire_before(FhAudioEffectEpochs *p,uint32_t token) {
    if(!p || !token || p->active_slot ||
       token<__atomic_load_n(&p->return_floor,__ATOMIC_RELAXED))return 0;
    __atomic_store_n(&p->return_floor,token,__ATOMIC_RELEASE);
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SLOTS;i++) {
        FhAudioEffectSlot *s=&p->slots[i];
        if(__atomic_load_n(&s->state,__ATOMIC_ACQUIRE)==PUBLISHED && s->token<token)
            __atomic_store_n(&s->state,RETIRED,__ATOMIC_RELEASE);
    }
    return 1;
}
