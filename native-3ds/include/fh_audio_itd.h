#ifndef FH_AUDIO_ITD_H
#define FH_AUDIO_ITD_H
#include "fh_audio_ramp.h"
#include "fh_audio_lerp.h"
#include <string.h>
static inline void fh_audio_itd(int32_t *left,int32_t *right,const int32_t *src,int16_t *history,uint32_t *cursor,const uint32_t delay[2],const uint32_t target[2],unsigned samples,unsigned offset,unsigned cycle) {
    if(!(delay[0]|delay[1]|target[0]|target[1])) {
        for(unsigned i=0;i<samples;i++){int16_t value=src[i];history[(*cursor+i)&127]=value;left[i]=right[i]=value;}
        *cursor+=samples;return;
    }
    /* At the fixed 32 kHz mixer rate, unchanged integer-sample ITD targets
     * need ring reads only. Still write every history sample and advance the
     * cursor exactly, including wraparound; changing/fractional delays retain
     * the original ramp and truncation path below. */
    if(delay[0]==target[0] && delay[1]==target[1] && !((delay[0]|delay[1])&65535)) {
        unsigned l=delay[0]>>16,r=delay[1]>>16,index=*cursor;
        for(unsigned i=0;i<samples;i++,index++){
            history[index&127]=src[i];
            left[i]=history[(index-l)&127];right[i]=history[(index-r)&127];
        }
        *cursor=index;return;
    }
    /* Positional panning commonly leaves one ear at a fixed integer delay.
     * Keep that ear's exact ring read; only the changing ear needs a ramp and
     * fractional interpolation. Both outputs still observe the same history
     * write, including wraparound and the most recent sample at delay zero. */
    unsigned fixed=delay[0]==target[0] && !(delay[0]&65535)?0:
                   delay[1]==target[1] && !(delay[1]&65535)?1:2;
    if(fixed<2) {
        unsigned other=fixed^1,shift=delay[fixed]>>16,index=*cursor;
        int32_t *steady=fixed?right:left,*moving=other?right:left;
        FhRamp ramp=fh_ramp(delay[other],target[other],cycle,offset);
        for(unsigned i=0;i<samples;i++,index++) {
            history[index&127]=src[i];steady[i]=history[(index-shift)&127];
            unsigned at=index-((unsigned)ramp.value>>16);
            moving[i]=fh_audio_lerp16(history[at&127],history[(at-1)&127],ramp.value&65535);
            fh_ramp_next(&ramp);
        }
        *cursor=index;return;
    }
    FhRamp ramps[2]={fh_ramp(delay[0],target[0],cycle,offset),fh_ramp(delay[1],target[1],cycle,offset)};
    int32_t *dest[2]={left,right};
    for(unsigned i=0;i<samples;i++) {
        history[*cursor&127]=src[i];
        for(unsigned c=0;c<2;c++) {
            unsigned d=ramps[c].value,index=*cursor-(d>>16);
            int32_t newer=history[index&127],older=history[(index-1)&127];
            dest[c][i]=fh_audio_lerp16(newer,older,d&65535);
            fh_ramp_next(&ramps[c]);
        }
        ++*cursor;
    }
}
/* The voice resampler guarantees signed-16-bit output, held in int32_t slots.
 * At zero delay both ears can consume that immutable source directly. Preserve
 * the ring and cursor for later delay changes; only overwritten ring writes
 * and the redundant temporary ear copies disappear. */
static inline void fh_audio_itd_prepare(const int32_t *out[2],int32_t *left,int32_t *right,const int32_t *src,int16_t *history,uint32_t *cursor,const uint32_t delay[2],const uint32_t target[2],unsigned samples,unsigned offset,unsigned cycle) {
    if(!(delay[0]|delay[1]|target[0]|target[1])) {
        unsigned first=samples>128?samples-128:0,index=*cursor;
        for(unsigned i=first;i<samples;i++)history[(index+i)&127]=src[i];
        *cursor=index+samples;out[0]=out[1]=src;
    } else {
        fh_audio_itd(left,right,src,history,cursor,delay,target,samples,offset,cycle);
        out[0]=left;out[1]=right;
    }
}
#endif
