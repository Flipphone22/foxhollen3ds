#ifndef FH_AUDIO_CACHE_H
#define FH_AUDIO_CACHE_H
#include <stddef.h>
#include <stdint.h>
/* Entire-range proof, avoiding pointer ordering UB and end-address overflow. */
static inline unsigned fh_audio_range_contains(const void *base,size_t capacity,const void *address,size_t bytes){
    uintptr_t first=(uintptr_t)base,at=(uintptr_t)address;
    return first && at && bytes && at>=first && at-first<capacity && bytes<=capacity-(at-first);
}
extern unsigned fh_audio_cpu_cache_enabled,fh_audio_cpu_cache_hits,fh_audio_cpu_cache_fallbacks;
void fh_audio_dma_flush(void *memory,uint32_t bytes);
void fh_audio_dma_invalidate(void *memory,uint32_t bytes);
#endif
