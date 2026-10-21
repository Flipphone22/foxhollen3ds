#ifndef FH_AUDIO_DSP_AUX_H
#define FH_AUDIO_DSP_AUX_H
#include <stdint.h>
#include "fh_audio_aux_window.h"

enum { FH_AUDIO_AUX_FRAMES=160, FH_AUDIO_AUX_CHANNELS=4 };
typedef void (*FhAudioAuxProcess)(void *,int32_t samples[4][160]);
/* Token identifies the publication that generated returned AUX INPUT, not the
 * current PCM submission or the older gain ramp's initial value. Zero means
 * provenance unavailable. Caller must retain any referenced effect state. */
typedef void (*FhAudioAuxTaggedProcess)(void *,int32_t samples[4][160],uint32_t token);
typedef void (*FhAudioAuxMappedProcess)(void *,int32_t samples[4][160],uint32_t token,const FhAudioAuxWindow *);
typedef struct {
    FhAudioAuxProcess process[2];
    void *context[2];
    int32_t work[4][160];
    unsigned calls[2],faults;
    FhAudioAuxTaggedProcess tagged_process[2];
    /* SDK-thread-only metadata. No producer/UI may write it concurrently. */
    uint32_t next_token,bank_token[2];
    unsigned bank_frame[2],bank_enabled[2],last_frame;
    FhAudioAuxMappedProcess mapped_process[2];
    FhAudioAuxWindow next_window,bank_window[2];
    /* Runs AFTER both bus callbacks and publication, SDK thread only. */
    void (*after_publish)(void *);
    void *after_publish_context;
} FhAudioDspAux;

/* SDK adjunct for the pinned libctru whose public AUX callbacks are uncalled.
 * Use audio_aux_sdk_patch.py's exact v2.7.0 thread hook. Install before ndspInit, retain the owner,
 * callbacks and contexts through ndspExit; change none concurrently. Enabling
 * buses/volume remains through SDK APIs. No hard-coded DSP addresses, sample
 * allocation or process-cache operations: memory is SDK-mapped DSP shared RAM.
 * The hook executes between master publication and the SDK semaphore.
 * Sleep/cancel guards remain in the original SDK thread. Experimental only;
 * no game effects/state ownership is implied by callback plumbing. */
int fh_audio_dsp_aux_install(FhAudioDspAux *);
/* Optional: call once from the ordinary NDSP callback, before this adjunct's
 * hook, for the upcoming channel publication. Never from the synth producer.
 * Missing call, startup, a previously disabled bus, unexpected bank/counter
 * order or a detected sleep discontinuity:
 * returned token is zero. Tokens propagate by actual SDK banks; do not replace
 * this with a source-generation read or a wall-clock delay. The pinned LLE
 * firmware exercises two-publication return latency, not physical acceptance.
 * Do not reset/destroy effect state on submission: returned data is older. */
void fh_audio_dsp_aux_tag(FhAudioDspAux *,uint32_t token);
/* Same SDK-owner publication, with copied PCM provenance and optional single
 * lookahead frame. Pointer may expire immediately after return. Consumers get
 * the window associated with the returned bank, NULL when unavailable. The
 * scalar tag remains for the initial single-effect consumer; it is NOT a
 * replacement for the source/effect spans in a mixed window. */
void fh_audio_dsp_aux_tag_mapped(FhAudioDspAux *,uint32_t token,const FhAudioAuxWindow *);
/* Clear only AFTER ndspExit. */
void fh_audio_dsp_aux_uninstall(void);
/* SDK thread hook only, after its sleep/cancel/readiness guards; NEVER call
 * from ndspiUpdateChn globally (that also runs during wake initialization). */
void fh_audio_dsp_aux_service(void);
/* SDK-only, after_publish context. Lowest known effect still named by queued
 * mapped banks; zero means no proof. This ALONE does not authorize retirement:
 * caller must enforce nondecreasing effect tokens in every future PCM enqueue,
 * require successful completed processing, and retain converter-owned history.
 * Legacy scalar callbacks are not eligible for this proof. */
uint32_t fh_audio_dsp_aux_effect_floor(const FhAudioDspAux *);
#endif
