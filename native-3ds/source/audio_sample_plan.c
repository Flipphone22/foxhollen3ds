#include "fh_audio_sample_plan.h"
#include <string.h>

static int overlaps(const void *a,size_t na,const void *b,size_t nb) {
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(na>UINTPTR_MAX-x || nb>UINTPTR_MAX-y)return 1;
    return x<y+nb && y<x+na;
}
int fh_audio_sample_measure(const FhAudioSampleInput *in,FhAudioSamplePlan *out) {
    if(!in || !out || !in->encoded || !in->length ||
       in->loop_start>in->length || in->loop_length>in->length-in->loop_start)
        return FH_AUDIO_PLAN_INVALID;
    uint32_t first=in->loop_length?in->loop_start+in->loop_length:in->length;
    uint64_t encoded=((uint64_t)first+13)/14*8;
    uint64_t offset=in->loop_length?((uint64_t)first+15)&~UINT64_C(15):first;
    uint64_t count=offset+in->loop_length;
    if(encoded>in->encoded_bytes || count>UINT32_MAX || count>SIZE_MAX/2 ||
       overlaps(out,sizeof(*out),in,sizeof(*in)) ||
       overlaps(out,sizeof(*out),in->encoded,in->encoded_bytes))
        return FH_AUDIO_PLAN_INVALID;
    *out=(FhAudioSamplePlan){first,(uint32_t)offset,in->loop_length,(uint32_t)count};
    return FH_AUDIO_PLAN_OK;
}
static void decode(const FhAudioSampleInput *in,uint32_t start,uint32_t count,
                   int16_t y1,int16_t y2,uint8_t ps,int16_t *out) {
    for(uint32_t i=0;i<count;i++) {
        uint32_t pos=start+i,within=pos%14;
        const uint8_t *block=in->encoded+(size_t)(pos/14)*8;
        if(i && !within)ps=block[0];
        unsigned raw=(block[1+within/2]>>(within%2?0:4))&15;
        int nibble=raw>=8?(int)raw-16:(int)raw;
        unsigned predictor=(ps>>4)&7;
        int64_t value=(int64_t)nibble*(1u<<(ps&15))*2048+1024;
        value+=(int64_t)in->coefficients[2*predictor]*y1+
               (int64_t)in->coefficients[2*predictor+1]*y2;
        value=value<0?-((-value+2047)/2048):value/2048;
        int16_t sample=value<-32768?-32768:value>32767?32767:(int16_t)value;
        y2=y1;y1=sample;out[i]=sample;
    }
}
int fh_audio_sample_compile(const FhAudioSampleInput *in,int16_t *pcm,
                           size_t capacity,FhAudioSamplePlan *out) {
    FhAudioSamplePlan plan;
    if(!out || !pcm || ((uintptr_t)pcm&31) ||
       fh_audio_sample_measure(in,&plan)!=FH_AUDIO_PLAN_OK)return FH_AUDIO_PLAN_INVALID;
    if(capacity<plan.storage_samples)return FH_AUDIO_PLAN_CAPACITY;
    size_t bytes=(size_t)plan.storage_samples*2;
    if(overlaps(pcm,bytes,in,sizeof(*in)) ||
       overlaps(pcm,bytes,in->encoded,in->encoded_bytes) ||
       overlaps(pcm,bytes,out,sizeof(*out)) || overlaps(out,sizeof(*out),in,sizeof(*in)) ||
       overlaps(out,sizeof(*out),in->encoded,in->encoded_bytes))return FH_AUDIO_PLAN_INVALID;
    decode(in,0,plan.first_samples,0,0,in->initial_ps,pcm);
    if(plan.loop_samples) {
        memset(pcm+plan.first_samples,0,(plan.loop_offset-plan.first_samples)*2);
        decode(in,in->loop_start,plan.loop_samples,in->loop_y1,in->loop_y2,
               in->loop_ps,pcm+plan.loop_offset);
    }
    *out=plan;
    return FH_AUDIO_PLAN_OK;
}
int fh_audio_dsp_sample_measure(const FhAudioSampleInput *in,FhAudioDspSamplePlan *out) {
    FhAudioSamplePlan pcm;
    if(!out || fh_audio_sample_measure(in,&pcm)!=FH_AUDIO_PLAN_OK ||
       overlaps(out,sizeof(*out),in,sizeof(*in)) ||
       overlaps(out,sizeof(*out),in->encoded,in->encoded_bytes))return FH_AUDIO_PLAN_INVALID;
    FhAudioDspSamplePlan result={0};
    result.first_samples=pcm.first_samples;result.loop_samples=pcm.loop_samples;
    int direct=in->initial_ps==in->encoded[0];
    if(pcm.loop_samples)direct=direct && !(in->loop_start%14) &&
        in->loop_ps==in->encoded[(size_t)(in->loop_start/14)*8];
    if(direct) {
        result.format=FH_AUDIO_DSP_ADPCM;
        result.storage_bytes=((uint64_t)pcm.first_samples+13)/14*8;
        result.loop_offset_bytes=pcm.loop_samples?(in->loop_start/14)*8:0;
        result.loop_y1=in->loop_y1;result.loop_y2=in->loop_y2;
        memcpy(result.coefficients,in->coefficients,sizeof(result.coefficients));
    } else {
        if(pcm.storage_samples>UINT32_MAX/2)return FH_AUDIO_PLAN_INVALID;
        result.format=FH_AUDIO_DSP_PCM16;result.storage_bytes=pcm.storage_samples*2;
        result.loop_offset_bytes=pcm.loop_samples?pcm.loop_offset*2:0;
    }
    *out=result;return FH_AUDIO_PLAN_OK;
}
int fh_audio_dsp_sample_compile(const FhAudioSampleInput *in,void *storage,
                               size_t capacity,FhAudioDspSamplePlan *out) {
    FhAudioDspSamplePlan plan;
    if(!out || !storage || ((uintptr_t)storage&31) ||
       fh_audio_dsp_sample_measure(in,&plan)!=FH_AUDIO_PLAN_OK)return FH_AUDIO_PLAN_INVALID;
    if(capacity<plan.storage_bytes)return FH_AUDIO_PLAN_CAPACITY;
    if(overlaps(storage,plan.storage_bytes,in,sizeof(*in)) ||
       overlaps(storage,plan.storage_bytes,in->encoded,in->encoded_bytes) ||
       overlaps(storage,plan.storage_bytes,out,sizeof(*out)) ||
       overlaps(out,sizeof(*out),in,sizeof(*in)) ||
       overlaps(out,sizeof(*out),in->encoded,in->encoded_bytes))return FH_AUDIO_PLAN_INVALID;
    if(plan.format==FH_AUDIO_DSP_ADPCM)memcpy(storage,in->encoded,plan.storage_bytes);
    else {
        FhAudioSamplePlan pcm;
        int status=fh_audio_sample_compile(in,storage,capacity/2,&pcm);
        if(status!=FH_AUDIO_PLAN_OK)return status;
    }
    *out=plan;return FH_AUDIO_PLAN_OK;
}
