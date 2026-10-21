#include "fh_audio_aux_native.h"
#include <string.h>
#include <limits.h>

enum { SOURCE_STEP=32000*512,DSP_STEP=16756991 };
/* First DSP sample whose source position reaches this boundary. All products
 * fit u32: source windows are at most160 samples. No per-sample division. */
static unsigned boundary(unsigned source,unsigned phase) {
    unsigned numerator=source*DSP_STEP;
    if(numerator<=phase)return 0;
    unsigned result=(numerator-phase+SOURCE_STEP-1)/SOURCE_STEP;
    return result>160?160:result;
}
int fh_audio_aux_native_plan(const FhAudioAuxWindow *w,FhAudioAuxSpans *out) {
    if(!w || !out || w->phase>=DSP_STEP ||
       (w->phase+UINT64_C(160)*SOURCE_STEP)/DSP_STEP!=w->source.frames)return 0;
    /* Include the source sample covering the LAST device sample, which can
     * require tagged lookahead. Never guess the current producer's route. */
    unsigned needed=(w->phase+UINT64_C(159)*SOURCE_STEP)/DSP_STEP+1;
    FhAudioAuxSpans source;
    if(!fh_audio_aux_spans(w,w->source.input_first,needed,&source))return 0;
    FhAudioAuxSpans device={0};
    for(unsigned i=0;i<source.count;i++) {
        FhAudioSpan s=source.spans[i];
        unsigned lo=boundary(s.offset,w->phase),hi=boundary(s.offset+s.frames,w->phase);
        if(hi<=lo)continue;
        if(lo!=device.frames)return 0;
        /* This internal dispatch plan uses DEVICE sample offsets/counts.
         * Source frame remains descriptive only; no source clock advances. */
        s.offset=lo;s.frames=hi-lo;
        if(s.generation && s.frames>UINT64_MAX-s.source_frame)return 0;
        device.spans[device.count++]=s;device.frames=hi;
    }
    if(device.frames!=160)return 0;
    *out=device;return 1;
}
int fh_audio_aux_native_process(FhAudioAuxStream *stream,FhAudioEffectRoutes *owner,
    const FhAudioAuxWindow *w,unsigned bus,unsigned stereo,int32_t samples[4][160],
    FhAudioAuxEffectAdmit admit,FhAudioAuxEffectApply apply,void *context) {
    if(!stream || !owner || !samples || !admit || !apply || bus>=2 || stereo>1)return FH_AUDIO_AUX_INVALID;
    if(!w) {
        for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)if(samples[c][i])return FH_AUDIO_AUX_INVALID;
        return FH_AUDIO_AUX_HELD;
    }
    FhAudioAuxStream next=*stream;
    if(!next.initialized) {
        /* This owner runs at the device rate, with no source-rate resampler
         * history to prime. Its first returned block can begin at any valid
         * stamped source phase. Admit that anchor once; subsequent blocks
         * must still match the exact source position, phase, epoch and mode. */
        next.origin=next.next_input=w->source.input_first;
        next.next_phase=w->phase;
        next.epoch=w->queue_epoch;next.stereo=stereo;next.initialized=1;
    }
    if(next.next_input!=w->source.input_first || next.next_phase!=w->phase ||
       next.epoch!=w->queue_epoch || next.stereo!=stereo)return FH_AUDIO_AUX_INVALID;
    FhAudioAuxSpans plan;
    if(!fh_audio_aux_native_plan(w,&plan) ||
       !fh_audio_aux_effect_spans(owner,&plan,bus,samples[0],samples[1],samples[2],admit,apply,context))
        return FH_AUDIO_AUX_INVALID;
    if(!stereo)for(unsigned i=0;i<160;i++) {
        int64_t surround=(int64_t)samples[2][i]*23170>>15;
        for(unsigned c=0;c<2;c++) {
            int64_t sum=(int64_t)samples[c][i]+surround;
            samples[c][i]=sum>INT32_MAX?INT32_MAX:sum<INT32_MIN?INT32_MIN:(int32_t)sum;
        }
    }
    memset(samples[2],0,2*160*sizeof(int32_t));
    next.next_input=w->source.input_first+w->source.frames;
    next.next_phase=(w->phase+UINT64_C(160)*SOURCE_STEP)%DSP_STEP;
    *stream=next;return FH_AUDIO_AUX_PROCESSED;
}
