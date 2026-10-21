#ifndef FH_AUDIO_AUX_OUTPUT_DELAY_H
#define FH_AUDIO_AUX_OUTPUT_DELAY_H
#include <stdint.h>
/* Callback-owned two-DSP-sample history, matching the AUX rate adapter's
 * output interpolation latency. Completed sample values only, no pointers or
 * effect/sample leases. Never producer-reset while callbacks can run. */
typedef struct { int32_t samples[4][2]; } FhAudioAuxOutputDelay;
int fh_audio_aux_output_delay(FhAudioAuxOutputDelay *,int32_t samples[4][160]);
#endif
