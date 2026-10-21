#include "fh_audio_aux_rate.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
static FhAudioAuxRate rate;
static int32_t block[4][160];
static uint64_t source_count;
static double worst_input,worst_output;
static void passthrough(void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n) {
    (void)p;(void)l;(void)r;(void)s;assert(n==156 || n==157);
}
static void identity(void *unused,int32_t *l,int32_t *r,int32_t *s,unsigned frames) {
    (void)unused;assert(frames==156 || frames==157);
    for(unsigned i=0;i<frames;i++) {
        /* Linear ramp is invariant under interpolation; independent absolute
         * rational coordinates, not the implementation's phase recurrence. */
        double t=(double)(source_count+i)*16756991.0/(32000.0*512.0);
        double expected=1000.0+fmod(t,1000000.0)*.125;
        if(t<999990) {
            double error=fabs(l[i]-expected);if(error>worst_input)worst_input=error;
            assert(error<2.1);
        }
        assert(r[i]==0 && s[i]==0);
    }
    source_count+=frames;
}
int main(void) {
    /*4096 callbacks cross thousands of input/output phase and partition boundaries. */
    for(unsigned b=0;b<4096;b++) {
        memset(block,0,sizeof(block));
        for(unsigned i=0;i<160;i++)block[0][i]=1000+(b*160+i)/8;
        assert(fh_audio_aux_rate_process(&rate,block,identity,NULL));
        for(unsigned i=0;i<160;i++) {
            unsigned at=b*160+i;
            if(at<2){assert(block[0][i]==0);continue;}
            double expected=1000+(at-2)*.125;
            double error=fabs(block[0][i]-expected);if(error>worst_output)worst_output=error;
            assert(error<3.1 && block[1][i]==0 && block[2][i]==0 && block[3][i]==0);
        }
        assert(rate.source_frames==((uint64_t)(b+1)*160-1)*32000*512/16756991+1);
    }
    assert(!rate.faults);
    for(unsigned test=0;test<4;test++) {
        FhAudioAuxRate other={0};
        int32_t values[4][3]={{INT32_MAX,INT32_MIN,INT32_MAX},{-12345,23456,-32768},{32767,32767,32767},{0,0,0}};
        for(unsigned b=0;b<10;b++) {
            for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)block[c][i]=c<3?values[test][c]:0;
            assert(fh_audio_aux_rate_process(&other,block,passthrough,NULL));
            if(!b)continue;
            for(unsigned c=0;c<2;c++)for(unsigned i=0;i<160;i++) {
                int64_t target=values[test][c]+((int64_t)values[test][2]*23170>>15);
                if(target>INT32_MAX)target=INT32_MAX;if(target<INT32_MIN)target=INT32_MIN;
                assert(fabs((double)block[c][i]-target)<=256);
            }
        }
        assert(!other.faults);
    }
    printf("{\"blocks\":%u,\"source_frames\":%llu,\"input_max_error\":%.6f,\"output_max_error\":%.6f,\"owner_bytes\":%zu}\n",
        rate.blocks,(unsigned long long)rate.source_frames,worst_input,worst_output,sizeof(rate));
}
