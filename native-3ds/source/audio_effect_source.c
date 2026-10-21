#include "fh_audio_effect_source.h"
#include <stddef.h>
#include <string.h>
#include <math.h>

int fh_audio_effect_source_create(FhAudioEffectSources *p,const void *address,const float v[5]) {
    if(!p || !address || !v || p->serial==UINT32_MAX)return 0;
    for(unsigned i=0;i<5;i++)if(!isfinite(v[i]))return 0;
    if(v[0]<0 || v[0]>1 || v[1]<.01f || v[1]>10 ||
       v[2]<0 || v[2]>1 || v[3]<0 || v[3]>1 || v[4]<0 || v[4]>.1f)return 0;
    unsigned index=FH_AUDIO_EFFECT_SOURCE_SLOTS;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SOURCE_SLOTS;i++) {
        if(p->slots[i].address==address){index=i;break;}
        if(!p->slots[i].address && index==FH_AUDIO_EFFECT_SOURCE_SLOTS)index=i;
    }
    if(index==FH_AUDIO_EFFECT_SOURCE_SLOTS)return 0;
    FhAudioEffectSource *s=&p->slots[index];
    s->address=address;s->token=++p->serial;memcpy(s->parameters,v,sizeof(s->parameters));
    return 1;
}
int fh_audio_effect_source_destroy(FhAudioEffectSources *p,const void *address) {
    if(!p || !address)return 0;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SOURCE_SLOTS;i++)if(p->slots[i].address==address) {
        memset(&p->slots[i],0,sizeof(p->slots[i]));return 1;
    }
    return 0;
}
int fh_audio_effect_source_read(const FhAudioEffectSources *p,const void *address,FhAudioEffectSource *out) {
    if(!p || !address || !out)return 0;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SOURCE_SLOTS;i++)if(p->slots[i].address==address) {
        *out=p->slots[i];return 1;
    }
    return 0;
}
