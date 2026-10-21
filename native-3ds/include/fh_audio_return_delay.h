#ifndef FH_AUDIO_RETURN_DELAY_H
#define FH_AUDIO_RETURN_DELAY_H
#include <stdint.h>
/* SDK callback owner only. One original 160-sample/32 kHz effect return cycle.
 * Zero-initialize before playback; never reset from the producer while old
 * playback is queued. Integration must define generation/bus-mode retirement.
 * No allocation. This component is not yet connected to the game backend. */
typedef struct {
    int32_t samples[3][160];
    unsigned cursor;
} FhAudioReturnDelay;
int fh_audio_return_delay(FhAudioReturnDelay *,int32_t *,int32_t *,int32_t *,unsigned frames);
#endif
