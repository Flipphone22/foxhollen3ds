#ifndef FH_AUDIO_AUX_WINDOW_H
#define FH_AUDIO_AUX_WINDOW_H
#include "fh_audio_timeline.h"
/* SDK-owned snapshot for one actual PCM submission. Source spans describe the
 * nominal input window; lookahead is ONE additional PCM input frame, with its
 * own original generation/effect token. It never extends voice playback time.
 * phase is the PCM clock numerator remainder BEFORE this block, denominator
 * SYSCLOCK_SOC. queue_epoch identifies PCM resets, not synth generations.
 * Missing lookahead stays unavailable; never infer it from current preparation.
 * This metadata does not itself schedule mixed-source voices/effects. */
typedef struct {
    FhAudioSourceWindow source;
    FhAudioSpan lookahead;
    uint32_t queue_epoch,phase,rate,has_lookahead;
} FhAudioAuxWindow;
#endif
