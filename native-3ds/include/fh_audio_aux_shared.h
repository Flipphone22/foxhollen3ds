#ifndef FH_AUDIO_AUX_SHARED_H
#define FH_AUDIO_AUX_SHARED_H
#include "fh_audio_aux_input.h"
#include "fh_audio_aux_spans.h"
enum { FH_AUDIO_AUX_INPUT_PENDING=3 };
/* Experimental one-history consumer. Adds software raw input at its exact
 * source position AFTER DSP rate conversion and BEFORE the original effect.
 * Metadata/availability preflight covers the complete block before any effects
 * or converter mutation. INPUT_PENDING means retry this same retained DSP block;
 * it is not permission to overwrite/skip it. Never waits inside the callback.
 * Arithmetic/effect faults during application are fatal, not transactional.
 * CPU input capture, callback retry scheduling, wet/dry latency and game mode
 * admission remain responsibilities of the future application integration. */
int fh_audio_aux_shared_process(FhAudioAuxStream *,FhAudioEffectRoutes *,FhAudioAuxRate *,
    const FhAudioAuxWindow *,unsigned bus,unsigned stereo,FhAudioAuxInput *,int32_t samples[4][160],
    FhAudioAuxEffectAdmit,FhAudioAuxEffectApply,void *context);
#endif
