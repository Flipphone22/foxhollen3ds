#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "musyx/musyx.h"
#include "fh_audio_effect_routes.h"
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
static unsigned checks,created,destroyed,live,peak,failed_token,source_allocs,source_frees,blocks;
#define CHECK(x) do{checks++;assert(x);}while(0)
typedef struct { _SND_REVSTD_WORK work; FhAudioEffectKey key; } State;
void *salMalloc(size_t n){void *p=calloc(1,n);CHECK(p);source_allocs++;return p;}
void salFree(void *p){if(p)source_frees++;free(p);}
static State *reference(const FhAudioEffectKey *k) {
    State *s=calloc(1,sizeof(*s));CHECK(s);s->key=*k;const float *v=k->parameters;
    CHECK(ReverbSTDCreate(&s->work,v[0],v[1],v[2],v[3],v[4]));return s;
}
static void free_reference(State *s){ReverbSTDFree(&s->work);free(s);}
static void *create(void *ctx,const FhAudioEffectKey *key) {
    CHECK(ctx==&created);if(key->source_token==failed_token)return NULL;
    State *s=reference(key);created++;live++;if(live>peak)peak=live;return s;
}
static void destroy(void *ctx,void *payload) {
    CHECK(ctx==&created && payload && live);destroyed++;live--;free_reference(payload);
}
static FhAudioEffectKey key(unsigned token,float mix) {
    FhAudioEffectKey k={.source_token=token,.parameters={.5f,3,mix,.5f,.01f}};return k;
}
static void output(FhAudioEffectRoutes *p,unsigned token,State *a,State *b,unsigned phase) {
    const FhAudioEffectRoute *r=fh_audio_effect_routes_begin(p,token);CHECK(r && r->token==token);
    State *ref[2]={a,b};
    for(unsigned bus=0;bus<2;bus++) {
        CHECK((r->bus[bus]!=NULL)==(ref[bus]!=NULL));if(!ref[bus])continue;
        s32 x[3][160],y[3][160];
        for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)
            x[c][i]=y[c][i]=(int32_t)((i*731+c*311+phase*193+bus*791)%12000)-6000;
        State *s=r->bus[bus];
        ReverbSTDCallback(x[0],x[1],x[2],&s->work);
        ReverbSTDCallback(y[0],y[1],y[2],&ref[bus]->work);
        CHECK(!memcmp(x,y,sizeof(x)));blocks++;
    }
    CHECK(fh_audio_effect_routes_end(p,token));
}
static void detached_capacity(void) {
    FhAudioEffectRoutes p={0};uint32_t t;
    CHECK(fh_audio_effect_routes_init(&p,create,destroy,&created));
    for(unsigned i=1;i<=4;i++) {
        FhAudioEffectKey keys[2]={key(100+i,.5f),{0}};
        CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==i);
        CHECK(fh_audio_effect_routes_begin(&p,t));CHECK(fh_audio_effect_routes_end(&p,t));
        CHECK(fh_audio_effect_routes_retire_before(&p,t));
        CHECK(fh_audio_effect_routes_collect(&p)==(i!=1));
    }
    CHECK(live==4);
    FhAudioEffectRoutes snapshot=p;unsigned allocs=created,frees=destroyed;
    FhAudioEffectKey keys[2]={key(105,.5f),{0}};
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_BUSY);
    CHECK(!memcmp(&p,&snapshot,sizeof(p)) && created==allocs && destroyed==frees);
    CHECK(fh_audio_effect_routes_source_end(&p,102) && live==3); /* Detached. */
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==5 && live==4);
    CHECK(fh_audio_effect_routes_source_end(&p,104) && live==4); /* Queued route. */
    CHECK(fh_audio_effect_routes_begin(&p,t));CHECK(fh_audio_effect_routes_end(&p,t));
    CHECK(fh_audio_effect_routes_retire_before(&p,t));CHECK(fh_audio_effect_routes_collect(&p)==1 && live==3);
    fh_audio_effect_routes_shutdown(&p);CHECK(!live && created==destroyed);
}
static void ended_shared_source(void) {
    FhAudioEffectRoutes p={0};uint32_t t;
    CHECK(fh_audio_effect_routes_init(&p,create,destroy,&created));
    FhAudioEffectKey keys[2]={key(201,.5f),{0}};
    State *a=reference(&keys[0]);
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==1);
    output(&p,1,a,NULL,200);
    keys[1]=key(202,.7f);State *b=reference(&keys[1]);
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==2);
    CHECK(fh_audio_effect_routes_source_end(&p,201)); /* Both buffered routes need it. */
    output(&p,1,a,NULL,201);output(&p,2,a,b,202);
    CHECK(fh_audio_effect_routes_retire_before(&p,2));
    CHECK(fh_audio_effect_routes_collect(&p)==1 && live==2);
    output(&p,2,a,b,203); /* First route's retirement must not destroy shared A. */
    CHECK(fh_audio_effect_routes_source_end(&p,202));
    keys[0]=(FhAudioEffectKey){0};keys[1]=(FhAudioEffectKey){0};
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==3);
    output(&p,3,NULL,NULL,204);
    CHECK(fh_audio_effect_routes_retire_before(&p,3));
    CHECK(fh_audio_effect_routes_collect(&p)==1 && !live);
    fh_audio_effect_routes_shutdown(&p);free_reference(a);free_reference(b);
}
int main(void) {
    FhAudioEffectRoutes p={0};uint32_t t=0;
    CHECK(!fh_audio_effect_routes_init(NULL,create,destroy,&created));
    CHECK(fh_audio_effect_routes_init(&p,create,destroy,&created));
    CHECK(!fh_audio_effect_routes_init(&p,create,destroy,&created));
    FhAudioEffectKey keys[2]={key(11,.9f),key(12,.6f)};
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==1 && live==2);
    State *a=reference(&keys[0]),*b=reference(&keys[1]);
    void *original_b=p.routes[p.current-1].bus[1];
    for(unsigned i=0;i<16;i++)output(&p,1,a,b,i);
    unsigned allocations=created;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==1 && created==allocations);
    keys[0]=key(13,.9f); /* Same coefficients, new source lifetime: fresh history. */
    State *c=reference(&keys[0]);
    CHECK(fh_audio_effect_routes_source_end(&p,11));
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==2 && live==3);
    CHECK(!fh_audio_effect_routes_retire_before(&p,2)); /* Merely published. */
    CHECK(p.routes[p.current-1].bus[1]==original_b);
    FhAudioEffectRoutes full=p;unsigned destroys=destroyed;allocations=created;
    keys[0]=key(14,.3f);t=777;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_BUSY && t==777);
    CHECK(!memcmp(&p,&full,sizeof(p)) && created==allocations && destroyed==destroys);
    CHECK(!fh_audio_effect_routes_collect(&p));
    for(unsigned i=16;i<24;i++)output(&p,1,a,b,i); /* Producer already published2. */
    output(&p,2,c,b,24);
    CHECK(fh_audio_effect_routes_retire_before(&p,2));
    CHECK(!fh_audio_effect_routes_begin(&p,1));
    CHECK(fh_audio_effect_routes_collect(&p)==1 && live==2 && destroyed==destroys+1);
    CHECK(p.routes[p.current-1].bus[1]==original_b);
    State *d=reference(&keys[0]);
    CHECK(fh_audio_effect_routes_source_end(&p,13));
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==3);
    for(unsigned i=25;i<48;i++)output(&p,2,c,b,i);
    for(unsigned i=48;i<96;i++)output(&p,3,d,b,i);
    CHECK(fh_audio_effect_routes_retire_before(&p,3));
    CHECK(fh_audio_effect_routes_collect(&p)==1 && live==2);
    CHECK(p.routes[p.current-1].bus[1]==original_b);
    /* Invalid immutable identity/parameters leave the live state untouched. */
    full=p;allocations=created;destroys=destroyed;
    keys[1].parameters[2]=.7f;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_INVALID);
    keys[1]=key(12,.6f);keys[1].discard_surround=1;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_INVALID);
    keys[1]=key(12,.6f);keys[1].parameters[0]=NAN;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_INVALID);
    CHECK(!memcmp(&full,&p,sizeof(p)) && created==allocations && destroyed==destroys);
    /* A failure on the second new bus destroys only the unpublished first bus. */
    keys[0]=key(15,.2f);keys[1]=key(16,.4f);failed_token=16;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_INVALID);
    CHECK(!memcmp(&full,&p,sizeof(p)) && live==2 && created==allocations+1 && destroyed==destroys+1);
    output(&p,3,d,b,96);failed_token=0;
    CHECK(fh_audio_effect_routes_source_end(&p,14));
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==4 && live==4);
    State *e=reference(&keys[0]),*f=reference(&keys[1]);
    output(&p,3,d,b,97);output(&p,4,e,f,98);
    CHECK(fh_audio_effect_routes_retire_before(&p,4));
    CHECK(fh_audio_effect_routes_collect(&p)==1 && live==3); /* Detached B stays alive. */
    CHECK(!fh_audio_effect_routes_begin(&p,0) && !fh_audio_effect_routes_begin(&p,99));
    CHECK(!fh_audio_effect_routes_retire_before(&p,99));
    /* Two source buses may share one original effect object; retain once per
     * route edge, process in source bus order, destroy exactly once. */
    keys[0]=keys[1]=key(17,.8f);
    CHECK(fh_audio_effect_routes_source_end(&p,15));CHECK(fh_audio_effect_routes_source_end(&p,16));
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==5 && live==4);
    const FhAudioEffectRoute *r=&p.routes[p.current-1];CHECK(r->bus[0]==r->bus[1]);
    State *g=reference(&keys[0]);output(&p,5,g,g,99);
    CHECK(fh_audio_effect_routes_retire_before(&p,5));CHECK(fh_audio_effect_routes_collect(&p)==1 && live==2);
    keys[0]=(FhAudioEffectKey){0};keys[1]=(FhAudioEffectKey){0};
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==6 && live==2);
    output(&p,6,NULL,NULL,100);
    CHECK(fh_audio_effect_routes_retire_before(&p,6));CHECK(fh_audio_effect_routes_collect(&p)==1 && live==2);
    /* Rebinding a detached live source resumes its actual history, including
     * the bus B which has been detached through several newer route tokens. */
    keys[0]=key(17,.8f);keys[1]=key(12,.6f);allocations=created;
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==7 && created==allocations);
    CHECK(p.routes[p.current-1].bus[1]==original_b);output(&p,7,g,b,101);
    CHECK(fh_audio_effect_routes_retire_before(&p,7));CHECK(fh_audio_effect_routes_collect(&p)==1 && live==2);
    CHECK(fh_audio_effect_routes_source_end(&p,17));CHECK(fh_audio_effect_routes_source_end(&p,12));
    CHECK(live==2); /* Still referenced by the actual playback route. */
    keys[0]=(FhAudioEffectKey){0};keys[1]=(FhAudioEffectKey){0};
    CHECK(fh_audio_effect_routes_prepare(&p,keys,&t)==FH_AUDIO_EFFECT_OK && t==8);
    output(&p,8,NULL,NULL,102);
    CHECK(fh_audio_effect_routes_retire_before(&p,8));CHECK(fh_audio_effect_routes_collect(&p)==1 && !live);
    fh_audio_effect_routes_shutdown(&p);CHECK(created==destroyed && peak==4);
    free_reference(a);free_reference(b);free_reference(c);free_reference(d);
    free_reference(e);free_reference(f);free_reference(g);
    detached_capacity();
    ended_shared_source();
    CHECK(source_allocs==source_frees);
    printf("{\"passed\":true,\"checks\":%u,\"waveform_bus_blocks\":%u,\"created\":%u,\"destroyed\":%u,\"peak_objects\":%u,\"source_allocations\":%u}\n",checks,blocks,created,destroyed,peak,source_allocs);
}
