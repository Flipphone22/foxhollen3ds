#include "fh_audio_aux_rate.h"
#include <limits.h>
#include <string.h>
/* SYSCLOCK_SOC is16756991 in the pinned SDK; exact integer clock ratio. */
enum { SOURCE_STEP=32000*512, DSP_STEP=16756991 };
#ifdef __3DS__
#include <3ds/os.h>
_Static_assert(SYSCLOCK_SOC==DSP_STEP,"AUX adapter requires the pinned DSP clock");
#endif
static int32_t blend_weight(int32_t a,int32_t b,float t) {
    float x=(1.0f-t)*(float)a+t*(float)b;
    if(x>=2147483648.0f)return INT32_MAX;
    if(x<=-2147483648.0f)return INT32_MIN;
    return (int32_t)x;
}
static int32_t blend(int32_t a,int32_t b,unsigned fraction,unsigned denominator) {
    return blend_weight(a,b,(float)fraction/(float)denominator);
}
static int process_channels(FhAudioAuxRate *r,int32_t p[4][160],FhAudioAux32Process process,void *context,unsigned channels) {
    if(!r || !p || !process)return 0;
    unsigned frames=0;
    for(unsigned i=0;i<160;i++) {
        if(r->input_phase<=0) {
            for(unsigned c=0;c<channels;c++)r->work[c][frames+2]=
                blend(r->previous[c],p[c][i],r->input_phase+SOURCE_STEP,SOURCE_STEP);
            frames++;r->input_phase+=DSP_STEP;
        }
        for(unsigned c=0;c<channels;c++)r->previous[c]=p[c][i];
        r->input_phase-=SOURCE_STEP;
    }
    process(context,r->work[0]+2,r->work[1]+2,r->work[2]+2,frames);
    for(unsigned i=0;i<160;i++) {
        int32_t value[3]={0};
        if(r->initial_silence<2)r->initial_silence++;
        else {
            /* Two retained source samples suffice at each callback boundary. */
            int64_t at=(int64_t)r->output_source-(int64_t)r->source_frames+2;
            if(at<0 || at+1>=frames+2) {
                r->faults++;memset(p,0,4*160*sizeof(int32_t));return 0;
            }
            for(unsigned c=0;c<channels;c++)value[c]=blend(r->work[c][at],r->work[c][at+1],r->output_phase,DSP_STEP);
            r->output_phase+=SOURCE_STEP;
            if(r->output_phase>=DSP_STEP){r->output_phase-=DSP_STEP;r->output_source++;}
        }
        int64_t surround=(int64_t)value[2]*23170>>15;
        for(unsigned c=0;c<2;c++) {
            int64_t sum=(int64_t)value[c]+surround;
            p[c][i]=sum>INT32_MAX?INT32_MAX:sum<INT32_MIN?INT32_MIN:(int32_t)sum;
        }
        p[2][i]=p[3][i]=0;
    }
    for(unsigned c=0;c<channels;c++) {
        r->work[c][0]=r->work[c][frames];r->work[c][1]=r->work[c][frames+1];
    }
    r->source_frames+=frames;r->blocks++;return 1;
}
int fh_audio_aux_rate_process(FhAudioAuxRate *r,int32_t p[4][160],FhAudioAux32Process process,void *context) {
    return process_channels(r,p,process,context,3);
}
int fh_audio_aux_rate_stereo(FhAudioAuxRate *r,int32_t p[4][160],FhAudioAux32Process process,void *context) {
    if(!r || !p || !process)return 0;
    /* DPL2's return is independently stereo. Share the exact interpolation
     * fraction, preserve floating operation order, and never process/fold S.
     * The generic three-channel route and callback ownership stay unchanged. */
    unsigned frames=0;
    for(unsigned i=0;i<160;i++) {
        if(r->input_phase<=0) {
            float t=(float)(r->input_phase+SOURCE_STEP)/(float)SOURCE_STEP;
            r->work[0][frames+2]=blend_weight(r->previous[0],p[0][i],t);
            r->work[1][frames+2]=blend_weight(r->previous[1],p[1][i],t);
            frames++;r->input_phase+=DSP_STEP;
        }
        r->previous[0]=p[0][i];r->previous[1]=p[1][i];
        r->input_phase-=SOURCE_STEP;
    }
    process(context,r->work[0]+2,r->work[1]+2,r->work[2]+2,frames);
    for(unsigned i=0;i<160;i++) {
        if(r->initial_silence<2) {
            r->initial_silence++;p[0][i]=p[1][i]=0;
        } else {
            int64_t at=(int64_t)r->output_source-(int64_t)r->source_frames+2;
            if(at<0 || at+1>=frames+2) {
                r->faults++;memset(p,0,4*160*sizeof(int32_t));return 0;
            }
            float t=(float)r->output_phase/(float)DSP_STEP;
            p[0][i]=blend_weight(r->work[0][at],r->work[0][at+1],t);
            p[1][i]=blend_weight(r->work[1][at],r->work[1][at+1],t);
            r->output_phase+=SOURCE_STEP;
            if(r->output_phase>=DSP_STEP){r->output_phase-=DSP_STEP;r->output_source++;}
        }
    }
    memset(p[2],0,2*160*sizeof(int32_t));
    r->work[0][0]=r->work[0][frames];r->work[0][1]=r->work[0][frames+1];
    r->work[1][0]=r->work[1][frames];r->work[1][1]=r->work[1][frames+1];
    r->source_frames+=frames;r->blocks++;return 1;
}
