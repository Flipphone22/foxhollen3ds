#ifndef FH_AUDIO_AUX_INPUT_H
#define FH_AUDIO_AUX_INPUT_H
#include <stddef.h>
#include <stdint.h>

enum { FH_AUX_INPUT_SLOTS=64, FH_AUX_INPUT_FRAMES=160 };
enum { FH_AUX_INPUT_OK=0, FH_AUX_INPUT_BUSY=1, FH_AUX_INPUT_INVALID=2 };
typedef struct {
    uint64_t first;
    uint32_t generation,effect;
} FhAudioAuxInputStamp;
typedef struct {
    FhAudioAuxInputStamp stamps[FH_AUX_INPUT_SLOTS];
    int32_t *samples;
    uint64_t next_frame; /* Producer only. */
    uint32_t head,tail,channels,generation,write_started,read_offset;
} FhAudioAuxInput;

/* Experimental CPU-to-CPU raw AUX transport, not an NDSP wave buffer.
 * One synth producer and one SDK-thread consumer. Normal RAM is sufficient;
 * the DSP/GPU never reads these samples. Copy before release-publishing head;
 * release tail only after all CPU reads. Existing SDK flush/exit rules still
 * own actual DSP buffers. Caller retains this storage until both owners stop.
 *
 * Two or three planar channels, 160 signed32 samples per source cycle. The
 * producer supplies EXACT source frame/generation/effect identity, not an
 * inferred output clock or borrowed source pointers. Generations increase
 * monotonically; a new generation may restart its source clock while older
 * samples remain queued. No producer reset may discard unread samples.
 * No allocation, wait, filesystem work, silence substitution or DSP calls.
 * This primitive does not establish wet/dry pipeline latency or game admission.
 * Initialize a zeroed, stopped queue. Storage must not overlap the queue or
 * any input/output planes. Output planes must be mutually disjoint. Only the
 * designated consumer may call ready/add; only the producer may call push.
 */
size_t fh_audio_aux_input_bytes(unsigned channels);
int fh_audio_aux_input_init(FhAudioAuxInput *,int32_t *storage,size_t bytes,unsigned channels);
int fh_audio_aux_input_push(FhAudioAuxInput *,uint32_t generation,uint32_t effect,
    uint64_t first,const int32_t *left,const int32_t *right,const int32_t *surround);
/* Consumer preflight may inspect a later range in the retained queue. It does
 * not move the read cursor or release samples; useful before changing effect
 * histories. BUSY means the producer has not published enough input yet. */
int fh_audio_aux_input_ready(const FhAudioAuxInput *,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames);
/* Validate a span exactly 'ahead' unconsumed samples from the read cursor.
 * This lets a complete returned block cross source-generation boundaries
 * without searching by source time (which can restart) or releasing old data.
 * Only the consumer calls this; successful checks do not reserve new capacity. */
int fh_audio_aux_input_check(const FhAudioAuxInput *,unsigned ahead,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames);
/* Same preflight, additionally requiring the current read cursor. */
int fh_audio_aux_input_next(const FhAudioAuxInput *,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames);
/* Consume only the NEXT source range, adding matching raw input to converted
 * native AUX. Reject overflow or missing/mismatched input BEFORE changing either
 * destination or cursor. No clipping rule or source-delay adjustment is invented.
 * A caller must preflight the complete multi-span effect operation separately. */
int fh_audio_aux_input_add(FhAudioAuxInput *,uint32_t generation,uint32_t effect,
    uint64_t first,unsigned frames,int32_t *left,int32_t *right,int32_t *surround);
#endif
