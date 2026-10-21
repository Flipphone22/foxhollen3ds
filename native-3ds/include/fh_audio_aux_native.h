#ifndef FH_AUDIO_AUX_NATIVE_H
#define FH_AUDIO_AUX_NATIVE_H
#include "fh_audio_aux_spans.h"
#include "fh_audio_aux_input.h"
/* Effects run on the DSP sample clock. Source provenance still selects each
 * output sample's route. The effect constructor retunes delay lengths/decay
 * for the device rate; dry voices and simulation clocks are unchanged. */
int fh_audio_aux_native_plan(const FhAudioAuxWindow *,FhAudioAuxSpans *);
int fh_audio_aux_native_process(FhAudioAuxStream *,FhAudioEffectRoutes *,
    const FhAudioAuxWindow *,unsigned bus,unsigned stereo,int32_t samples[4][160],
    FhAudioAuxEffectAdmit,FhAudioAuxEffectApply,void *context);
/* Software sends are read from their exact retained source spans, held to the
 * device grid, and summed before the common effect. Tail is released only after
 * copying; tagged lookahead remains queued for the next block. */
int fh_audio_aux_native_shared(FhAudioAuxStream *,FhAudioEffectRoutes *,
    const FhAudioAuxWindow *,unsigned bus,unsigned stereo,FhAudioAuxInput *,int32_t samples[4][160],
    FhAudioAuxEffectAdmit,FhAudioAuxEffectApply,void *context);
#endif
