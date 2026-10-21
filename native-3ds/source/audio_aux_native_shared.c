#include "fh_audio_aux_native.h"
#include <limits.h>
#include <string.h>
/* The SDK executes AUX callbacks serially on its sole audio thread. Keep the
 * copied raw block off that thread's 4 KiB stack. No pointer escapes this call;
 * both buses reuse it only after the preceding callback has returned. */
static int32_t raw_scratch[3][160];
typedef struct {
    FhAudioAuxInput *input;
    const FhAudioAuxWindow *window;
    int32_t (*raw)[160],(*samples)[160];
    unsigned consume,committed;
    FhAudioAuxEffectAdmit admit;
    FhAudioAuxEffectApply apply;
    void *context;
} Shared;
static int prepare_mix(Shared *s) {
    /* Expand backwards in the private scratch: destination i is never below
     * source at, so an expanded output cannot overwrite an unread raw sample.
     * Admission failure changes only scratch, never caller PCM or queue tail. */
    uint64_t end=s->window->phase+UINT64_C(159)*32000*512;
    unsigned at=end/16756991;int phase=end%16756991;
    for(int i=159;i>=0;i--) {
        for(unsigned c=0;c<s->input->channels;c++) {
            int32_t sum;
            if(__builtin_add_overflow(s->samples[c][i],s->raw[c][at],&sum))return 0;
            s->raw[c][i]=sum;
        }
        if(i){phase-=32000*512;if(phase<0){phase+=16756991;at--;}}
    }
    return 1;
}
static int admission(void *context,void *effect,unsigned bus){Shared *s=context;return s->admit(s->context,effect,bus);}
static int application(void *context,void *effect,int32_t *l,int32_t *r,int32_t *a,unsigned frames) {
    Shared *s=context;
    if(!s->committed) {
        for(unsigned c=0;c<s->input->channels;c++)memcpy(s->samples[c],s->raw[c],160*sizeof(int32_t));
        unsigned end=s->input->read_offset+s->consume;
        s->input->read_offset=end%FH_AUX_INPUT_FRAMES;
        __atomic_store_n(&s->input->tail,s->input->tail+end/FH_AUX_INPUT_FRAMES,__ATOMIC_RELEASE);
        s->committed=1;
    }
    return s->apply(s->context,effect,l,r,a,frames);
}
int fh_audio_aux_native_shared(FhAudioAuxStream *stream,FhAudioEffectRoutes *owner,
    const FhAudioAuxWindow *w,unsigned bus,unsigned stereo,FhAudioAuxInput *input,int32_t samples[4][160],
    FhAudioAuxEffectAdmit admit,FhAudioAuxEffectApply apply,void *context) {
    if(!stream || !owner || !input || !samples || !admit || !apply || bus>=2 || stereo>1 ||
       input->channels!=(stereo?2u:3u))return FH_AUDIO_AUX_INVALID;
    if(!w)return fh_audio_aux_native_process(stream,owner,w,bus,stereo,samples,admit,apply,context);
    FhAudioAuxSpans device,source;
    if(!fh_audio_aux_native_plan(w,&device))return FH_AUDIO_AUX_INVALID;
    unsigned needed=(w->phase+UINT64_C(159)*32000*512)/16756991+1;
    if(!fh_audio_aux_spans(w,w->source.input_first,needed,&source))return FH_AUDIO_AUX_INVALID;
    int32_t (*raw)[160]=raw_scratch;memset(raw,0,sizeof(raw_scratch));
    Shared shared={.input=input,.window=w,.raw=raw,.samples=samples,.admit=admit,.apply=apply,.context=context};
    unsigned ahead=0;
    /* Only this consumer writes tail/read_offset. The producer cannot overwrite
     * retained slots until the release below. Each range is acquired/validated
     * by the existing ring owner before these CPU-only reads. */
    for(unsigned i=0;i<source.count;i++) {
        const FhAudioSpan *span=&source.spans[i];if(!span->generation)continue;
        int status=fh_audio_aux_input_check(input,ahead,span->generation,span->effect_token,span->source_frame,span->frames);
        if(status!=FH_AUX_INPUT_OK)return FH_AUDIO_AUX_INVALID;
        unsigned offset=input->read_offset+ahead;
        /* The ring owner has validated the complete range. Copy contiguous
         * portions, splitting only at an input block boundary. The acquired
         * slots stay retained until application() commits the consumer tail. */
        unsigned serial=input->tail+offset/FH_AUX_INPUT_FRAMES;
        unsigned within=offset%FH_AUX_INPUT_FRAMES;
        for(unsigned j=0;j<span->frames;) {
            unsigned take=FH_AUX_INPUT_FRAMES-within;
            if(take>span->frames-j)take=span->frames-j;
            const int32_t *block=input->samples+(serial%FH_AUX_INPUT_SLOTS)*input->channels*FH_AUX_INPUT_FRAMES;
            for(unsigned c=0;c<input->channels;c++)
                memcpy(raw[c]+span->offset+j,block+c*FH_AUX_INPUT_FRAMES+within,take*sizeof(int32_t));
            j+=take;serial++;within=0;
        }
        ahead+=span->frames;
        if(span->offset<w->source.frames) {
            unsigned take=w->source.frames-span->offset;if(take>span->frames)take=span->frames;
            shared.consume+=take;
        }
    }
    if(!prepare_mix(&shared))return FH_AUDIO_AUX_INVALID;
    return fh_audio_aux_native_process(stream,owner,w,bus,stereo,samples,admission,application,&shared);
}
