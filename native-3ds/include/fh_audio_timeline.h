#ifndef FH_AUDIO_TIMELINE_H
#define FH_AUDIO_TIMELINE_H
#include <stdint.h>
#include <stddef.h>

enum { FH_AUDIO_MAP_SPANS=4, FH_AUDIO_MAP_BUFFERS=6 };
/* A generation identifies a source synth lifetime, not a DSP queue epoch.
 * generation==0 means no synth contribution (silence, movie-only or unknown).
 * Offsets/counts are PCM input frames, never wall time or DSP output samples. */
typedef struct {
    uint64_t source_frame;
    uint32_t generation;
    uint16_t offset,frames;
    uint32_t effect_token; /* Native route which produced these source frames. */
} FhAudioSpan;
typedef struct { FhAudioSpan spans[FH_AUDIO_MAP_SPANS]; unsigned count; } FhAudioMap;
/* Upcoming DSP block, expressed in PCM input frames. Unknown spans are retained
 * with generation 0; valid means complete coverage, NOT all-synth audio. Four
 * parts cover the game's 160-frame source slices at 32/48kHz. Unrepresentable
 * fragmentation returns invalid rather than silently dropping a transition. */
typedef struct {
    uint64_t input_first;
    FhAudioSpan spans[FH_AUDIO_MAP_SPANS];
    unsigned frames,count,valid;
} FhAudioSourceWindow;
typedef struct { uint64_t frame; unsigned generation,available; } FhAudioSourceCursor;
typedef struct { const int16_t *address; FhAudioSpan span; } FhAudioBufferStamp;
typedef struct { FhAudioBufferStamp buffers[FH_AUDIO_MAP_BUFFERS]; } FhAudioTimeline;

/* Producer/synth-owner only. The six exact, owned buffers are bound once:
 * four synth buffers and two movie mix buffers. Unknown sources stay unmapped.
 * Copy provenance at the time PCM is copied, not when a worker is scheduled.
 * Metadata never changes the original PCM arithmetic or packet boundaries. */
int fh_audio_timeline_bind(FhAudioTimeline *,const int16_t *const buffers[FH_AUDIO_MAP_BUFFERS]);
int fh_audio_timeline_stamp(FhAudioTimeline *,const int16_t *,unsigned frames,
    uint64_t source_frame,unsigned generation);
/* Same source buffer ownership; token is copied into queued PCM metadata.
 * Zero preserves software-only/unknown-effect callers. A nonzero effect token
 * requires a source generation. Do not stamp from the producer's newer state
 * when copying an older buffer, or infer it from a synth-clock generation. */
int fh_audio_timeline_stamp_effect(FhAudioTimeline *,const int16_t *,unsigned frames,
    uint64_t source_frame,unsigned generation,uint32_t effect_token);
FhAudioSpan fh_audio_timeline_read(const FhAudioTimeline *,const int16_t *,unsigned frames);
int fh_audio_timeline_copy(FhAudioTimeline *,const int16_t *destination,
    const int16_t *source,unsigned frames);
int fh_audio_map_append(FhAudioMap *,FhAudioSpan,unsigned offset,unsigned frames);
int fh_audio_map_valid(const FhAudioMap *,unsigned frames);
FhAudioSourceCursor fh_audio_map_cursor(const FhAudioMap *,unsigned offset,unsigned available);
/* Discard the entire window on append failure; partial accumulation is allowed.
 * Only a successful finish grants complete coverage. */
int fh_audio_source_window_append(FhAudioSourceWindow *,const FhAudioMap *,
    uint64_t packet_first,unsigned packet_frames,uint64_t first,uint64_t end);
int fh_audio_source_window_finish(FhAudioSourceWindow *,uint64_t first,uint64_t end);
#endif
