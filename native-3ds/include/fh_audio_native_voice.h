#ifndef FH_AUDIO_NATIVE_VOICE_H
#define FH_AUDIO_NATIVE_VOICE_H
#include "fh_audio_dsp_voice.h"
#include "fh_audio_sample_registry.h"
#include "fh_audio_source_bridge.h"

typedef struct {
    FhAudioPoolToken token;
    unsigned source_index,studio;
    FhAudioDspControls controls;
} FhAudioNativeVoice;
typedef struct {
    FhAudioSourceBridge bridge;
    FhAudioDspVoices *dsp;
    FhAudioSampleRegistry *registry;
    FhAudioNativeVoice voices[FH_AUDIO_DSP_CHANNELS];
    uint64_t epoch;
    unsigned initialized,experimental_direct_stereo;
    unsigned admitted,declined,segments,stops,faults,peak_pending;
    unsigned aux_bound,aux_studio;
    uint64_t aux_epoch;
    SND_AUX_CALLBACK aux_handlers[2];
    void *aux_users[2];
} FhAudioNativeVoices;

/* Experimental SOURCE->DSP integration, not enabled in ordinary game builds.
 * The explicit experimental flag admits only direct master-studio stereo with
 * no ITD/filter/AUX/rear/surround. Those families still need a native consumer;
 * they are not silently discarded. A change to an unsupported route after
 * admission is a fault, not a restart from stale software history.
 * Gain endpoints are sent at source 1ms boundaries, but the lower NDSP owner
 * applies them at DSP boundaries. This is NOT accepted envelope/depop parity.
 * Combining DSP voices with already-clipped software PCM also needs a clipping
 * comparison; the isolated unclipped tone does not establish mixed-game parity.
 * No production admission until scheduling, effects and backpressure are complete.
 * Experimental value 2 supplies compact source curves for the DSP-window
 * service API. Value 1 retains the old endpoint-only diagnostic comparison.
 * Value3 additionally permits one explicitly bound master studio's main S and
 * AUX A/B sends. The caller must have prepared corresponding native effects,
 * owned through NDSP exit, with serialized source-parameter updates. Binding
 * checks studio/handler/user identity, NOT mutations inside the user object.
 * No ITD/rear/filter/studio-input support or game admission is implied.
 * Value4 implements the original stereo DPL2 studio: ignore main S, fold
 * AUX B L/R directly into main L/R, ignore AUX B effects and return S. Its
 * AUX A owner must discard returned S instead of applying STD stereo downmix.
 * Producer uses the original synth lock. Caller must reserve capacity BEFORE
 * source control advances; available() conservatively reserves seven events
 * per live voice for one 5ms cycle. Callback never accesses this source owner.
 */
int fh_audio_native_voices_init(FhAudioNativeVoices *,FhAudioDspVoices *,
    FhAudioSampleRegistry *,uint64_t source_epoch,unsigned experimental_direct_stereo);
int fh_audio_native_voices_available(const FhAudioNativeVoices *);
int fh_audio_native_voices_poll(FhAudioNativeVoices *);
/* Synth owner, after the original source really resets its output/control state.
 * Advance monotonically, clear only source-side mappings, and invalidate AUX
 * admission. Buffered old DSP commands/descriptors/leases remain untouched until
 * their output generation is played/retired by the callback. New starts can use
 * free channels or remain in software from their first sample; never resume a
 * previously admitted voice from stale software history. Effects need a separate
 * generation-safe handoff before AUX may be rebound. Does not itself reset or
 * cancel DSP effects, flush PCM, free samples, or wait for hardware. */
int fh_audio_native_voices_reset(FhAudioNativeVoices *,uint64_t source_epoch);
/* Synth owner only, before admission in this source epoch. Source maps must be
 * empty; older timed DSP leases may remain intact after a source reset. Reject
 * current/future/immediate leases. Caller must publish a separately retained
 * effect route for the new source; this binding supplies no effect ownership or
 * mixed-generation scheduling. Never borrows source pointers in the callback. */
int fh_audio_native_voices_bind_aux(FhAudioNativeVoices *,unsigned studio);
/* Synth owner only, in the SAME already-bound generation/studio/mix mode.
 * Refresh source callback identities after the caller publishes an independently
 * retained effect route. Existing voices and immutable playback commands remain
 * untouched; future source segments capture the new handler mask. No epoch reset,
 * DSP operation, pool release, effect ownership or backpressure is supplied. */
int fh_audio_native_voices_refresh_aux(FhAudioNativeVoices *,unsigned studio);
#endif
