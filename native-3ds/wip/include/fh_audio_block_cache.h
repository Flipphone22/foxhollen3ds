#ifndef FH_AUDIO_BLOCK_CACHE_H
#define FH_AUDIO_BLOCK_CACHE_H
#include <string.h>
/* Experimental content-addressed static ADPCM blocks. A span lives for one
 * synchronous resample call under the existing synth lock. Streaming/virtual
 * sources never enter this cache. No sample is decoded before consumption. */
#define FhPCMSpan FhPCMSpanUncached
#define fh_pcm_span_read fh_pcm_span_read_uncached
#include "fh_audio_span.h"
#undef fh_pcm_span_read
#undef FhPCMSpan

typedef struct {
    u32 key[5]; /* encoded bytes, coefficients, initial history, scale */
    s16 samples[13];
    unsigned char valid,count;
} FhAudioBlockEntry;
static FhAudioBlockEntry fh_audio_blocks[64];
static u32 fh_audio_block_requests,fh_audio_block_hits,fh_audio_cached_samples;
typedef struct {
    FhPCMSpanUncached base;
    FhAudioBlockEntry *entry;
    unsigned consumed;
} FhPCMSpan;

static inline s16 fh_pcm_span_read(FhPCMSpan *span,MusyPCMReader *reader,
                                  const SAMPLE_INFO *sample,u8 loopPS) {
    if(span->base.remaining) {
        FhAudioBlockEntry *entry=span->entry;
        if(entry && span->consumed<entry->count) {
            s16 result=entry->samples[span->consumed++];
            --span->base.remaining;++span->base.within;++reader->position;
            reader->yn2=reader->yn1;reader->yn1=result;
            ++fh_audio_cached_samples;return result;
        }
        s16 result=fh_pcm_span_read_uncached(&span->base,reader,sample,loopPS);
        if(entry) {
            entry->samples[span->consumed++]=result;
            entry->count=span->consumed;
        }
        return result;
    }
    span->entry=NULL;span->consumed=0;
    s16 result=fh_pcm_span_read_uncached(&span->base,reader,sample,loopPS);
    if(span->base.within==1 && span->base.remaining==13 &&
       (sample->compType==SAMPLE_TYPE_ADPCM || sample->compType==SAMPLE_TYPE_ADPCM_PLUS)) {
        u32 key[5];
        __builtin_memcpy(key,span->base.block,8);
        __builtin_memcpy(key+2,span->base.predictor,4);
        __builtin_memcpy(key+3,&reader->yn1,4);
        key[4]=(u32)span->base.scale;
        u32 hash=key[0]^(key[1]*0x9e3779b9u)^key[2]^key[3]^key[4];
        hash^=hash>>16;hash^=hash>>8;
        FhAudioBlockEntry *entry=fh_audio_blocks+(hash&63);
        ++fh_audio_block_requests;
        if(entry->valid && !memcmp(entry->key,key,sizeof(key)))++fh_audio_block_hits;
        else {memcpy(entry->key,key,sizeof(key));entry->count=0;entry->valid=1;}
        span->entry=entry;
    }
    return result;
}
#endif
