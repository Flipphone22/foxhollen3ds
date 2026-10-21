/* Actual archived stream/rate/raw owners, with an identifiable effect operation.
 * Source-mixed prefix PCM is never passed here: it remains on the dry bus.
 * This checks native contribution and metadata; not DSP waveform fidelity. */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fh_audio_aux_shared.h"
#include "fh_audio_output_clock.h"
#include "fh_audio_voice_window.h"
#include "prefix-owner.inc"
static unsigned checks,blocks,splits,lookaheads;
#define CHECK(x) do {checks++;assert(x);} while(0)
static void *create(void *c,const FhAudioEffectKey *k){(void)c;(void)k;return malloc(1);}
static void destroy(void *c,void *p){(void)c;free(p);}
static int admit(void *c,void *p,unsigned b){(void)c;(void)p;return b==0;}
static int apply(void *c,void *p,int32_t *l,int32_t *r,int32_t *a,unsigned n){
    (void)c;for(unsigned i=0;i<n;i++){l[i]=p?l[i]+17:0;r[i]=p?r[i]-29:0;a[i]=0;}return 1;
}
static int32_t wave(unsigned channel,uint64_t frame){return (int32_t)((frame*73+channel*197)%11000)-5500;}
typedef struct {unsigned boundary;uint64_t at;} Oracle;
static void oracle(void *c,int32_t *l,int32_t *r,int32_t *a,unsigned n){
    Oracle *o=c;for(unsigned i=0;i<n;i++,o->at++){
        l[i]=o->at>=o->boundary?wave(0,o->at)+17:0;
        r[i]=o->at>=o->boundary?wave(1,o->at)-29:0;a[i]=0;
    }
}
static FhAudioAuxWindow window(uint64_t first,unsigned n,unsigned phase,unsigned boundary){
    FhAudioAuxWindow w={.source={.input_first=first,.frames=n,.valid=1},.phase=phase,.queue_epoch=1,.rate=32000,.has_lookahead=1};
    unsigned prefix=first>=boundary?0:boundary-first<n?boundary-first:n;
    if(prefix)w.source.spans[w.source.count++]=(FhAudioSpan){first,1,0,prefix,0};
    if(prefix<n)w.source.spans[w.source.count++]=(FhAudioSpan){first+prefix,1,prefix,n-prefix,1};
    w.lookahead=(FhAudioSpan){first+n,1,n,1,first+n<boundary?0:1};return w;
}
static void run(unsigned boundary){
    FhAudioEffectRoutes routes={0};CHECK(fh_audio_effect_routes_init(&routes,create,destroy,NULL));
    FhAudioEffectKey keys[2]={{.source_token=1,.discard_surround=1,.parameters={.5f,3,.9f,.5f,.01f}},{0}};
    uint32_t token;CHECK(fh_audio_effect_routes_prepare(&routes,keys,&token)==FH_AUDIO_EFFECT_OK && token==1);
    FhAudioAuxRate rate={0},reference={0};FhAudioAuxStream stream={0};FhAudioOutputClock clock={0};FhAudioAuxInput raw={0};
    size_t bytes=fh_audio_aux_input_bytes(2);int32_t *storage=malloc(bytes);CHECK(storage);
    CHECK(fh_audio_aux_input_init(&raw,storage,bytes,2));unsigned produced=boundary;Oracle o={boundary,0};
    for(unsigned k=0;k<96;k++){
        unsigned phase=clock.fraction;
        FhAudioOutputWindow span=fh_audio_output_step(&clock,k+1,0,1,32000,16756991,k==0,0,1,UINT64_MAX);CHECK(span.available);
        FhAudioAuxWindow w=window(span.first,span.end-span.first,phase,boundary),original=w;
        native_aux_window(&w);
        CHECK(w.phase==phase && w.source.input_first==span.first && w.source.frames==original.source.frames);
        CHECK(w.lookahead.offset==original.lookahead.offset && w.lookahead.frames==original.lookahead.frames);
        if(original.lookahead.source_frame<boundary)CHECK(!w.lookahead.generation && !w.lookahead.source_frame && !w.lookahead.effect_token);
        else CHECK(!memcmp(&w.lookahead,&original.lookahead,sizeof(FhAudioSpan)));
        for(unsigned j=0;j<w.source.count;j++){
            CHECK(w.source.spans[j].offset==original.source.spans[j].offset && w.source.spans[j].frames==original.source.spans[j].frames);
            if(original.source.spans[j].source_frame>=boundary)CHECK(!memcmp(&w.source.spans[j],&original.source.spans[j],sizeof(FhAudioSpan)));
        }
        FhAudioSourceWindow voices=original.source;native_source_window(&voices);FhAudioSpan v={0};
        int mapped=fh_audio_voice_window(&voices,&v);
        if(span.end<=boundary)CHECK(mapped==0);
        else {CHECK(mapped==1);CHECK(v.source_frame==(span.first>boundary?span.first:boundary));CHECK(v.frames==span.end-v.source_frame);}
        while(produced<rate.source_frames+160){
            int32_t l[160],r[160];for(unsigned i=0;i<160;i++){l[i]=wave(0,produced+i);r[i]=wave(1,produced+i);}
            CHECK(fh_audio_aux_input_push(&raw,1,1,produced,l,r,NULL)==FH_AUX_INPUT_OK);produced+=160;
        }
        int32_t actual[4][160]={0},expected[4][160]={0};
        CHECK(fh_audio_aux_shared_process(&stream,&routes,&rate,&w,0,1,&raw,actual,admit,apply,NULL)==FH_AUDIO_AUX_PROCESSED);
        CHECK(fh_audio_aux_rate_stereo(&reference,expected,oracle,&o));
        CHECK(!memcmp(actual,expected,sizeof(actual)));
        CHECK(rate.source_frames==reference.source_frames && rate.input_phase==reference.input_phase);
        if(rate.source_frames<=boundary)CHECK(raw.tail==0 && raw.read_offset==0);
        else CHECK((uint64_t)raw.tail*160+raw.read_offset==rate.source_frames-boundary);
        splits+=original.source.count>1;lookaheads+=original.lookahead.source_frame<boundary;blocks++;
    }
    free(storage);fh_audio_effect_routes_shutdown(&routes);
}
int main(void){
    /* Captured New startup: admission at960; also every160-frame boundary
     * through5120 to vary returned-DSP block and lookahead alignment. */
    for(unsigned b=160;b<=5120;b+=160)run(b);
    CHECK(splits && lookaheads);
    printf("{\"passed\":true,\"checks\":%u,\"blocks\":%u,\"split_blocks\":%u,\"prefix_lookaheads\":%u}\n",checks,blocks,splits,lookaheads);
}
