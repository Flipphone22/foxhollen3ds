#ifndef FH_AUDIO_VOICE_WINDOW_H
#define FH_AUDIO_VOICE_WINDOW_H
#include "fh_audio_timeline.h"

/* SDK callback only. Coalesce continuous source frames for ONE voice service,
 * irrespective of effect boundaries. The original AUX window remains intact.
 * Leading unknown PCM is allowed; interior gaps or source-generation changes
 * require a different scheduling mechanism and are rejected before mutation.
 * Returns -1 invalid/unsupported, 0 hold, 1 region. Writes result only on 1.
 * Result.effect_token is the first token, NOT a replacement for AUX provenance. */
int fh_audio_voice_window(const FhAudioSourceWindow *,FhAudioSpan *result);
#endif
