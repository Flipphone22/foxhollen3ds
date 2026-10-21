#include "fh_audio_aux_spans.h"
#include <string.h>

static int valid_span(const FhAudioSpan *s) {
    return s->frames && (s->generation?s->frames<=UINT64_MAX-s->source_frame:!s->effect_token);
}
int fh_audio_aux_spans(const FhAudioAuxWindow *w,uint64_t first,unsigned frames,FhAudioAuxSpans *out) {
    if(!w || !out || !frames || frames>160 || !w->source.valid || !w->source.count ||
       w->source.count>FH_AUDIO_MAP_SPANS || w->source.frames>160 || !w->queue_epoch ||
       w->rate!=32000 || w->phase>=16756991 || frames>UINT64_MAX-first ||
       w->source.frames>=UINT64_MAX-w->source.input_first ||
       first<w->source.input_first || first-w->source.input_first>1)return 0;
    unsigned coverage=0;
    for(unsigned i=0;i<w->source.count;i++) {
        const FhAudioSpan *s=&w->source.spans[i];
        if(!valid_span(s) || s->offset!=coverage || s->frames>w->source.frames-coverage)return 0;
        coverage+=s->frames;
    }
    if(coverage!=w->source.frames || w->has_lookahead>1)return 0;
    if(w->has_lookahead && (!valid_span(&w->lookahead) || w->lookahead.frames!=1 ||
       w->lookahead.offset!=coverage))return 0;
    uint64_t end=first+frames;
    if(end>w->source.input_first+coverage+w->has_lookahead)return 0;
    FhAudioAuxSpans result={0};
    for(unsigned i=0;i<w->source.count+w->has_lookahead;i++) {
        FhAudioSpan s=i<w->source.count?w->source.spans[i]:w->lookahead;
        uint64_t a=w->source.input_first+s.offset,b=a+s.frames;
        uint64_t lo=a>first?a:first,hi=b<end?b:end;
        if(lo>=hi)continue;
        if(s.generation)s.source_frame+=lo-a;else s.source_frame=0;
        s.offset=(uint16_t)(lo-first);s.frames=(uint16_t)(hi-lo);
        if(result.count) {
            FhAudioSpan *prior=&result.spans[result.count-1];
            if(prior->generation==s.generation && prior->effect_token==s.effect_token &&
               (!s.generation || prior->source_frame+prior->frames==s.source_frame)) {
                prior->frames+=s.frames;result.frames+=s.frames;continue;
            }
        }
        result.spans[result.count++]=s;result.frames+=s.frames;
    }
    if(result.frames!=frames)return 0;
    *out=result;return 1;
}
static int admitted(FhAudioEffectRoutes *owner,const FhAudioAuxSpans *plan,unsigned bus,
    FhAudioAuxEffectAdmit admit,void *context) {
    if(!owner || !plan || !plan->count || plan->count>FH_AUDIO_AUX_SPANS || !plan->frames ||
       plan->frames>160 || bus>=2 || !admit || owner->epochs.active_slot)return 0;
    unsigned covered=0;
    for(unsigned i=0;i<plan->count;i++) {
        const FhAudioSpan *span=&plan->spans[i];
        if(!valid_span(span) || (span->generation && !span->effect_token) ||
           span->offset!=covered || span->frames>plan->frames-covered)return 0;
        covered+=span->frames;
    }
    if(covered!=plan->frames)return 0;
    for(unsigned i=0;i<plan->count;i++) {
        uint32_t token=plan->spans[i].effect_token;
        if(!plan->spans[i].generation) {
            if(!admit(context,NULL,bus))return 0;
            continue;
        }
        /* Validation does not publish a completed-route watermark. No payload
         * can be collected until this callback explicitly retires its route. */
        const FhAudioEffectRoute *route=fh_audio_effect_begin(&owner->epochs,token);
        if(!route)return 0;
        int ok=admit(context,route->bus[bus],bus);
        if(!fh_audio_effect_end(&owner->epochs,token) || !ok)return 0;
    }
    return 1;
}
static int apply_spans(FhAudioEffectRoutes *owner,const FhAudioAuxSpans *plan,unsigned bus,
    int32_t *l,int32_t *r,int32_t *s,FhAudioAuxEffectApply apply,void *context) {
    for(unsigned i=0;i<plan->count;i++) {
        const FhAudioSpan *span=&plan->spans[i];
        if(!span->generation) {
            unsigned at=span->offset;
            if(!apply(context,NULL,l+at,r+at,s+at,span->frames))return 0;
            continue;
        }
        const FhAudioEffectRoute *route=fh_audio_effect_routes_begin(owner,span->effect_token);
        if(!route)return 0;
        unsigned at=span->offset;
        int ok=apply(context,route->bus[bus],l+at,r+at,s+at,span->frames);
        if(!fh_audio_effect_routes_end(owner,span->effect_token) || !ok)return 0;
    }
    return 1;
}
int fh_audio_aux_effect_spans(FhAudioEffectRoutes *owner,const FhAudioAuxSpans *plan,unsigned bus,
    int32_t *l,int32_t *r,int32_t *s,FhAudioAuxEffectAdmit admit,FhAudioAuxEffectApply apply,void *context) {
    return l && r && s && apply && admitted(owner,plan,bus,admit,context) &&
        apply_spans(owner,plan,bus,l,r,s,apply,context);
}
typedef struct {
    FhAudioEffectRoutes *owner;
    FhAudioAuxSpans plan;
    unsigned bus,ok;
    FhAudioAuxEffectApply apply;
    void *context;
} Block;
static void converted(void *context,int32_t *l,int32_t *r,int32_t *s,unsigned frames) {
    Block *b=context;
    b->ok=frames==b->plan.frames && apply_spans(b->owner,&b->plan,b->bus,l,r,s,b->apply,b->context);
}
int fh_audio_aux_effect_block(FhAudioEffectRoutes *owner,FhAudioAuxRate *rate,const FhAudioAuxWindow *w,
    uint64_t first,unsigned bus,unsigned stereo,int32_t samples[4][160],
    FhAudioAuxEffectAdmit admit,FhAudioAuxEffectApply apply,void *context) {
    enum { SOURCE_STEP=32000*512,DSP_STEP=16756991 };
    if(!rate || !w || !samples || !apply || stereo>1 || rate->faults ||
       first<w->source.input_first || first-w->source.input_first>1)return 0;
    /* Before DSP sample zero of this block, emitted source frames can lead the
     * nominal clock by one. This identity checks the actual retained phase;
     * it does not reset/seed it to make a discontinuous stream appear aligned. */
    int64_t phase=(int64_t)(first-w->source.input_first)*DSP_STEP-w->phase;
    if(phase!=rate->input_phase || phase<=-SOURCE_STEP || phase>DSP_STEP-SOURCE_STEP ||
       (w->phase+UINT64_C(160)*SOURCE_STEP)/DSP_STEP!=w->source.frames)return 0;
    unsigned frames=(INT64_C(159)*SOURCE_STEP-phase)/DSP_STEP+1;
    Block block={.owner=owner,.bus=bus,.apply=apply,.context=context};
    if(!fh_audio_aux_spans(w,first,frames,&block.plan) || !admitted(owner,&block.plan,bus,admit,context))return 0;
    int ok=stereo?fh_audio_aux_rate_stereo(rate,samples,converted,&block):
        fh_audio_aux_rate_process(rate,samples,converted,&block);
    return ok && block.ok;
}
int fh_audio_aux_stream_process(FhAudioAuxStream *stream,FhAudioEffectRoutes *owner,FhAudioAuxRate *rate,
    const FhAudioAuxWindow *w,unsigned bus,unsigned stereo,int32_t samples[4][160],
    FhAudioAuxEffectAdmit admit,FhAudioAuxEffectApply apply,void *context) {
    if(!stream || !owner || !rate || !samples || !admit || !apply || bus>=2 || stereo>1)return FH_AUDIO_AUX_INVALID;
    if(!w) {
        for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)if(samples[c][i])return FH_AUDIO_AUX_INVALID;
        return FH_AUDIO_AUX_HELD; /* No source clock advance, no reverb decay. */
    }
    FhAudioAuxStream next=*stream;
    if(!next.initialized) {
        if(w->phase || rate->blocks || rate->source_frames || rate->input_phase ||
           rate->output_phase || rate->output_source || rate->initial_silence || rate->faults)return FH_AUDIO_AUX_INVALID;
        next.origin=next.next_input=w->source.input_first;
        next.epoch=w->queue_epoch;next.stereo=stereo;next.initialized=1;
    }
    if(!w->source.valid || next.next_input!=w->source.input_first || next.next_phase!=w->phase ||
       next.epoch!=w->queue_epoch || next.stereo!=stereo || rate->source_frames>UINT64_MAX-next.origin)
        return FH_AUDIO_AUX_INVALID;
    if(!fh_audio_aux_effect_block(owner,rate,w,next.origin+rate->source_frames,bus,stereo,samples,admit,apply,context))
        return FH_AUDIO_AUX_INVALID;
    next.next_input=w->source.input_first+w->source.frames;
    next.next_phase=(w->phase+UINT64_C(160)*32000*512)%16756991;
    *stream=next;return FH_AUDIO_AUX_PROCESSED;
}
