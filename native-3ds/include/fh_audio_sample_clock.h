#ifndef FH_AUDIO_SAMPLE_CLOCK_H
#define FH_AUDIO_SAMPLE_CLOCK_H
#include <stdint.h>

enum { FH_AUDIO_CLOCK_POLYPHASE=0,FH_AUDIO_CLOCK_LINEAR=1,FH_AUDIO_CLOCK_NONE=2 };
typedef struct {
    uint32_t end,loop_start,loop_length;
    uint32_t position,phase,history_index;
    uint8_t tail,ended;
} FhAudioSampleClock;
typedef struct { uint32_t read_samples,sample_done; } FhAudioSampleStep;

/* Static samples starting at source position zero. Init validates immutable
 * bounds once. No stream/virtual/ADPCM_PLUS offsets or refill callbacks.
 * This tracks the ORIGINAL 32kHz source control timeline, not elapsed wall time
 * or NDSP's output position. It deliberately produces no audio/history values.
 * Only a permanently admitted native voice may use it in place of resampling;
 * switching back to software requires reconstructing waveform/filter/ITD state.
 * The integration must retain original ADSR, pitch selection, subframe order,
 * zero-pitch admission and SAMPLE_END delivery. This helper does not execute
 * callbacks or suppress software mixing. Clock state is single-owner.
 */
int fh_audio_sample_clock_init(FhAudioSampleClock *,uint32_t length,uint32_t loop_start,uint32_t loop_length);
/* One source segment, <=160 output samples, <=6x pitch in unsigned Q16.16.
 * NONE consumes one source sample/output sample regardless of pitch, matching
 * the original reader. Zero-frame calls do not advance. Invalid arguments leave
 * state and result untouched. Return 1 success, 0 invalid. */
int fh_audio_sample_clock_step(FhAudioSampleClock *,uint32_t frames,uint32_t pitch,
                               unsigned mode,FhAudioSampleStep *);
#endif
