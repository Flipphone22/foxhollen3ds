#ifndef FH_AUDIO_EFFECT_SOURCE_H
#define FH_AUDIO_EFFECT_SOURCE_H
#include <stdint.h>

enum { FH_AUDIO_EFFECT_SOURCE_SLOTS=16 };
typedef struct {
    const void *address;
    uint32_t token;
    float parameters[5]; /* coloration, time, mix, damping, predelay */
} FhAudioEffectSource;
typedef struct {
    FhAudioEffectSource slots[FH_AUDIO_EFFECT_SOURCE_SLOTS];
    uint32_t serial;
} FhAudioEffectSources;

/* Synth-owner only, under the original source synchronization. This records
 * successful effect construction, not synth-clock generations or mutable
 * public settings. A recreation with identical parameters is a NEW lifetime.
 * No worker may borrow these slots or the source work pointer. Copy a snapshot
 * into a separately retained payload before publishing it to the DSP callback.
 * Zero initialize once; do not reset serial when the game resets its synth.
 * Full/overflow/invalid creation fails without changing the registry. */
int fh_audio_effect_source_create(FhAudioEffectSources *,const void *,const float parameters[5]);
int fh_audio_effect_source_destroy(FhAudioEffectSources *,const void *);
int fh_audio_effect_source_read(const FhAudioEffectSources *,const void *,FhAudioEffectSource *);
#endif
