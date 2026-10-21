#include "fh_audio_timeline.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do {checks++;assert(x);} while(0)
static int16_t samples[6][320];
typedef struct FhAudioPcmQueue { unsigned unused; } FhAudioPcmQueue;
static FhAudioSourceWindow window;
static unsigned source_epoch,playback_epoch,playback_effect_token,callback_fault,callback_windows,callback_gaps;
static unsigned effect_route_token=999,tagged;
static struct {unsigned faults,prefix_faults;} voices;
static struct {unsigned faults;} aux;
static struct {uint64_t epoch,first,end;unsigned leading,available,calls;} served;
static FhAudioSourceWindow fh_audio_pcm_source_window(const FhAudioPcmQueue *q,unsigned c){(void)q;CHECK(c==0);return window;}
static void mapping_journal(const FhAudioPcmQueue *q,const FhAudioSourceWindow *w){(void)q;(void)w;}
static void fh_audio_dsp_aux_tag(void *p,uint32_t token){CHECK(p==&aux);tagged=token;}
static void fh_audio_dsp_voices_service_region(void *p,uint64_t epoch,uint64_t first,uint64_t end,unsigned leading,int available) {
    CHECK(p==&voices);served.epoch=epoch;served.first=first;served.end=end;
    served.leading=leading;served.available=available;served.calls++;
}
#include "service.c"
int main(void) {
    FhAudioTimeline t={0};const int16_t *addresses[6];
    for(unsigned i=0;i<6;i++)addresses[i]=samples[i];
    CHECK(fh_audio_timeline_bind(&t,addresses));
    /* Render -> delayed movie/DMA copy -> source buffer reuse. A copy keeps
     * its own provenance even when the source owner has prepared future audio. */
    CHECK(fh_audio_timeline_stamp_effect(&t,samples[0],160,3200,7,11));
    CHECK(fh_audio_timeline_copy(&t,samples[4],samples[0],160));
    CHECK(fh_audio_timeline_stamp_effect(&t,samples[0],160,3360,7,12));
    CHECK(fh_audio_timeline_read(&t,samples[4],160).effect_token==11);
    CHECK(fh_audio_timeline_read(&t,samples[0],160).effect_token==12);
    FhAudioTimeline before=t;
    CHECK(!fh_audio_timeline_stamp_effect(&t,samples[0],160,0,0,13));
    CHECK(!memcmp(&before,&t,sizeof(t)));
    CHECK(fh_audio_timeline_stamp(&t,samples[0],160,3520,7));
    CHECK(!fh_audio_timeline_read(&t,samples[0],160).effect_token);
    FhAudioMap map={0};
    for(unsigned i=0;i<4;i++)CHECK(fh_audio_map_append(&map,(FhAudioSpan){
        .source_frame=3200+i*160,.generation=7,.frames=160,.effect_token=11+i},i*160,160));
    CHECK(fh_audio_map_valid(&map,640));
    for(unsigned first=0;first<640;first++)for(unsigned n=1;n<=160 && first+n<=640;n++) {
        FhAudioSourceWindow w={0};
        CHECK(fh_audio_source_window_append(&w,&map,10000,640,10000+first,10000+first+n));
        CHECK(fh_audio_source_window_finish(&w,10000+first,10000+first+n));
        unsigned count=0;
        for(unsigned j=0;j<w.count;j++) {
            FhAudioSpan *s=&w.spans[j];CHECK(s->source_frame==3200+first+s->offset);
            CHECK(s->generation==7 && s->effect_token==11+(first+s->offset)/160);
            CHECK(s->effect_token==11+(first+s->offset+s->frames-1)/160);
            count+=s->frames;
        }
        CHECK(count==n);
    }
    /* Adjacent packets coalesce only if source position AND effect identity
     * agree. Same synth generation is not sufficient. */
    FhAudioMap one={0},two={0};FhAudioSourceWindow w={0};
    CHECK(fh_audio_map_append(&one,(FhAudioSpan){.source_frame=0,.generation=7,.frames=160,.effect_token=3},0,160));
    CHECK(fh_audio_map_append(&two,(FhAudioSpan){.source_frame=160,.generation=7,.frames=160,.effect_token=4},0,160));
    CHECK(fh_audio_source_window_append(&w,&one,0,160,150,170));
    CHECK(fh_audio_source_window_append(&w,&two,160,160,150,170));
    CHECK(fh_audio_source_window_finish(&w,150,170));
    CHECK(w.count==2 && w.spans[0].effect_token==3 && w.spans[1].effect_token==4);
    FhAudioPcmQueue q={0};window=w;
    fh_audio_game_dsp_service(&q);CHECK(callback_fault==3 && !served.available);
    callback_fault=0;window=(FhAudioSourceWindow){.frames=156,.count=1,.valid=1,
        .spans={{.source_frame=0,.generation=7,.frames=156,.effect_token=3}}};
    fh_audio_game_dsp_service(&q);CHECK(tagged==3 && served.available);
    source_epoch=100;effect_route_token=999;window=(FhAudioSourceWindow){0};
    fh_audio_game_dsp_service(&q);CHECK(tagged==3 && served.epoch==7 && !served.available);
    window=(FhAudioSourceWindow){.frames=156,.count=1,.valid=1,
        .spans={{.source_frame=160,.generation=7,.frames=156,.effect_token=4}}};
    fh_audio_game_dsp_service(&q);CHECK(tagged==4 && served.available && !callback_fault);
    window.spans[0].generation=8;window.spans[0].effect_token=0;
    fh_audio_game_dsp_service(&q);CHECK(!tagged && !served.available && !callback_fault);
    printf("{\"passed\":true,\"checks\":%u,\"span_bytes_host\":%zu,\"mixed_route_rejected\":true,\"producer_route_not_read\":true}\n",checks,sizeof(FhAudioSpan));
}
