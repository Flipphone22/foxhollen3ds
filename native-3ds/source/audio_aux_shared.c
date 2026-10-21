#include "fh_audio_aux_shared.h"
typedef struct {
    FhAudioAuxInput *input;
    FhAudioAuxSpans plan;
    unsigned applied;
    FhAudioAuxEffectAdmit admit;
    FhAudioAuxEffectApply apply;
    void *context;
} Shared;
static int admission(void *context,void *effect,unsigned bus) {
    Shared *s=context;return s->admit(s->context,effect,bus);
}
static int application(void *context,void *effect,int32_t *l,int32_t *r,int32_t *a,unsigned frames) {
    Shared *s=context;
    if(s->applied>=s->plan.count)return 0;
    const FhAudioSpan *span=&s->plan.spans[s->applied++];
    if(frames!=span->frames)return 0;
    if(span->generation && fh_audio_aux_input_add(s->input,span->generation,span->effect_token,
        span->source_frame,frames,l,r,a)!=FH_AUX_INPUT_OK)return 0;
    return s->apply(s->context,effect,l,r,a,frames);
}
int fh_audio_aux_shared_process(FhAudioAuxStream *stream,FhAudioEffectRoutes *owner,FhAudioAuxRate *rate,
    const FhAudioAuxWindow *w,unsigned bus,unsigned stereo,FhAudioAuxInput *input,int32_t samples[4][160],
    FhAudioAuxEffectAdmit admit,FhAudioAuxEffectApply apply,void *context) {
    if(!stream || !owner || !rate || !input || !samples || !admit || !apply || bus>=2 || stereo>1 ||
       input->channels!=(stereo?2u:3u))return FH_AUDIO_AUX_INVALID;
    if(!w)return fh_audio_aux_stream_process(stream,owner,rate,w,bus,stereo,samples,admit,apply,context);
    uint64_t origin=stream->initialized?stream->origin:w->source.input_first;
    if(rate->source_frames>UINT64_MAX-origin)return FH_AUDIO_AUX_INVALID;
    uint64_t first=origin+rate->source_frames;
    if(first<w->source.input_first || first-w->source.input_first>1)return FH_AUDIO_AUX_INVALID;
    enum { SOURCE_STEP=32000*512,DSP_STEP=16756991 };
    int64_t phase=(int64_t)(first-w->source.input_first)*DSP_STEP-w->phase;
    if(phase!=rate->input_phase || phase<=-SOURCE_STEP || phase>DSP_STEP-SOURCE_STEP)return FH_AUDIO_AUX_INVALID;
    unsigned frames=(INT64_C(159)*SOURCE_STEP-phase)/DSP_STEP+1;
    Shared s={.input=input,.admit=admit,.apply=apply,.context=context};
    if(!fh_audio_aux_spans(w,first,frames,&s.plan))return FH_AUDIO_AUX_INVALID;
    unsigned ahead=0;
    for(unsigned i=0;i<s.plan.count;i++) {
        const FhAudioSpan *span=&s.plan.spans[i];
        if(!span->generation)continue;
        int status=fh_audio_aux_input_check(input,ahead,span->generation,span->effect_token,
            span->source_frame,span->frames);
        if(status==FH_AUX_INPUT_BUSY)return FH_AUDIO_AUX_INPUT_PENDING;
        if(status!=FH_AUX_INPUT_OK)return FH_AUDIO_AUX_INVALID;
        ahead+=span->frames;
    }
    int result=fh_audio_aux_stream_process(stream,owner,rate,w,bus,stereo,samples,admission,application,&s);
    if(result==FH_AUDIO_AUX_PROCESSED && s.applied!=s.plan.count)return FH_AUDIO_AUX_INVALID;
    return result;
}
