#include "fh_audio_effect_routes.h"
#include <string.h>
#include <math.h>

static int same(const FhAudioEffectKey *a,const FhAudioEffectKey *b) {
    return a->source_token==b->source_token && (!a->source_token ||
        (a->discard_surround==b->discard_surround && !memcmp(a->parameters,b->parameters,sizeof(a->parameters))));
}
static int valid(const FhAudioEffectKey *k) {
    if(!k->source_token)return 1;
    if(k->discard_surround>1)return 0;
    for(unsigned i=0;i<5;i++)if(!isfinite(k->parameters[i]))return 0;
    return k->parameters[0]>=0 && k->parameters[0]<=1 &&
        k->parameters[1]>=.01f && k->parameters[1]<=10 &&
        k->parameters[2]>=0 && k->parameters[2]<=1 &&
        k->parameters[3]>=0 && k->parameters[3]<=1 &&
        k->parameters[4]>=0 && k->parameters[4]<=.1f;
}
static void release_object(FhAudioEffectRoutes *p,FhAudioEffectObject *o) {
    p->destroy(p->context,o->payload);memset(o,0,sizeof(*o));
}
static void release_route(FhAudioEffectRoutes *p,FhAudioEffectRoute *r,unsigned unpublished) {
    for(unsigned b=0;b<FH_AUDIO_EFFECT_BUSES;b++)if(r->objects[b]) {
        FhAudioEffectObject *o=&p->objects[r->objects[b]-1];
        if(!--o->references && (o->source_ended || (unpublished && (unpublished&(1u<<(r->objects[b]-1))))))
            release_object(p,o);
    }
    memset(r,0,sizeof(*r));
}
int fh_audio_effect_routes_init(FhAudioEffectRoutes *p,
    void *(*create)(void *,const FhAudioEffectKey *),void (*destroy)(void *,void *),void *context) {
    if(!p || !create || !destroy || p->create)return 0;
    memset(p,0,sizeof(*p));p->create=create;p->destroy=destroy;p->context=context;return 1;
}
unsigned fh_audio_effect_routes_collect(FhAudioEffectRoutes *p) {
    if(!p || !p->create)return 0;
    uint32_t token;void *payload;unsigned count=0;
    while(fh_audio_effect_collect(&p->epochs,&token,&payload)) {
        FhAudioEffectRoute *r=payload;
        if(p->current && r==&p->routes[p->current-1])p->current=0;
        release_route(p,r,0);count++;
    }
    return count;
}
int fh_audio_effect_routes_source_end(FhAudioEffectRoutes *p,uint32_t token) {
    if(!p || !p->create || !token)return 0;
    unsigned found=0;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_OBJECTS;i++) {
        FhAudioEffectObject *o=&p->objects[i];
        if(o->payload && o->key.source_token==token) {
            found=1;o->source_ended=1;
            if(!o->references)release_object(p,o);
        }
    }
    return found;
}
int fh_audio_effect_routes_prepare(FhAudioEffectRoutes *p,const FhAudioEffectKey keys[2],uint32_t *token) {
    if(!p || !p->create || !keys || !token || !valid(&keys[0]) || !valid(&keys[1]))return FH_AUDIO_EFFECT_INVALID;
    /* Repeated source IDs are immutable; a new constructor has a NEW token.
     * Reject inconsistent parameters rather than aliasing incompatible state. */
    for(unsigned b=0;b<2;b++)if(keys[b].source_token) {
        for(unsigned i=0;i<FH_AUDIO_EFFECT_OBJECTS;i++) {
            const FhAudioEffectObject *o=&p->objects[i];
            if(o->payload && o->key.source_token==keys[b].source_token &&
                (o->source_ended || !same(&o->key,&keys[b])))return FH_AUDIO_EFFECT_INVALID;
        }
    }
    if(keys[0].source_token && keys[0].source_token==keys[1].source_token && !same(&keys[0],&keys[1]))return FH_AUDIO_EFFECT_INVALID;
    if(p->current) {
        const FhAudioEffectRoute *r=&p->routes[p->current-1];unsigned equal=1;
        for(unsigned b=0;b<2;b++) {
            FhAudioEffectKey off={0};
            const FhAudioEffectKey *k=r->objects[b]?&p->objects[r->objects[b]-1].key:&off;
            if(!same(k,&keys[b]))equal=0;
        }
        if(equal){*token=r->token;return FH_AUDIO_EFFECT_OK;}
    }
    if(p->epochs.published_token==UINT32_MAX)return FH_AUDIO_EFFECT_INVALID;
    /* Explicit collect keeps BUSY free of allocation/destruction/publication. */
    unsigned index=0;
    for(;index<FH_AUDIO_EFFECT_SLOTS;index++)if(!p->routes[index].token)break;
    if(index==FH_AUDIO_EFFECT_SLOTS)return FH_AUDIO_EFFECT_BUSY;
    unsigned missing=0,free_objects=0;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_OBJECTS;i++)free_objects+=p->objects[i].payload==NULL;
    for(unsigned b=0;b<2;b++)if(keys[b].source_token && (!b || !same(&keys[0],&keys[1]))) {
        unsigned found=0;
        for(unsigned i=0;i<FH_AUDIO_EFFECT_OBJECTS;i++)
            found|=p->objects[i].payload && same(&p->objects[i].key,&keys[b]);
        missing+=!found;
    }
    if(missing>free_objects)return FH_AUDIO_EFFECT_BUSY;
    FhAudioEffectRoute *r=&p->routes[index];
    unsigned created_mask=0;
    for(unsigned b=0;b<2;b++)if(keys[b].source_token) {
        unsigned object=0;
        for(;object<FH_AUDIO_EFFECT_OBJECTS;object++)
            if(p->objects[object].payload && same(&p->objects[object].key,&keys[b]))break;
        if(object==FH_AUDIO_EFFECT_OBJECTS) {
            for(object=0;object<FH_AUDIO_EFFECT_OBJECTS;object++)if(!p->objects[object].payload)break;
            if(object==FH_AUDIO_EFFECT_OBJECTS){release_route(p,r,created_mask);return FH_AUDIO_EFFECT_INVALID;}
            void *payload=p->create(p->context,&keys[b]);
            if(!payload){release_route(p,r,created_mask);return FH_AUDIO_EFFECT_INVALID;}
            p->objects[object].key=keys[b];p->objects[object].payload=payload;
            created_mask|=1u<<object;
        }
        p->objects[object].references++;
        r->objects[b]=object+1;r->bus[b]=p->objects[object].payload;
    }
    r->token=p->epochs.published_token+1;
    int result=fh_audio_effect_publish(&p->epochs,r->token,r);
    if(result!=FH_AUDIO_EFFECT_OK){release_route(p,r,created_mask);return result;}
    p->current=index+1;*token=r->token;return FH_AUDIO_EFFECT_OK;
}
const FhAudioEffectRoute *fh_audio_effect_routes_begin(FhAudioEffectRoutes *p,uint32_t token) {
    return p?fh_audio_effect_begin(&p->epochs,token):NULL;
}
int fh_audio_effect_routes_end(FhAudioEffectRoutes *p,uint32_t token) {
    if(!p || !fh_audio_effect_end(&p->epochs,token))return 0;
    p->completed=token;return 1;
}
int fh_audio_effect_routes_retire_before(FhAudioEffectRoutes *p,uint32_t token) {
    return p && token && token<=p->completed && fh_audio_effect_retire_before(&p->epochs,token);
}
void fh_audio_effect_routes_shutdown(FhAudioEffectRoutes *p) {
    if(!p || !p->create)return;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_OBJECTS;i++)if(p->objects[i].payload)release_object(p,&p->objects[i]);
    memset(p,0,sizeof(*p));
}
