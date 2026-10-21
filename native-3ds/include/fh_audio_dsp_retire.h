#ifndef FH_AUDIO_DSP_RETIRE_H
#define FH_AUDIO_DSP_RETIRE_H
#include <3ds.h>
#include "fh_audio_sample_pool.h"
enum { FH_AUDIO_DSP_FIRST_CHANNEL=2,FH_AUDIO_DSP_CHANNELS=22,FH_AUDIO_DSP_SILENCE_SAMPLES=160 };
typedef struct { ndspWaveBuf sentinel;FhAudioPoolToken lease;unsigned state; } FhAudioDspRetiringVoice;
typedef struct {
    FhAudioSamplePool *pool;
    int16_t *silence;
    unsigned initialized,requests,acknowledged,faults;
    FhAudioDspRetiringVoice voices[FH_AUDIO_DSP_CHANNELS];
} FhAudioDspRetirement;
/* Experimental adapter, not hooked to game playback. Requires initialized NDSP,
 * exclusive ownership of channels 2..23. request/drain have ONE producer,
 * serialized with the sample pool. service MUST execute inside NDSP's own frame
 * callback, never concurrently with ndspiReadChnState/ndspiUpdateChn. An unrelated
 * game/audio mutex is insufficient: SDK sync/cache writes precede its lock.
 * The acquire/release mailbox isolates callback-owned SDK descriptors from
 * producer-owned pool leases. The callback never reads or mutates the pool.
 * Movie and existing software-mix channels 0/1 remain outside this owner.
 * Flush is synchronous. Silence must be DSP-visible linear memory. Silence and
 * this owner MUST live until ndspExit returns,
 * including after pending reaches zero: clear/pause are not device fences.
 * Start from zero initialization. No allocation, wait, timeout or frame-count
 * guess is used. Natural endings/depop/envelope timing belong to the voice
 * scheduler; request is cancellation, not a replacement for its stop envelope.
 */
int fh_audio_dsp_retire_init(FhAudioDspRetirement *,FhAudioSamplePool *,int16_t *silence,
                            size_t silence_bytes,FhAudioPoolFlush,void *context);
/* Transfers exclusive ownership of this lease until acknowledgement. Keep the
 * old voice descriptor alive until its completed channel bit is returned. */
int fh_audio_dsp_retire_request(FhAudioDspRetirement *,unsigned channel,FhAudioPoolToken lease);
/* Playback owner: producer transfers a lease before publishing its START.
 * ARMED retains it without changing the SDK channel. begin is then called by
 * the SDK callback after the ordered STOP (or natural end). Do not call request
 * for the same channel. Both stages retain the descriptor until drain. */
int fh_audio_dsp_retire_arm(FhAudioDspRetirement *,unsigned channel,FhAudioPoolToken lease);
int fh_audio_dsp_retire_begin(FhAudioDspRetirement *,unsigned channel);
/* Only inside the SDK frame callback; no pool/allocator/file IO/application
 * lock operations. The SDK's own channel locking is retained. */
void fh_audio_dsp_retire_service(FhAudioDspRetirement *);
/* completed_channels remains valid even if a later channel reports an error. */
int fh_audio_dsp_retire_drain(FhAudioDspRetirement *,uint32_t *completed_channels);
uint32_t fh_audio_dsp_retire_pending(const FhAudioDspRetirement *);
#endif
