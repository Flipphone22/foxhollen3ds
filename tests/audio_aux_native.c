#include "fh_audio_aux_native.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
static unsigned checks,applies,seed=0x78abcd;
#define CHECK(x) do{checks++;assert(x);}while(0)
static unsigned random_word(void){return seed=seed*1664525u+1013904223u;}
static FhAudioAuxWindow window(uint64_t first,unsigned phase,unsigned split,unsigned unknown) {
    unsigned n=(phase+UINT64_C(160)*32000*512)/16756991;
    FhAudioAuxWindow w={.source={.input_first=first,.frames=n,.valid=1},.phase=phase,.queue_epoch=8,.rate=32000,.has_lookahead=1};
    unsigned at=0;
    while(at<n) {
        unsigned end=at<split && split<n?split:n,token=at<split?(unknown?0:1):2;
        w.source.spans[w.source.count++]=(FhAudioSpan){token?first+at:0,token?90+token:0,at,end-at,token};at=end;
    }
    unsigned token=n<split?(unknown?0:1):2;
    w.lookahead=(FhAudioSpan){token?first+n:0,token?90+token:0,n,1,token};return w;
}
static void *create(void *c,const FhAudioEffectKey *k){(void)c;unsigned *p=malloc(sizeof(*p));*p=k->source_token;return p;}
static void destroy(void *c,void *p){(void)c;free(p);}
static int admit(void *c,void *p,unsigned bus){(void)bus;return !p || !c || *(unsigned*)p!=*(unsigned*)c;}
static int apply(void *c,void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){
    (void)c;applies++;unsigned token=p?*(unsigned*)p:0;
    for(unsigned i=0;i<n;i++){l[i]=token*100;r[i]=token*200;s[i]=token*300;}return 1;
}
static void mapping(void) {
    for(unsigned k=0;k<20000;k++) {
        unsigned phase=random_word()%16756991,split=random_word()%160,unknown=k&1;
        FhAudioAuxWindow w=window(1234567,phase,split,unknown);FhAudioAuxSpans plan;
        CHECK(fh_audio_aux_native_plan(&w,&plan));CHECK(plan.frames==160);
        for(unsigned j=0,part=0;j<160;j++) {
            while(j>=plan.spans[part].offset+plan.spans[part].frames)part++;
            uint64_t source=(phase+(uint64_t)j*32000*512)/16756991;
            unsigned token=source<split?(unknown?0:1):2;
            CHECK(plan.spans[part].effect_token==token);
        }
    }
}
static void routes(unsigned stereo,unsigned bus) {
    FhAudioEffectRoutes owner={0};FhAudioAuxStream stream={0};
    CHECK(fh_audio_effect_routes_init(&owner,create,destroy,NULL));
    uint32_t token;FhAudioEffectKey keys[2]={ {.source_token=1,.parameters={.5f,3,.9f,.5f,.01f}},{.source_token=3,.parameters={.5f,3,.9f,.5f,.01f}} };
    CHECK(!fh_audio_effect_routes_prepare(&owner,keys,&token) && token==1);
    keys[0].source_token=2;keys[1].source_token=4;
    CHECK(!fh_audio_effect_routes_prepare(&owner,keys,&token) && token==2);
    for(unsigned k=0;k<400;k++) {
        unsigned split=random_word()%160,unknown=k&1;
        FhAudioAuxWindow w=window(stream.next_input,stream.next_phase,split,unknown);
        int32_t pcm[4][160];memset(pcm,0xcc,sizeof(pcm));
        CHECK(fh_audio_aux_native_process(&stream,&owner,&w,bus,stereo,pcm,admit,apply,NULL)==FH_AUDIO_AUX_PROCESSED);
        for(unsigned j=0;j<160;j++) {
            uint64_t pos=(w.phase+(uint64_t)j*32000*512)/16756991;
            unsigned id=pos<split?(unknown?0:1):2;if(id)id+=bus*2;
            int32_t surround=stereo?0:(int64_t)(id*300)*23170>>15;
            CHECK(pcm[0][j]==(int32_t)id*100+surround && pcm[1][j]==(int32_t)id*200+surround);
            CHECK(!pcm[2][j] && !pcm[3][j]);
        }
        FhAudioAuxStream before=stream;unsigned count=applies;
        memset(pcm,0,sizeof(pcm));
        CHECK(fh_audio_aux_native_process(&stream,&owner,NULL,bus,stereo,pcm,admit,apply,NULL)==FH_AUDIO_AUX_HELD);
        CHECK(!memcmp(&before,&stream,sizeof(stream)) && count==applies);
        pcm[1][159]=1;
        CHECK(!fh_audio_aux_native_process(&stream,&owner,NULL,bus,stereo,pcm,admit,apply,NULL));
        CHECK(pcm[1][159]==1 && count==applies);
        CHECK(!fh_audio_aux_native_process(&stream,&owner,&w,bus,stereo,pcm,admit,apply,NULL));
        CHECK(!memcmp(&before,&stream,sizeof(stream)) && count==applies);
    }
    FhAudioAuxWindow w=window(stream.next_input,stream.next_phase,1,0);
    int32_t pcm[4][160],before_pcm[4][160];memset(pcm,0x45,sizeof(pcm));memcpy(before_pcm,pcm,sizeof(pcm));
    FhAudioAuxStream before=stream;unsigned reject=bus?4:2,count=applies;
    CHECK(!fh_audio_aux_native_process(&stream,&owner,&w,bus,stereo,pcm,admit,apply,&reject));
    CHECK(!memcmp(pcm,before_pcm,sizeof(pcm)) && !memcmp(&before,&stream,sizeof(stream)) && count==applies);
    w.queue_epoch++;
    CHECK(!fh_audio_aux_native_process(&stream,&owner,&w,bus,stereo,pcm,admit,apply,NULL));w.queue_epoch--;
    CHECK(!fh_audio_aux_native_process(&stream,&owner,&w,bus,!stereo,pcm,admit,apply,NULL));
    CHECK(!memcmp(pcm,before_pcm,sizeof(pcm)) && !memcmp(&before,&stream,sizeof(stream)));
    fh_audio_effect_routes_shutdown(&owner);
}
static void invalid(void) {
    unsigned phase=(16756991-UINT64_C(159)*32000*512%16756991)%16756991;
    FhAudioAuxWindow w=window(0,phase,156,0);FhAudioAuxSpans plan,before;memset(&plan,0xcc,sizeof(plan));before=plan;
    w.has_lookahead=0;CHECK(!fh_audio_aux_native_plan(&w,&plan) && !memcmp(&plan,&before,sizeof(plan)));w.has_lookahead=1;
    w.rate=31000;CHECK(!fh_audio_aux_native_plan(&w,&plan));w.rate=32000;
    w.source.frames++;CHECK(!fh_audio_aux_native_plan(&w,&plan));w.source.frames--;
    w.lookahead.offset++;CHECK(!fh_audio_aux_native_plan(&w,&plan));w.lookahead.offset--;
    w.source.count=FH_AUDIO_MAP_SPANS+1;CHECK(!fh_audio_aux_native_plan(&w,&plan));
    CHECK(!memcmp(&plan,&before,sizeof(plan)));
}
static int identity(void *c,void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){(void)c;(void)p;(void)l;(void)r;(void)s;(void)n;return 1;}
static int32_t raw_value(unsigned at,unsigned c){return (int32_t)((at*17+c*31)%1000)-500;}
static void shared_input(unsigned stereo,unsigned prefix) {
    FhAudioEffectRoutes owner={0};FhAudioAuxStream stream={0};FhAudioAuxInput q={0};
    CHECK(fh_audio_effect_routes_init(&owner,create,destroy,NULL));
    FhAudioEffectKey keys[2]={ {.source_token=1,.parameters={.5f,3,.9f,.5f,.01f}}, {0} };uint32_t token;
    CHECK(!fh_audio_effect_routes_prepare(&owner,keys,&token));keys[0].source_token=2;
    CHECK(!fh_audio_effect_routes_prepare(&owner,keys,&token));
    size_t bytes=fh_audio_aux_input_bytes(stereo?2:3);int32_t *storage=malloc(bytes);
    CHECK(fh_audio_aux_input_init(&q,storage,bytes,stereo?2:3));unsigned produced=prefix;
    for(unsigned k=0;k<400;k++) {
        uint64_t first=stream.next_input;unsigned phase=stream.next_phase;
        unsigned split=first>=3200?0:3200-first;FhAudioAuxWindow w=window(first,phase,split,0);
        if(first<prefix) {
            w=window(first,phase,prefix-first,1);
            for(unsigned i=0;i<w.source.count;i++)if(w.source.spans[i].generation)w.source.spans[i].effect_token=1;
            if(w.lookahead.generation)w.lookahead.effect_token=1;
        }
        for(unsigned i=0;i<w.source.count;i++)if(w.source.spans[i].generation)w.source.spans[i].generation=91;
        if(w.lookahead.generation)w.lookahead.generation=91;
        while(produced<first+160) {
            int32_t source[3][160];for(unsigned c=0;c<3;c++)for(unsigned j=0;j<160;j++)source[c][j]=raw_value(produced+j,c);
            CHECK(!fh_audio_aux_input_push(&q,91,produced<3200?1:2,produced,source[0],source[1],source[2]));produced+=160;
        }
        int32_t pcm[4][160];for(unsigned c=0;c<4;c++)for(unsigned j=0;j<160;j++)pcm[c][j]=c<3?17:0;
        if(k==10) {
            int32_t before_pcm[4][160];memcpy(before_pcm,pcm,sizeof(pcm));FhAudioAuxInput before=q;
            unsigned reject=1;
            CHECK(!fh_audio_aux_native_shared(&stream,&owner,&w,0,stereo,&q,pcm,admit,identity,&reject));
            CHECK(!memcmp(&q,&before,sizeof(q)) && !memcmp(pcm,before_pcm,sizeof(pcm)));
            pcm[0][0]=INT32_MAX;unsigned pos=first+phase/16756991;
            if(raw_value(pos,0)>0){CHECK(!fh_audio_aux_native_shared(&stream,&owner,&w,0,stereo,&q,pcm,admit,identity,NULL));CHECK(!memcmp(&q,&before,sizeof(q)));}
            pcm[0][0]=17;
        }
        CHECK(fh_audio_aux_native_shared(&stream,&owner,&w,0,stereo,&q,pcm,admit,identity,NULL)==FH_AUDIO_AUX_PROCESSED);
        for(unsigned j=0;j<160;j++) {
            unsigned pos=first+(phase+(uint64_t)j*32000*512)/16756991;
            int32_t surround=stereo?0:(int64_t)(17+(pos<prefix?0:raw_value(pos,2)))*23170>>15;
            CHECK(pcm[0][j]==17+(pos<prefix?0:raw_value(pos,0))+surround && pcm[1][j]==17+(pos<prefix?0:raw_value(pos,1))+surround);
            CHECK(!pcm[2][j] && !pcm[3][j]);
        }
        CHECK((uint64_t)q.tail*160+q.read_offset==(stream.next_input>prefix?stream.next_input-prefix:0));
    }
    free(storage);fh_audio_effect_routes_shutdown(&owner);
}
int main(void){mapping();invalid();for(unsigned s=0;s<2;s++){shared_input(s,0);shared_input(s,160);for(unsigned b=0;b<2;b++)routes(s,b);}
    printf("{\"passed\":true,\"checks\":%u,\"applications\":%u}\n",checks,applies);}
