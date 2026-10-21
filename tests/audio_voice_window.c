#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fh_audio_voice_window.h"
#include "fh_audio_aux_window.h"
static unsigned checks;
#define CHECK(x) do {++checks;assert(x);} while(0)

/* Actual game service body, with the SDK/PCM boundary recorded, not emulated. */
typedef struct {unsigned identity;} FhAudioPcmQueue;
static struct {unsigned faults,prefix_faults;} voices;
static struct {unsigned faults;} aux;
static unsigned callback_fault,playback_epoch,callback_windows,callback_gaps;
static uint32_t playback_effect_token;
static FhAudioSourceWindow input;
static FhAudioAuxWindow input_aux,tagged_aux;
static FhAudioPcmQueue queue={17};
static unsigned journal_calls,voice_calls,tag_calls,called_epoch,called_leading,called_available;
static uint64_t called_first,called_end;
static FhAudioSourceWindow fh_audio_pcm_source_window(const FhAudioPcmQueue *q,unsigned c) {
    CHECK(q==&queue && !c);return input;
}
static void mapping_journal(const FhAudioPcmQueue *q,const FhAudioSourceWindow *w) {
    CHECK(q==&queue && !memcmp(w,&input,sizeof(*w)));journal_calls++;
}
static FhAudioAuxWindow fh_audio_pcm_aux_window(const FhAudioPcmQueue *q,unsigned c) {
    CHECK(q==&queue && !c);return input_aux;
}
static void fh_audio_dsp_voices_service_region(void *v,uint64_t epoch,uint64_t first,uint64_t end,unsigned leading,int available) {
    CHECK(v==&voices);voice_calls++;called_epoch=epoch;called_first=first;called_end=end;
    called_leading=leading;called_available=available;
}
static void fh_audio_dsp_aux_tag_mapped(void *a,uint32_t token,const FhAudioAuxWindow *w) {
    CHECK(a==&aux && token==playback_effect_token);tagged_aux=*w;tag_calls++;
}
#include "voice-owner.c"

static void check_case(FhAudioSourceWindow w,int expected,unsigned leading,uint64_t first,unsigned epoch,unsigned effect) {
    FhAudioSpan result,before;memset(&result,0xa5,sizeof(result));before=result;
    FhAudioSourceWindow original=w;
    CHECK(fh_audio_voice_window(&w,&result)==expected);
    CHECK(!memcmp(&w,&original,sizeof(w)));
    if(expected>0) {
        CHECK(result.offset==leading && result.frames==w.frames-leading);
        CHECK(result.source_frame==first && result.generation==epoch && result.effect_token==effect);
    } else CHECK(!memcmp(&result,&before,sizeof(result)));
    input=w;input_aux=(FhAudioAuxWindow){.source=w,.queue_epoch=13,.phase=567,.rate=32000,.has_lookahead=1,
        .lookahead={987654,456,157,1,1024}};
    /* Lookahead has independent identity and must NOT extend the voice block. */
    callback_fault=0;playback_epoch=5;playback_effect_token=7;
    callback_windows=callback_gaps=journal_calls=voice_calls=tag_calls=0;
    memset(&tagged_aux,0,sizeof(tagged_aux));fh_audio_game_dsp_service(&queue);
    CHECK(voice_calls==1 && journal_calls==1 && tag_calls==1);
    CHECK(!memcmp(&tagged_aux,&input_aux,sizeof(input_aux)));
    CHECK(!memcmp(&input,&original,sizeof(input)));
    CHECK(callback_fault==(expected<0?3u:0u));
    if(expected>0) {
        CHECK(callback_windows==1 && !callback_gaps && playback_epoch==epoch && playback_effect_token==effect);
        CHECK(called_epoch==epoch && called_first==first && called_end==first+w.frames-leading);
        CHECK(called_leading==leading && called_available==(effect!=0));
    } else {
        CHECK(!callback_windows && callback_gaps==1 && playback_epoch==5 && playback_effect_token==7);
        CHECK(called_epoch==5 && !called_first && !called_end && !called_leading && !called_available);
    }
}
static void append(FhAudioSourceWindow *w,unsigned n,unsigned generation,uint64_t first,unsigned effect) {
    if(!n)return;
    CHECK(w->count<FH_AUDIO_MAP_SPANS);
    w->spans[w->count++]=(FhAudioSpan){first,generation,w->frames,n,effect};w->frames+=n;
}
int main(void) {
    FhAudioSourceWindow w={0};check_case(w,0,0,0,0,0);
    /* Every placement of a leading movie prefix and two adjacent effect cuts,
     * including exact block edges and tiny spans. No source work is skipped. */
    for(unsigned prefix=0;prefix<=160;prefix++)for(unsigned cut=prefix;cut<=160;cut++) {
        w=(FhAudioSourceWindow){.valid=1};append(&w,prefix,0,0,0);
        append(&w,cut-prefix,23,10000,41);append(&w,160-cut,23,10000+cut-prefix,42);
        check_case(w,prefix==160?0:1,prefix,10000,23,cut==prefix?42:41);
        if(prefix<cut && cut<160) {
            unsigned last=w.count-1;
            FhAudioSourceWindow bad=w;bad.spans[last].generation++;check_case(bad,-1,0,0,0,0);
            bad=w;bad.spans[last].source_frame++;check_case(bad,-1,0,0,0,0);
            bad=w;bad.spans[last].effect_token=40;check_case(bad,-1,0,0,0,0);
            bad=w;bad.spans[last].generation=0;bad.spans[last].effect_token=0;check_case(bad,-1,0,0,0,0);
            bad=w;bad.spans[last].effect_token=0;check_case(bad,-1,0,0,0,0);
        }
    }
    for(unsigned n=1;n<=160;n++) {
        w=(FhAudioSourceWindow){.valid=1};append(&w,n,23,UINT64_MAX-n,0);
        check_case(w,1,0,UINT64_MAX-n,23,0);
        w.spans[0].source_frame++;check_case(w,-1,0,0,0,0);
        w.spans[0].source_frame=0;w.spans[0].offset=1;check_case(w,-1,0,0,0,0);
        w.spans[0].offset=0;w.spans[0].frames=0;check_case(w,-1,0,0,0,0);
    }
    w=(FhAudioSourceWindow){.valid=1};
    for(unsigned i=0;i<4;i++)append(&w,39,12,900+39*i,80+i);
    check_case(w,1,0,900,12,80);
    w.count++;check_case(w,-1,0,0,0,0);w.count=0;check_case(w,-1,0,0,0,0);
    w.count=4;w.frames=157;check_case(w,-1,0,0,0,0);
    w.frames=161;check_case(w,-1,0,0,0,0);w.frames=0;check_case(w,-1,0,0,0,0);
    w.valid=2;check_case(w,-1,0,0,0,0);
    w=(FhAudioSourceWindow){.valid=1};append(&w,156,0,0,1);check_case(w,-1,0,0,0,0);
    FhAudioSpan result;CHECK(fh_audio_voice_window(NULL,&result)<0 && fh_audio_voice_window(&w,NULL)<0);
    printf("{\"passed\":true,\"checks\":%u}\n",checks);return 0;
}
