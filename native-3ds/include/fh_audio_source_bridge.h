#ifndef FH_AUDIO_SOURCE_BRIDGE_H
#define FH_AUDIO_SOURCE_BRIDGE_H
#include "fh_audio_sample_clock.h"
#include "musyx/dspvoice.h"

typedef struct {
    uint64_t source_frame;
    uint32_t pitch;
    uint16_t offset,frames,cycle_frames,env_start,env_end;
    unsigned held;
} FhAudioSourceSegment;
typedef struct {
    /* Calls are synchronous on the original synth owner. Pointers are borrowed
     * only until return. Native backend must copy controls/pin sample storage.
     * begin=0 declines BEFORE software history is discarded. Once accepted,
     * every segment/stop must succeed; no mid-note fallback with stale history.
     * The backend must support changes in all routing/gain/ITD/filter controls,
     * source start offset, depop tails and reverb/studio routing before admission.
     * It must reserve enough queue capacity before source control advances.
     * This interface alone authorizes no CPU omission in a deployed build. */
    uint64_t (*begin)(void *,unsigned,const DSPvoice *,uint64_t source_frame);
    int (*segment)(void *,uint64_t,const DSPvoice *,const FhAudioSourceSegment *);
    int (*stop)(void *,uint64_t,uint64_t source_frame);
    void *context;
} FhAudioSourceBackend;
typedef struct {
    FhAudioSampleClock clock;
    uint64_t token;
    SAMPLE_INFO sample;
} FhAudioSourceVoice;
typedef struct {
    FhAudioSourceBackend backend;
    FhAudioSourceVoice voices[SYNTH_MAX_VOICES];
    unsigned accepted,declined,segments,stops,faults;
} FhAudioSourceBridge;
/* Single synth owner. NULL leaves the source mixer unchanged. Not an NDSP
 * callback object; DSP playback needs the separate output-cursor mapping. */
extern FhAudioSourceBridge *fh_audio_source_bridge;
int fh_audio_source_begin(unsigned,const DSPvoice *,uint64_t source_frame,unsigned rate);
/* -1=software voice, -2=fault (must not fall back), 0=active, 1=sample ended. */
int fh_audio_source_segment(unsigned,DSPvoice *,const FhAudioSourceSegment *);
int fh_audio_source_stop(unsigned,uint64_t source_frame);
#endif
