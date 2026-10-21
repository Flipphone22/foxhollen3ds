#ifndef FH_AUDIO_AUX_SPANS_H
#define FH_AUDIO_AUX_SPANS_H
#include "fh_audio_aux_window.h"
#include "fh_audio_effect_routes.h"
#include "fh_audio_aux_rate.h"
enum { FH_AUDIO_AUX_SPANS=FH_AUDIO_MAP_SPANS+1 };
typedef struct {
    FhAudioSpan spans[FH_AUDIO_AUX_SPANS];
    unsigned count,frames;
} FhAudioAuxSpans;
/* Split the ACTUAL converted interval, not the nominal PCM interval. Caller
 * supplies its established PCM-domain first sample. May start one sample ahead
 * and consume the tagged lookahead. Failure leaves output untouched. No clock
 * alignment is guessed here; gaps/startup/reset must be scheduled by caller. */
int fh_audio_aux_spans(const FhAudioAuxWindow *,uint64_t first,unsigned frames,FhAudioAuxSpans *);
typedef int (*FhAudioAuxEffectAdmit)(void *context,void *effect,unsigned bus);
typedef int (*FhAudioAuxEffectApply)(void *context,void *effect,int32_t *,int32_t *,int32_t *,unsigned frames);
/* SDK callback only. Preflight ALL retained route payloads before processing.
 * Unknown source samples use an explicitly admitted NULL payload, never a newer
 * producer route. Caller defines no-handler/unknown behavior through admit/apply.
 * No route retirement: both buses, queued banks and converter history must finish
 * first. Application failure can occur after processing has begun; not rollback.
 * Shared state BETWEEN two buses needs source block ordering outside this API. */
int fh_audio_aux_effect_spans(FhAudioEffectRoutes *,const FhAudioAuxSpans *,unsigned bus,
    int32_t *left,int32_t *right,int32_t *surround,FhAudioAuxEffectAdmit,FhAudioAuxEffectApply,void *context);
/* Complete converted block. Caller establishes queue epoch and absolute sample
 * origin; input phase must agree with the supplied PCM window. stereo is an
 * immutable bus mode, as required by the underlying converter. Mapping/admission
 * failures preserve converter, effects and samples. Never resets the converter.
 * Converter/apply faults after preflight are failures, not transactional rollback. */
int fh_audio_aux_effect_block(FhAudioEffectRoutes *,FhAudioAuxRate *,const FhAudioAuxWindow *,
    uint64_t first,unsigned bus,unsigned stereo,int32_t samples[4][160],
    FhAudioAuxEffectAdmit,FhAudioAuxEffectApply,void *context);
typedef struct {
    uint64_t origin,next_input;
    uint32_t epoch,next_phase,stereo,initialized;
} FhAudioAuxStream;
enum { FH_AUDIO_AUX_INVALID=0,FH_AUDIO_AUX_PROCESSED=1,FH_AUDIO_AUX_HELD=2 };
/* One SDK-owned stream per bus. First valid window must start at phase zero;
 * never invent prior history for a missed startup. Missing windows with silent
 * sends hold converter/effect history; nonzero unmapped sends fail. Unknown
 * spans in VALID windows advance the converter using caller's NULL handler.
 * Queue reset, changed mode, skipped/misaligned windows fail before processing.
 * No automatic restart: the caller must resolve a real reset/source transition. */
int fh_audio_aux_stream_process(FhAudioAuxStream *,FhAudioEffectRoutes *,FhAudioAuxRate *,
    const FhAudioAuxWindow *,unsigned bus,unsigned stereo,int32_t samples[4][160],
    FhAudioAuxEffectAdmit,FhAudioAuxEffectApply,void *context);
#endif
