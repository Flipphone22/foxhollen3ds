#ifndef FH_AUDIO_GAME_DSP_H
#define FH_AUDIO_GAME_DSP_H
#include <stdint.h>
#include "fh_audio_sample_pool.h"
#include "fh_audio_timeline.h"
struct FhAudioPcmQueue;
/* Experimental replay integration only. Source owner uses the existing synth
 * lock; callback never takes it. All allocations outlive ndspExit. */
void fh_audio_game_output_mix(unsigned channel,const float mix[12]);
int fh_audio_game_dsp_preinit(void); /* Install immutable hook before ndspInit. */
int fh_audio_game_dsp_initialize(FhAudioPoolFlush,void *); /* After ndspInit. */
void fh_audio_game_dsp_prepare(uint64_t source_frame,unsigned generation);
/* Synth owner only, immediately after prepare and before rendering the block.
 * Stamp this immutable route into that block's existing source provenance. */
uint32_t fh_audio_game_dsp_effect_token(void);
void fh_audio_game_dsp_service(const struct FhAudioPcmQueue *);
void fh_audio_game_dsp_report(void);
void fh_audio_game_dsp_shutdown(void); /* AFTER ndspExit and worker join. */
#endif
