#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "musyx/musyx.h"
#include "fh_audio_aux_spans.h"
#include "fh_audio_aux_rate.h"
#include "fh_audio_output_clock.h"
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern bool fh_game_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void fh_game_ReverbSTDFree(_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static unsigned checks,applies,split_blocks,lookahead_blocks,blocks;
#define CHECK(x) do{checks++;assert(x);}while(0)
void *salMalloc(size_t n){void *p=calloc(1,n);CHECK(p);return p;}
void salFree(void *p){free(p);}
void *fh_game_reverb_alloc(size_t n){return salMalloc(n);}
void fh_game_reverb_free(void *p){salFree(p);}
typedef struct { _SND_REVSTD_WORK work; unsigned stereo,id; } Effect;
static void *create(void *ctx,const FhAudioEffectKey *k) {
    (void)ctx;Effect *e=calloc(1,sizeof(*e));CHECK(e);e->id=k->source_token;e->stereo=k->discard_surround;
    CHECK(fh_game_ReverbSTDCreate(&e->work,.5f,3,k->parameters[2],.5f,.01f));return e;
}
static void destroy(void *ctx,void *p){(void)ctx;Effect *e=p;fh_game_ReverbSTDFree(&e->work);free(e);}
static int admit(void *ctx,void *p,unsigned bus){(void)bus;return p && (!ctx || ((Effect*)p)->id!=*(unsigned*)ctx);}
static int apply(void *ctx,void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n) {
    (void)ctx;Effect *e=p;applies++;
    return e->stereo?fh_audio_reverb_std_stereo_frames(l,r,n,&e->work):fh_audio_reverb_std_frames(l,r,s,n,&e->work);
}
static FhAudioEffectKey key(unsigned id,unsigned stereo) {
    FhAudioEffectKey k={.source_token=id,.discard_surround=stereo,.parameters={.5f,3,.9f,.5f,.01f}};return k;
}
static FhAudioAuxWindow window(uint64_t first,unsigned n,unsigned phase,unsigned boundary) {
    FhAudioAuxWindow w={.source={.input_first=first,.frames=n,.valid=1},.phase=phase,.queue_epoch=8,.rate=32000,.has_lookahead=1};
    unsigned prefix=first>=boundary?0:boundary-first<n?boundary-first:n;
    if(prefix)w.source.spans[w.source.count++]=(FhAudioSpan){first,91,0,prefix,1};
    if(prefix<n)w.source.spans[w.source.count++]=(FhAudioSpan){first+prefix,92,prefix,n-prefix,2};
    w.lookahead=(FhAudioSpan){first+n,first+n<boundary?91:92,n,1,first+n<boundary?1:2};return w;
}
enum { BLOCKS=128,CAPACITY=20480 };
static int32_t original[2][3][CAPACITY],expected[2][3][CAPACITY];
static unsigned source_at,bus_now;
static void input(unsigned bus,unsigned block,int32_t p[4][160]) {
    for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)p[c][i]=(int32_t)((i*731+c*311+block*193+bus*197)%12000)-6000;
}
static void capture(void *ctx,int32_t *l,int32_t *r,int32_t *s,unsigned n) {
    (void)ctx;CHECK(source_at+n<=CAPACITY);
    memcpy(original[bus_now][0]+source_at,l,n*4);memcpy(original[bus_now][1]+source_at,r,n*4);
    memcpy(original[bus_now][2]+source_at,s,n*4);source_at+=n;
}
static void oracle(void *ctx,int32_t *l,int32_t *r,int32_t *s,unsigned n) {
    (void)ctx;memcpy(l,expected[bus_now][0]+source_at,n*4);memcpy(r,expected[bus_now][1]+source_at,n*4);
    memcpy(s,expected[bus_now][2]+source_at,n*4);source_at+=n;
}
static void waveform(unsigned boundary,unsigned stereo) {
    CHECK(boundary%160==0);
    memset(original,0,sizeof(original));memset(expected,0,sizeof(expected));
    unsigned total=0;
    for(bus_now=0;bus_now<2;bus_now++) {
        FhAudioAuxRate rate={0};source_at=0;
        for(unsigned k=0;k<BLOCKS;k++){int32_t p[4][160];input(bus_now,k,p);CHECK(fh_audio_aux_rate_process(&rate,p,capture,NULL));}
        total=source_at;
    }
    memcpy(expected,original,sizeof(expected));
    _SND_REVSTD_WORK reference[3];
    for(unsigned i=0;i<3;i++)CHECK(ReverbSTDCreate(&reference[i],.5f,3,.9f,.5f,.01f));
    for(unsigned first=0;first<total;first+=160)for(unsigned bus=0;bus<2;bus++) {
        unsigned id=bus?1:first<boundary?0:2;
        /* Original HandleReverb requires three CONTIGUOUS 160-sample planes;
         * its right/surround arguments are ignored. Preserve that actual ABI. */
        int32_t planes[3][160];
        for(unsigned c=0;c<3;c++)memcpy(planes[c],expected[bus][c]+first,sizeof(planes[c]));
        ReverbSTDCallback(planes[0],planes[1],planes[2],&reference[id]);
        for(unsigned c=0;c<3;c++)memcpy(expected[bus][c]+first,planes[c],sizeof(planes[c]));
    }
    FhAudioEffectRoutes routes={0};CHECK(fh_audio_effect_routes_init(&routes,create,destroy,NULL));
    FhAudioEffectKey keys[2]={key(1,stereo),key(2,stereo)};uint32_t token;
    CHECK(!fh_audio_effect_routes_prepare(&routes,keys,&token) && token==1);
    keys[0]=key(3,stereo);CHECK(!fh_audio_effect_routes_prepare(&routes,keys,&token) && token==2);
    CHECK(routes.routes[0].bus[1]==routes.routes[1].bus[1]);
    FhAudioAuxRate rates[2]={0},references[2]={0};FhAudioOutputClock clock={0};
    for(unsigned k=0;k<BLOCKS;k++) {
        unsigned phase=clock.fraction;
        FhAudioOutputWindow w=fh_audio_output_step(&clock,k+1,0,8,32000,16756991,k==0,0,1,UINT64_MAX);CHECK(w.available);
        for(unsigned bus=0;bus<2;bus++) {
            int32_t actual[4][160],ref[4][160];input(bus,k,actual);memcpy(ref,actual,sizeof(ref));
            FhAudioAuxWindow mapped=window(w.first,w.end-w.first,phase,boundary);
            uint64_t first=rates[bus].source_frames;
            source_at=references[bus].source_frames;bus_now=bus;
            CHECK(fh_audio_aux_effect_block(&routes,&rates[bus],&mapped,first,bus,stereo,actual,admit,apply,NULL));
            if(stereo) {
                CHECK(fh_audio_aux_rate_stereo(&references[bus],ref,oracle,NULL));
            } else {
                CHECK(fh_audio_aux_rate_process(&references[bus],ref,oracle,NULL));
            }
            unsigned n=rates[bus].source_frames-first;FhAudioAuxSpans spans;
            CHECK(fh_audio_aux_spans(&mapped,first,n,&spans));
            split_blocks+=spans.count>1;lookahead_blocks+=first+n>w.end;
            CHECK(rates[bus].input_phase==references[bus].input_phase && rates[bus].source_frames==references[bus].source_frames);
            CHECK(!memcmp(actual,ref,sizeof(actual)));blocks++;
            CHECK(!routes.epochs.active_slot);
        }
    }
    CHECK(!fh_audio_effect_routes_collect(&routes)); /* No automatic retirement. */
    fh_audio_effect_routes_shutdown(&routes);
    for(unsigned i=0;i<3;i++)ReverbSTDFree(&reference[i]);
}
static void rejections(void) {
    FhAudioAuxWindow w=window(150,156,0,160);FhAudioAuxSpans p,before;
    memset(&p,0xcc,sizeof(p));before=p;
    w.has_lookahead=0;CHECK(!fh_audio_aux_spans(&w,150,157,&p) && !memcmp(&p,&before,sizeof(p)));
    w.has_lookahead=1;CHECK(fh_audio_aux_spans(&w,151,156,&p));
    CHECK(p.count==2 && p.spans[0].source_frame==151 && p.spans[0].frames==9 && p.spans[1].frames==147);
    before=p;w.lookahead.offset++;CHECK(!fh_audio_aux_spans(&w,151,156,&p) && !memcmp(&p,&before,sizeof(p)));w.lookahead.offset--;
    CHECK(!fh_audio_aux_spans(&w,152,155,&p));CHECK(!fh_audio_aux_spans(&w,149,156,&p));
    FhAudioEffectRoutes owner={0};uint32_t t;CHECK(fh_audio_effect_routes_init(&owner,create,destroy,NULL));
    FhAudioEffectKey k[2]={key(1,0),key(2,0)};CHECK(!fh_audio_effect_routes_prepare(&owner,k,&t));
    int32_t samples[3][160],saved[3][160];memset(samples,0x11,sizeof(samples));memcpy(saved,samples,sizeof(saved));
    unsigned calls=applies;FhAudioEffectRoutes old=owner;
    CHECK(!fh_audio_aux_effect_spans(&owner,&p,0,samples[0],samples[1],samples[2],admit,apply,NULL));
    CHECK(applies==calls && !memcmp(samples,saved,sizeof(samples)) && !memcmp(&old,&owner,sizeof(owner)));
    int32_t dsp[4][160],dsp_before[4][160];memset(dsp,0x11,sizeof(dsp));memcpy(dsp_before,dsp,sizeof(dsp));
    FhAudioAuxRate rate={0},rate_before=rate;FhAudioAuxWindow mixed=window(0,156,0,10);
    CHECK(!fh_audio_aux_effect_block(&owner,&rate,&mixed,0,0,0,dsp,admit,apply,NULL));
    CHECK(applies==calls && !memcmp(dsp,dsp_before,sizeof(dsp)) && !memcmp(&rate,&rate_before,sizeof(rate)));
    CHECK(!memcmp(&old,&owner,sizeof(owner)));
    k[0]=key(3,0);CHECK(!fh_audio_effect_routes_prepare(&owner,k,&t));old=owner;unsigned reject=3;
    CHECK(!fh_audio_aux_effect_spans(&owner,&p,0,samples[0],samples[1],samples[2],admit,apply,&reject));
    CHECK(applies==calls && !memcmp(samples,saved,sizeof(samples)) && !memcmp(&old,&owner,sizeof(owner)));
    mixed.phase=1;CHECK(!fh_audio_aux_effect_block(&owner,&rate,&mixed,0,0,0,dsp,admit,apply,NULL));
    CHECK(applies==calls && !memcmp(dsp,dsp_before,sizeof(dsp)) && !memcmp(&rate,&rate_before,sizeof(rate)));
    mixed.phase=0;
    CHECK(!fh_audio_aux_effect_block(&owner,&rate,&mixed,0,0,0,dsp,admit,apply,&reject));
    CHECK(applies==calls && !memcmp(dsp,dsp_before,sizeof(dsp)) && !memcmp(&rate,&rate_before,sizeof(rate)));
    p.spans[1].generation=0;p.spans[1].effect_token=0;
    CHECK(!fh_audio_aux_effect_spans(&owner,&p,0,samples[0],samples[1],samples[2],admit,apply,NULL));
    CHECK(applies==calls && !memcmp(samples,saved,sizeof(samples)));
    fh_audio_effect_routes_shutdown(&owner);
}
int main(void) {
    rejections();
    const unsigned boundaries[]={160,6400,12800};
    for(unsigned stereo=0;stereo<2;stereo++)for(unsigned i=0;i<3;i++)waveform(boundaries[i],stereo);
    CHECK(split_blocks && lookahead_blocks);
    printf("{\"passed\":true,\"checks\":%u,\"returned_bus_blocks\":%u,\"split_blocks\":%u,\"lookahead_blocks\":%u,\"applications\":%u}\n",checks,blocks,split_blocks,lookahead_blocks,applies);
}
