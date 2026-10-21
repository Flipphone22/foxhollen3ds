#ifndef FH_AUDIO_SAMPLE_REGISTRY_H
#define FH_AUDIO_SAMPLE_REGISTRY_H
#include "fh_audio_sample_pool.h"
#include "musyx/synthdata.h"

enum { FH_AUDIO_REGISTRY_ENTRIES=64 };
typedef struct {
    const void *key; /* Exact source SDIR_DATA record, invalidated before free. */
    SAMPLE_INFO sample;
    FhAudioPoolToken bank,entry;
    uint64_t use;
} FhAudioSampleRegistration;
typedef struct {
    FhAudioSamplePool *pool;
    FhAudioPoolFlush flush;
    void *flush_context;
    uint64_t clock;
    unsigned prepared,hits,declined,forgotten,evicted,faults;
    FhAudioSampleRegistration entries[FH_AUDIO_REGISTRY_ENTRIES];
} FhAudioSampleRegistry;

/* Optional candidate loader integration. NULL leaves original loading intact.
 * Set/clear under the same single producer/recursive synth lock as sample loads.
 * NDSP callbacks never access the registry or original asset pointers.
 * Zero initialize once, then init with an already initialized bounded pool.
 * Each cache residency receives a fresh bank incarnation. Forget is REQUIRED
 * before source refcount reaches zero/free; reset precedes source registry reset.
 * Original inputs are immutable while registered. Eviction closes an incarnation
 * but retains DSP-pinned copies until the voice owner's acknowledgement.
 * No original file/asset mutation, allocation, background work or hidden wait.
 */
extern FhAudioSampleRegistry *fh_audio_native_samples;
int fh_audio_sample_registry_init(FhAudioSampleRegistry *,FhAudioSamplePool *,FhAudioPoolFlush,void *);
int fh_audio_sample_registry_prepare(FhAudioSampleRegistry *,const void *key,const SAMPLE_INFO *);
int fh_audio_sample_registry_find(FhAudioSampleRegistry *,const SAMPLE_INFO *,FhAudioPoolToken *bank);
int fh_audio_sample_registry_forget(FhAudioSampleRegistry *,const void *key);
int fh_audio_sample_registry_reset(FhAudioSampleRegistry *);
/* Returned bank uses pool sample ID zero. Find accepts only a current, successful
 * source dataGetSample result, not a stale SAMPLE_INFO retained across unload.
 * Find/start are serialized with pool
 * operations; voice start still acquires its own lease and may report failure.
 * Declined preparation MUST retain software playback. Admission to native voice
 * processing requires the timing/ADSR/AUX bridge, not just this cache lookup. */
#endif
