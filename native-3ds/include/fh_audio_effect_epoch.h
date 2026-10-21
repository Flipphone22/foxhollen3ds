#ifndef FH_AUDIO_EFFECT_EPOCH_H
#define FH_AUDIO_EFFECT_EPOCH_H
#include <stdint.h>

enum { FH_AUDIO_EFFECT_SLOTS=2 };
enum { FH_AUDIO_EFFECT_OK=0,FH_AUDIO_EFFECT_BUSY=1,FH_AUDIO_EFFECT_INVALID=2 };
typedef struct {
    void *payload;
    uint32_t token,state;
} FhAudioEffectSlot;
typedef struct {
    FhAudioEffectSlot slots[FH_AUDIO_EFFECT_SLOTS];
    uint32_t published_token; /* Producer only. */
    uint32_t return_floor;    /* Callback writes; producer acquires. */
    uint32_t active_slot;     /* Callback only, index+1; zero outside processing. */
} FhAudioEffectEpochs;

/* Experimental bounded effect-buffer ownership; zero initialize before use.
 * One source producer prepares/destroys payloads. One SDK callback processes
 * them. Payload storage is supplied by the caller; this module never allocates,
 * waits, processes samples, touches the SDK or resets a source effect.
 * Tokens identify effect lifetimes, NOT synth-clock resets. They increase and
 * cannot wrap. The producer must observe actual source effect create/modify/
 * free events, including recreation with IDENTICAL parameters.
 *
 * Busy preserves all state: retain the unsubmitted change, do not omit it or
 * move an already-native voice onto a stale software resampler. Two live
 * payloads bound memory; further changes need source scheduling/backpressure.
 * No game integration is implied by this owner alone. */
int fh_audio_effect_publish(FhAudioEffectEpochs *,uint32_t token,void *payload);
/* Producer only. Returns a retired payload for destruction/reuse. Publication
 * does not authorize collection; only a callback retirement does. */
int fh_audio_effect_collect(FhAudioEffectEpochs *,uint32_t *token,void **payload);
/* SDK callback only. Begin/end bracket ALL accesses to returned effect state.
 * Unknown, retired, unavailable, nested or backwards selection fails closed. */
void *fh_audio_effect_begin(FhAudioEffectEpochs *,uint32_t token);
int fh_audio_effect_end(FhAudioEffectEpochs *,uint32_t token);
/* SDK callback only, outside begin/end. Caller must prove no future input,
 * queued bank tag, bus callback or needed tail can reference an older token.
 * A returned AUX tag can establish input order; an upcoming tag CANNOT.
 * Do not call this merely because the synth producer advanced. Unknown tags
 * never authorize retirement. Can retire older unused publications too.
 * This watermark is irreversible; it is not a command to reset reverb tails. */
int fh_audio_effect_retire_before(FhAudioEffectEpochs *,uint32_t token);
#endif
