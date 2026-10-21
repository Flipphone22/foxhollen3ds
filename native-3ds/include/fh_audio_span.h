#ifndef FH_AUDIO_SPAN_H
#define FH_AUDIO_SPAN_H
#include "hw_pc_decode.h"
/* A cursor into the current ADPCM block, valid only within one synchronous
 * resampling call. Decode no samples ahead of consumption. The generic reader
 * handles block headers, loop history, source changes and end-of-sample. */
typedef struct {
    u32 remaining,within;
    const u8 *block;
    s16 predictor[2];
    s32 scale;
} FhPCMSpan;
static inline s16 fh_pcm_span_read(FhPCMSpan *span,MusyPCMReader *reader,const SAMPLE_INFO *sample,u8 loopPS) {
    if(span->remaining) {
        u32 within=span->within++;--span->remaining;++reader->position;
        u8 nibble=(span->block[1+within/2]>>(within%2?0:4))&15;
        s32 value=nibble>=8?(s32)nibble-16:nibble;
        s64 sum=value*span->scale+1024;
#if defined(__arm__) && !defined(__thumb__) && __ARM_ARCH>=6 && __BYTE_ORDER__==__ORDER_LITTLE_ENDIAN__
        /* Little-endian pairs preserve each signed predictor/history lane.
         * The long accumulator is required: their sum can exceed int32. */
        _Static_assert(offsetof(MusyPCMReader,yn2)==offsetof(MusyPCMReader,yn1)+2,"Packed signed history lanes");
        u32 predictors,history,lo=(u32)sum,hi=(u32)(sum>>32);
        __builtin_memcpy(&predictors,span->predictor,4);
        __builtin_memcpy(&history,&reader->yn1,4);
        __asm__("smlald %0, %1, %2, %3" : "+&r"(lo), "+&r"(hi) : "r"(predictors), "r"(history));
        sum=(s64)(((u64)hi<<32)|lo);
#else
        sum+=(s64)span->predictor[0]*reader->yn1+(s64)span->predictor[1]*reader->yn2;
#endif
        s32 scaled=(s32)(sum>>11);
        s16 result=scaled<-32768?-32768:scaled>32767?32767:scaled;
        reader->yn2=reader->yn1;reader->yn1=result;return result;
    }
    s16 value=salPCReadSample(reader,sample,loopPS);
    if(!reader->ended && (sample->compType==SAMPLE_TYPE_ADPCM || sample->compType==SAMPLE_TYPE_ADPCM_PLUS || sample->compType==SAMPLE_TYPE_ADPCM_STREAM || sample->compType==SAMPLE_TYPE_ADPCM_VIRTUAL)) {
        u32 position=reader->position,within=position%SND_STREAM_ADPCM_BLKSIZE;
        u32 end=sample->loopLength?sample->loop+sample->loopLength:sample->length;
        if(within && position<end) {
            span->within=within;span->remaining=SND_STREAM_ADPCM_BLKSIZE-within;
            if(span->remaining>end-position)span->remaining=end-position;
            span->block=(const u8*)sample->addr+(position/SND_STREAM_ADPCM_BLKSIZE)*SND_STREAM_ADPCM_BLKBYTES;
            const DSPADPCMplusInfo *info=sample->extraData;
            unsigned predictor=(reader->predictorScale>>4)&7;
            span->predictor[0]=info?info->coefTab[predictor][0]:0;
            span->predictor[1]=info?info->coefTab[predictor][1]:0;
            span->scale=(1u<<(reader->predictorScale&15))*2048;
        }
    }
    return value;
}
#endif
