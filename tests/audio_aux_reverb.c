#include "musyx/musyx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *salMalloc(size_t bytes){return calloc(1,bytes);}
void salFree(void *p){free(p);}
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern void ReferenceReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static _SND_REVSTD_WORK a,b;
static s32 pcm[3][160],other[3][160];
int main(void) {
    unsigned random=0x52931,partitions=0;
    for(unsigned test=0;test<4;test++) {
        assert(ReverbSTDCreate(&a,.2f+test*.2f,.5f+test,.8f,.3f,test*.01f));
        assert(ReverbSTDCreate(&b,.2f+test*.2f,.5f+test,.8f,.3f,test*.01f));
        for(unsigned cycle=0;cycle<200;cycle++) {
            for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++) {
                random=random*1664525+1013904223;pcm[c][i]=cycle<40?(int)(random%100001)-50000:0;
            }
            memcpy(other,pcm,sizeof(pcm));ReferenceReverbSTDCallback(pcm[0],pcm[1],pcm[2],&a);
            for(unsigned at=0;at<160;) {
                random=random*1664525+1013904223;unsigned n=1+random%160;
                if(n>160-at)n=160-at;
                assert(fh_audio_reverb_std_frames(other[0]+at,other[1]+at,other[2]+at,n,&b));
                at+=n;partitions++;
            }
            assert(!memcmp(pcm,other,sizeof(pcm)));
        }
        for(unsigned i=0;i<6;i++) {
            assert(a.C[i].inPoint==b.C[i].inPoint && a.C[i].outPoint==b.C[i].outPoint);
            assert(a.AP[i].inPoint==b.AP[i].inPoint && a.AP[i].outPoint==b.AP[i].outPoint);
            assert(!memcmp(a.C[i].inputs,b.C[i].inputs,a.C[i].length));
            assert(!memcmp(a.AP[i].inputs,b.AP[i].inputs,a.AP[i].length));
        }
        assert(!memcmp(a.lpLastout,b.lpLastout,sizeof(a.lpLastout)));
        for(unsigned i=0;i<3;i++)if(a.preDelayTime) {
            assert(a.preDelayPtr[i]-a.preDelayLine[i]==b.preDelayPtr[i]-b.preDelayLine[i]);
            assert(!memcmp(a.preDelayLine[i],b.preDelayLine[i],a.preDelayTime*sizeof(float)));
        }
        ReverbSTDFree(&a);ReverbSTDFree(&b);
    }
    printf("{\"partitions\":%u,\"samples_compared\":384000,\"state_exact\":true}\n",partitions);
}
