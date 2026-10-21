#ifndef FH_AUDIO_SAMPLE_POOL_H
#define FH_AUDIO_SAMPLE_POOL_H
#include "fh_audio_sample_plan.h"

enum { FH_AUDIO_POOL_SAMPLES=64,FH_AUDIO_POOL_BANKS=64,FH_AUDIO_POOL_LEASES=128,
       FH_AUDIO_POOL_MAX_BYTES=2*1024*1024 };
enum { FH_AUDIO_POOL_OK=0,FH_AUDIO_POOL_INVALID=1,FH_AUDIO_POOL_FULL=2,
       FH_AUDIO_POOL_UNPUBLISHED=3,FH_AUDIO_POOL_BUSY=4,FH_AUDIO_POOL_FLUSH_FAILED=5 };
typedef uint64_t FhAudioPoolToken;
typedef struct {
    FhAudioPoolToken token,bank;
    FhAudioDspSamplePlan plan;
    uint64_t last_use;
    uint32_t offset,bytes,sample;
    uint16_t pins;
    uint8_t state; /* 0 empty, 1 prepared, 2 published, 3 retired */
} FhAudioPoolEntry;
typedef struct { FhAudioPoolToken token,entry; } FhAudioPoolLease;
typedef struct {
    void *arena;
    uint64_t serial,clock;
    uint32_t capacity,used,peak,compiles,hits,evictions,flush_failures;
    unsigned closing;
    FhAudioPoolEntry entries[FH_AUDIO_POOL_SAMPLES];
    FhAudioPoolToken banks[FH_AUDIO_POOL_BANKS];
    FhAudioPoolLease leases[FH_AUDIO_POOL_LEASES];
} FhAudioSamplePool;
typedef int (*FhAudioPoolFlush)(void *context,const void *data,size_t bytes);

/* Zero-initialize once. All calls are serialized by one owner, including the
 * synchronous flush callback, which must not reenter this API. No allocation,
 * background thread, CPU/DSP publication or cache operation is hidden here.
 * Caller provides a bounded 32-byte-aligned arena and immutable bank inputs.
 * Each bank token denotes a NEW immutable incarnation, never a pointer key.
 * Stream/virtual source admission remains the game adapter's responsibility.
 */
int fh_audio_pool_init(FhAudioSamplePool *,void *arena,size_t bytes);
int fh_audio_pool_bank_open(FhAudioSamplePool *,FhAudioPoolToken *bank);
int fh_audio_pool_bank_close(FhAudioSamplePool *,FhAudioPoolToken bank);
int fh_audio_pool_prepare(FhAudioSamplePool *,FhAudioPoolToken bank,uint32_t sample,
                          const FhAudioSampleInput *,FhAudioPoolToken *entry);
int fh_audio_pool_publish(FhAudioSamplePool *,FhAudioPoolToken entry,
                          FhAudioPoolFlush,void *context);
int fh_audio_pool_acquire(FhAudioSamplePool *,FhAudioPoolToken bank,uint32_t sample,
                          FhAudioPoolToken *lease);
/* Borrowed pointers remain valid until this exact lease is released. One lease
 * can cover a voice's first/loop buffers. Release only after authoritative DSP
 * completion/cancellation, NOT a timeout or a requested stop. Duplicate releases
 * and stale generations are rejected. Closing a bank prevents new acquisitions
 * but does not move or free bytes pinned by queued/playing/retiring voices. */
int fh_audio_pool_view(FhAudioSamplePool *,FhAudioPoolToken lease,
                       const void **data,const FhAudioDspSamplePlan **plan);
int fh_audio_pool_release(FhAudioSamplePool *,FhAudioPoolToken lease);
/* Begins retirement. BUSY means the arena MUST remain allocated. After OK the
 * arena can be freed. Serial is retained across reinit to reject old handles. */
int fh_audio_pool_shutdown(FhAudioSamplePool *);
#endif
