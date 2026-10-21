#ifndef FH_AUDIO_SAMPLE_PLAN_H
#define FH_AUDIO_SAMPLE_PLAN_H
#include <stddef.h>
#include <stdint.h>

/* Load-time compiler for immutable, static GC-ADPCM only. Streaming/refill
 * sources must never enter this interface. No ownership or playback state is
 * retained. Caller owns source/output, admission budget and DSP publication.
 * The initial traversal and loop traversal can have DIFFERENT decoder history;
 * they therefore occupy separate PCM ranges. Counts exclude alignment padding.
 * Source and output may not overlap. Output must be 32-byte aligned.
 * This compiler is not yet connected to the application's voice scheduler. */
typedef struct {
    const uint8_t *encoded;
    size_t encoded_bytes;
    uint32_t length, loop_start, loop_length;
    int16_t coefficients[16];
    int16_t loop_y1, loop_y2;
    uint8_t initial_ps, loop_ps;
} FhAudioSampleInput;
typedef struct {
    uint32_t first_samples, loop_offset, loop_samples, storage_samples;
} FhAudioSamplePlan;
enum { FH_AUDIO_PLAN_OK=0, FH_AUDIO_PLAN_INVALID=1, FH_AUDIO_PLAN_CAPACITY=2 };

/* No mutation on failure. measure does not read encoded payload bytes. */
int fh_audio_sample_measure(const FhAudioSampleInput *, FhAudioSamplePlan *);
int fh_audio_sample_compile(const FhAudioSampleInput *, int16_t *pcm,
                           size_t pcm_samples, FhAudioSamplePlan *);

/* DSP descriptors retain compressed frames when both traversals can use the
 * existing frame boundaries/headers. Otherwise use exact, separate PCM ranges.
 * This is representation selection, not authorization to play a live voice.
 * No-extra-data static loops whose history is not reset are not representable
 * by this reset-history input contract. Their adapter must reject admission.
 * ADPCM loop_offset_bytes can be 8-byte aligned; PCM offsets are 32-byte aligned.
 */
enum { FH_AUDIO_DSP_PCM16=1, FH_AUDIO_DSP_ADPCM=2 };
typedef struct {
    uint32_t first_samples,loop_samples,loop_offset_bytes,storage_bytes;
    uint16_t format;
    int16_t loop_y1,loop_y2;
    int16_t coefficients[16];
} FhAudioDspSamplePlan;
int fh_audio_dsp_sample_measure(const FhAudioSampleInput *,FhAudioDspSamplePlan *);
int fh_audio_dsp_sample_compile(const FhAudioSampleInput *,void *storage,
                               size_t storage_bytes,FhAudioDspSamplePlan *);
#endif
