#ifndef FH_AUDIO_AUX_RATE_H
#define FH_AUDIO_AUX_RATE_H
#include <stdint.h>
/* Callback owns all state. Input L/R/S are DSP sends at SOC/512; original
 * effect receives a variable count at exactly32kHz. The effect implementation
 * must support arbitrary block partitioning; original fixed160 APIs do not.
 * No effect pointer/parameter may be changed or freed until owner quiescence.
 * Return is stereo with source S downmix. Channels2/3 are zero on return.
 * Two DSP samples of causal interpolation delay; firmware AUX delay additional.
 * Original rate retained, linear resampling is an explicit approximation. */
typedef void (*FhAudioAux32Process)(void *,int32_t *left,int32_t *right,int32_t *surround,unsigned frames);
typedef struct {
    int32_t work[3][162],previous[3];
    int32_t input_phase;
    uint32_t output_phase;
    uint64_t source_frames,output_source;
    unsigned initial_silence,blocks,faults;
} FhAudioAuxRate;
/* Zero-initialize before first call; no allocations or global state. */
int fh_audio_aux_rate_process(FhAudioAuxRate *,int32_t samples[4][160],FhAudioAux32Process,void *context);
/* Immutable DPL2 stereo-return route only: S is not returned by the source.
 * Callback must process L/R independently and not read or write S. Do not
 * switch an existing rate/effect owner between these two entry points. */
int fh_audio_aux_rate_stereo(FhAudioAuxRate *,int32_t samples[4][160],FhAudioAux32Process,void *context);
#endif
