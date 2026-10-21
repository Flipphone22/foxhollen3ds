#ifndef FH_AUDIO_RAMP_H
#define FH_AUDIO_RAMP_H
#include <stdint.h>
typedef struct { int32_t value,step,sign;uint32_t remainder,increment,denominator; } FhRamp;
static inline FhRamp fh_ramp(int32_t start,int32_t end,uint32_t denominator,uint32_t offset) {
    if(start==end)return (FhRamp){start,0,1,0,0,denominator};
    int32_t delta=end-start;uint32_t magnitude=delta<0?-delta:delta;
    uint32_t position=magnitude*offset;
    int32_t sign=delta<0?-1:1;
    return (FhRamp){start+sign*(int32_t)(position/denominator),sign*(int32_t)(magnitude/denominator),sign,position%denominator,magnitude%denominator,denominator};
}
static inline void fh_ramp_next(FhRamp *r) {
    r->value+=r->step;r->remainder+=r->increment;
    if(r->remainder>=r->denominator){r->remainder-=r->denominator;r->value+=r->sign;}
}
static inline int32_t fh_mix_ramp(int32_t *dest,const int32_t *src,int samples,int16_t start,int16_t end,uint16_t env_start,uint16_t env_end,int offset,unsigned cycle) {
    if(!dest || samples<=0 || (!start && !end))return 0;
    if(start==end && env_start==env_end) {
        int32_t mix=((int32_t)start*env_start)>>15,contribution=0;
        for(int i=0;i<samples;i++){contribution=(src[i]*mix)>>15;dest[i]+=contribution;}
        return contribution;
    }
    if(start==end){
        FhRamp envelope=fh_ramp(env_start,env_end,samples,0);int32_t contribution=0;
        for(int i=0;i<samples;i++){
            int32_t mix=((int32_t)start*envelope.value)>>15;
            contribution=(src[i]*mix)>>15;dest[i]+=contribution;fh_ramp_next(&envelope);
        }
        return contribution;
    }
    if(env_start==env_end){
        FhRamp volume=fh_ramp(start,end,cycle,offset);int32_t contribution=0;
        for(int i=0;i<samples;i++){
            int32_t mix=(volume.value*env_start)>>15;
            contribution=(src[i]*mix)>>15;dest[i]+=contribution;fh_ramp_next(&volume);
        }
        return contribution;
    }
    FhRamp volume=fh_ramp(start,end,cycle,offset),envelope=fh_ramp(env_start,env_end,samples,0);
    int32_t contribution=0;
    for(int i=0;i<samples;i++) {
        int32_t mix=(volume.value*envelope.value)>>15;
        contribution=(src[i]*mix)>>15;dest[i]+=contribution;
        fh_ramp_next(&volume);fh_ramp_next(&envelope);
    }
    return contribution;
}
/* One voice envelope is shared by all eleven destination channels. Keep the
 * signed division/truncation sequence identical while calculating it once. */
static inline void fh_audio_envelope(int32_t *out,int samples,uint16_t start,uint16_t end) {
    if(samples<=0)return;
    FhRamp r=fh_ramp(start,end,samples,0);
    for(int i=0;i<samples;i++){out[i]=r.value;fh_ramp_next(&r);}
}
static inline int32_t fh_mix_envelope(int32_t *dest,const int32_t *src,int samples,int16_t start,int16_t end,const int32_t *env,int offset,unsigned cycle) {
    if(!dest || samples<=0 || (!start && !end))return 0;
    int32_t contribution=0;
    if(start==end) {
        for(int i=0;i<samples;i++){
            int32_t mix=((int32_t)start*env[i])>>15;
            contribution=(src[i]*mix)>>15;dest[i]+=contribution;
        }
    } else {
        FhRamp volume=fh_ramp(start,end,cycle,offset);
        for(int i=0;i<samples;i++){
            int32_t mix=(volume.value*env[i])>>15;
            contribution=(src[i]*mix)>>15;dest[i]+=contribution;fh_ramp_next(&volume);
        }
    }
    return contribution;
}
static inline void fh_depop_tail(int32_t *ring,unsigned mask,unsigned cursor,unsigned frames,int32_t last) {
    if(!last)return;
    uint32_t magnitude=last<0?0u-(uint32_t)last:(uint32_t)last;
    uint32_t step=magnitude/frames,increment=magnitude%frames,value=0,remainder=0;
    for(unsigned i=0;i<frames;i++) {
        ring[(cursor+frames-i-1)&mask]+=last<0?-(int32_t)value:(int32_t)value;
        value+=step;remainder+=increment;
        if(remainder>=frames){remainder-=frames;++value;}
    }
}
#endif
