#ifndef FH_AUDIO_PCM_QUEUE_H
#define FH_AUDIO_PCM_QUEUE_H
#include <3ds.h>
#include "fh_audio_sample_pool.h"
#include "fh_audio_timeline.h"
#include "fh_audio_output_clock.h"
enum { FH_PCM_CHANNELS=2,FH_PCM_SLOTS=8,FH_PCM_FRAMES=2048,FH_PCM_SILENCE=160 };
typedef struct {
    ndspWaveBuf wave; /* Callback-owned; producer never reads SDK status. */
    uint64_t first_frame;
    unsigned frames,rate,state;
    FhAudioMap map;
} FhAudioPcmSlot;
typedef struct { uint64_t frame;unsigned epoch,rate,available; } FhAudioPcmCursor;
typedef struct { unsigned frames,buffers,rate,submitted,underruns,faults,paused; } FhAudioPcmStatus;
typedef struct {
    FhAudioPcmSlot slots[FH_PCM_SLOTS];
    ndspWaveBuf sentinel;
    /* Producer-owned or explicitly atomic mailboxes. */
    uint64_t next_frame;
    unsigned write_index,requested_epoch,acknowledged_epoch,paused,finish;
    /* SDK callback-owned state. */
    unsigned read_index,active_epoch,resetting,configured,prebuffer,rate,hardware_paused,start_pending;
    unsigned submitted,underruns,faults;
    FhAudioPcmCursor cursor;
    FhAudioSourceCursor source_cursor;
    FhAudioOutputClock output_clock;
    FhAudioSourceWindow source_window;
    unsigned output_draining;
    /* Set once before callback registration, only for native voice transport. */
    unsigned output_guard;
    /* Set before callback registration. Producer-owned FIFO effect watermark;
     * persists across PCM reset because effect tokens are application lifetimes. */
    unsigned effect_order_guard,queued_effect_token;
} FhAudioPcmChannel;
typedef struct FhAudioPcmQueue {
    FhAudioPcmChannel channels[FH_PCM_CHANNELS];
    int16_t *storage,*silence;
    FhAudioPoolFlush flush;void *flush_context;
    unsigned initialized;
} FhAudioPcmQueue;

/* Candidate replacement for existing two PCM queues: same 2*8*2048 stereo
 * sample storage, plus 640 permanent linear bytes for reset acknowledgement.
 * Zero initialize once. All producer APIs are serialized by the existing I/O
 * lock. service and cursor run ONLY inside NDSP's frame callback, after SDK
 * status read and before SDK command publication. No application locks or
 * allocation in callback. Owner/storage/silence live through ndspExit.
 * Reset is asynchronous: space/buffered return zero until the new silent
 * sequence is acknowledged. No buffer may be reused on speculative DONE.
 * A cursor counts submitted PCM INPUT frames in its epoch, at the reported
 * rate. A rate change needs a new epoch for consumers requiring one timebase.
 * The caller must not equate this cursor to source synth time without mapping
 * the original DMA/synth delay. Initial/rebuffer start is paired SDK submission
 * at first_frame, then actual returned buffer sequence/sample position drives it.
 */
int fh_audio_pcm_init(FhAudioPcmQueue *,int16_t *storage,size_t storage_bytes,
    int16_t *silence,size_t silence_bytes,FhAudioPoolFlush,void *context);
int fh_audio_pcm_space(FhAudioPcmQueue *,unsigned channel);
unsigned fh_audio_pcm_buffered(FhAudioPcmQueue *,unsigned channel);
FhAudioPcmStatus fh_audio_pcm_status(FhAudioPcmQueue *,unsigned channel);
int fh_audio_pcm_queue(FhAudioPcmQueue *,unsigned channel,const int16_t *,unsigned frames,unsigned rate);
int fh_audio_pcm_queue_mapped(FhAudioPcmQueue *,unsigned,const int16_t *,unsigned,unsigned,const FhAudioMap *);
/* Callback-only, copied before a DONE slot is released; zero for gaps/pauses/reset. */
FhAudioSourceCursor fh_audio_pcm_source_cursor(const FhAudioPcmQueue *,unsigned);
FhAudioSourceWindow fh_audio_pcm_source_window(const FhAudioPcmQueue *,unsigned);
int fh_audio_pcm_reset(FhAudioPcmQueue *,unsigned channel);
int fh_audio_pcm_pause(FhAudioPcmQueue *,unsigned channel,int paused);
int fh_audio_pcm_finish(FhAudioPcmQueue *,unsigned channel);
void fh_audio_pcm_service(FhAudioPcmQueue *);
FhAudioPcmCursor fh_audio_pcm_cursor(const FhAudioPcmQueue *,unsigned channel);
#include "fh_audio_aux_window.h"
/* SDK callback ONLY, immediately after pcm_service. No producer-owned metadata
 * is read. Does not change output clock, queue ownership or voice window. */
FhAudioAuxWindow fh_audio_pcm_aux_window(const FhAudioPcmQueue *,unsigned channel);
#endif
