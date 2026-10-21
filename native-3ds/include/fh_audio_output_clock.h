#ifndef FH_AUDIO_OUTPUT_CLOCK_H
#define FH_AUDIO_OUTPUT_CLOCK_H
#include <stdint.h>

typedef struct {
    uint64_t frame;
    uint32_t fraction,epoch,rate,last_tick,last_dropped;
    unsigned valid,running,observed,buffering,starvations;
} FhAudioOutputClock;
typedef struct { uint64_t first,end;unsigned available; } FhAudioOutputWindow;

/* Before command publication, co-pause PCM and native voices if less than two
 * DSP blocks remain. One block of reserve covers returned-state latency and
 * fixed-point rate rounding. Refill the caller's four-PCM-packet margin (at
 * least four DSP blocks) before resuming. Neither input
 * position nor fractional phase advances while held. Explicit terminal drains
 * bypass this gate; it is not recovery for an unknown/dropped DSP clock. */
int fh_audio_output_gate(FhAudioOutputClock *,unsigned soc_clock,uint64_t queued_end,unsigned refill_frames);

/* Callback-owned prediction of submitted work, not a wall-clock extrapolation.
 * tick is the SDK submission count. Missing callbacks make it unavailable
 * until an explicit paired start. Dropped output deadlines do not consume extra
 * source samples and do not reset this timeline. rate/SOC rational preserves
 * fractional input frames over pauses. DSP quantization/feedback must still be
 * checked by the caller before native playback admission. No sound is omitted
 * merely because this metadata is unavailable. */
FhAudioOutputWindow fh_audio_output_step(FhAudioOutputClock *,uint32_t tick,
    uint32_t dropped,uint32_t epoch,unsigned rate,unsigned soc_clock,
    int start,uint64_t first,int running,uint64_t queued_end);
#endif
