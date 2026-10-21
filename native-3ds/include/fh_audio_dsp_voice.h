#ifndef FH_AUDIO_DSP_VOICE_H
#define FH_AUDIO_DSP_VOICE_H
#include "fh_audio_dsp_retire.h"

/* All translation units using this ABI must use the same capacity. Production
 * remains at four until the complete source/effects backend is admitted. The
 * integration fixture uses 256 to cover buffered 1ms controls without dropping
 * events. It is normal metadata, not DSP-visible sample storage. */
#ifndef FH_AUDIO_DSP_COMMAND_CAPACITY
#define FH_AUDIO_DSP_COMMAND_CAPACITY 4
#endif
enum { FH_AUDIO_DSP_COMMANDS=FH_AUDIO_DSP_COMMAND_CAPACITY };
_Static_assert(FH_AUDIO_DSP_COMMANDS>=4 && FH_AUDIO_DSP_COMMANDS<=512 &&
    !(FH_AUDIO_DSP_COMMANDS&(FH_AUDIO_DSP_COMMANDS-1)),"bounded DSP command capacity");
typedef struct {
    float rate, mix[12];
    unsigned interpolation,paused;
} FhAudioDspControls;

/* Compact source curve, evaluated only at a DSP block boundary. The original
 * volume and envelope divisions remain separate (their product is quadratic).
 * It fits inside the existing command payload; no per-sample CPU synthesis. */
typedef struct {
    float rate;
    /* Source order: main L/R/S, AUX A L/R/S, AUX B L/R/S. */
    uint16_t volume_start[9],volume_end[9];
    uint16_t env_start,env_end,offset,frames,cycle_frames,interpolation,paused,routing;
} FhAudioDspCurve;
enum { FH_AUDIO_DSP_ROUTE_STD=0,FH_AUDIO_DSP_ROUTE_DPL2_STEREO=1 };
typedef struct {
    FhAudioPoolToken generation;
    uint64_t due;
    unsigned kind;
    union { FhAudioDspControls controls; FhAudioDspCurve curve; };
} FhAudioDspCommand;
typedef struct {
    /* Producer-owned, immutable between START publication and retirement. */
    FhAudioPoolToken lease;
    const void *samples;
    FhAudioDspSamplePlan plan;
    unsigned stopping;
    uint64_t epoch,last_due,curve_end;
    unsigned timed,cancel;
    /* SPSC: producer head, callback tail. Never reset while NDSP is running. */
    unsigned head,tail;
    FhAudioDspCommand commands[FH_AUDIO_DSP_COMMANDS];
    /* Callback-owned. Neither producer nor UI reads these concurrently. */
    FhAudioPoolToken current;
    ndspWaveBuf first,loop;
    ndspWaveBuf delay;
    ndspAdpcmData first_history,loop_history;
    ndspAdpcmData delay_history;
    unsigned live;
    FhAudioDspControls applied;
    unsigned clock_held;
    unsigned ramped,curve_valid;
    uint64_t curve_due;
    FhAudioDspCurve curve;
} FhAudioDspVoice;
typedef struct {
    FhAudioDspRetirement retirement;
    FhAudioDspVoice voices[FH_AUDIO_DSP_CHANNELS];
    unsigned initialized,starts,controls,stops,natural_ends,discarded,faults;
    unsigned cancelled,epoch_rejected,held;
    unsigned curve_shortages;
    unsigned prefix_bytes,prefix_starts,prefix_samples,prefix_faults;
    unsigned retained_controls;
    uint64_t late_samples;
} FhAudioDspVoices;

/* Experimental native playback owner, not a MusyX scheduler. Requires the same
 * exclusive channels 2..23 and NDSP-callback execution as retirement. One audio
 * producer owns pool/start/control/stop/poll; callback performs SDK calls only.
 * No allocations, decoding, cache flushing or pool access in callback. All
 * prepared data must have been published. The entire owner and silence remain
 * alive through ndspExit. Source ADSR/depop/filter/ITD/AUX routing and the 32kHz
 * control clock MUST be integrated separately before any game voice is omitted
 * from the software mixer. This is not sample-accurate command scheduling.
 * Zero-initialize once before registering the callback. Queue full returns BUSY
 * without dropping/reordering a command; caller must
 * retain/retry it. No timeout or DONE-only sample release.
 */
int fh_audio_dsp_voices_init(FhAudioDspVoices *,FhAudioSamplePool *,int16_t *silence,
                            size_t bytes,FhAudioPoolFlush,void *context);
int fh_audio_dsp_voice_start(FhAudioDspVoices *,unsigned channel,
    FhAudioPoolToken bank,uint32_t sample,const FhAudioDspControls *,FhAudioPoolToken *voice);
int fh_audio_dsp_voice_control(FhAudioDspVoices *,unsigned channel,
    FhAudioPoolToken voice,const FhAudioDspControls *);
int fh_audio_dsp_voice_stop(FhAudioDspVoices *,unsigned channel,FhAudioPoolToken voice);
/* Timed variant. `due` is an absolute frame on the OUTPUT SOURCE timeline (not
 * renderer frames or wall time); epoch is nonzero and must increase on output
 * reset. Commands must be nondecreasing within a voice. Never mix immediate and
 * timed controls on a voice. Existing four-slot backpressure is unchanged.
 * These are DSP-frame-boundary commands, NOT subframe/sample-accurate envelopes.
 * The source adapter must supply an authoritative buffered-output cursor and
 * preserve/translate the original 1ms envelopes separately. */
int fh_audio_dsp_voice_start_at(FhAudioDspVoices *,unsigned channel,
    FhAudioPoolToken bank,uint32_t sample,const FhAudioDspControls *,
    uint64_t epoch,uint64_t due,FhAudioPoolToken *voice);
int fh_audio_dsp_voice_control_at(FhAudioDspVoices *,unsigned channel,
    FhAudioPoolToken voice,const FhAudioDspControls *,uint64_t due);
/* Experimental frame-ramped direct stereo. Firmware format fade bit is used;
 * this is NOT arbitrary 1ms envelope reproduction or depop parity. */
int fh_audio_dsp_voice_start_ramped_at(FhAudioDspVoices *,unsigned channel,
    FhAudioPoolToken bank,uint32_t sample,const FhAudioDspControls *,
    uint64_t epoch,uint64_t due,FhAudioPoolToken *voice);
/* Contiguous identical constant curves retain their published state without
 * a new queue slot. Ramps/changes/gaps still publish; no queued entry is edited.
 * Producer deadline validation advances for retained segments as well. */
int fh_audio_dsp_voice_curve_at(FhAudioDspVoices *,unsigned channel,
    FhAudioPoolToken voice,const FhAudioDspCurve *,uint64_t due);
int fh_audio_dsp_curve_gain(const FhAudioDspCurve *,unsigned sample,int32_t gain[2]);
int fh_audio_dsp_curve_gains(const FhAudioDspCurve *,unsigned sample,int32_t gain[9]);
/* Main S uses the two rear lanes for stereo downmix; AUX S remains separate
 * until the original three-channel effect processes it. No early clipping. */
int fh_audio_dsp_curve_mix(const FhAudioDspCurve *,unsigned sample,float mix[12]);
int fh_audio_dsp_voice_stop_at(FhAudioDspVoices *,unsigned channel,FhAudioPoolToken voice,uint64_t due);
/* Explicit cancellation, including queued future starts, bypasses deadlines.
 * This is reset/shutdown/error cancellation, NOT an audible release envelope. */
int fh_audio_dsp_voice_abort(FhAudioDspVoices *,unsigned channel,FhAudioPoolToken voice);
/* Register/multiplex ONLY from ndspSetCallback; preserve existing capture hook. */
void fh_audio_dsp_voices_service(FhAudioDspVoices *);
/* SDK callback only. unavailable holds active timed voices; an older voice
 * epoch is cancelled once playback advances past it. Future-epoch commands
 * remain queued; epoch zero holds them until output provenance is known.
 * Retirement acknowledgements keep running while output is held.
 * Immediate voices retain their existing behavior. Cursor must be monotonic
 * within an epoch; no timer interpolation or frame-rate multiplier is hidden. */
void fh_audio_dsp_voices_service_at(FhAudioDspVoices *,uint64_t epoch,uint64_t cursor,int available);
/* `first`/`end` describe the upcoming DSP block on the source timeline, NOT a
 * stale returned playback position. Caller must prove that mapping, including
 * pause/reset/underrun. Future curves are read without consuming their rate or
 * pause commands. They remain immutable until the callback releases tail. */
void fh_audio_dsp_voices_service_window(FhAudioDspVoices *,uint64_t epoch,
    uint64_t first,uint64_t end,int available);
/* Experimental 32kHz source region inside one upcoming DSP block. Leading
 * silence delays NEW voices by queued zero samples, not a CPU timer. Existing
 * voices cannot resume across an interior gap through this API: that case is
 * explicitly faulted/held. No complete transition or pitch-change claim.
 * init's silence storage may be 2048 bytes to cover prefixes through192kHz;
 * the original320-byte allocation remains valid but has limited prefix capacity.
 * Entire silence allocation must remain alive through ndspExit. */
void fh_audio_dsp_voices_service_region(FhAudioDspVoices *,uint64_t epoch,
    uint64_t first,uint64_t end,unsigned leading_frames,int available);
/* Completed channel bits mean sample leases AND old descriptors can be reused.
 * Natural end is observed on the DSP clock, not a game-simulation notification. */
int fh_audio_dsp_voices_poll(FhAudioDspVoices *,uint32_t *completed_channels);
#endif
