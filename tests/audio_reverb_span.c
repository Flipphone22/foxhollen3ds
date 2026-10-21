#include "musyx/musyx.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void *salMalloc(size_t n){return calloc(1,n);}
void salFree(void *p){free(p);}
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int reference_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static void state(const _SND_REVSTD_WORK *a,const _SND_REVSTD_WORK *b){
    assert(a->preDelayTime==b->preDelayTime);
    assert(!memcmp(a->lpLastout,b->lpLastout,sizeof(a->lpLastout)));
    for(unsigned j=0;j<12;j++){
        const _SND_REVSTD_DELAYLINE *x=j<6?&a->C[j]:&a->AP[j-6];
        const _SND_REVSTD_DELAYLINE *y=j<6?&b->C[j]:&b->AP[j-6];
        assert(x->length==y->length && x->inPoint==y->inPoint && x->outPoint==y->outPoint);
        assert(!memcmp(&x->lastOutput,&y->lastOutput,sizeof(float)));
        assert(!memcmp(x->inputs,y->inputs,x->length));
    }
    if(a->preDelayTime)for(unsigned j=0;j<3;j++){
        assert(a->preDelayPtr[j]-a->preDelayLine[j]==b->preDelayPtr[j]-b->preDelayLine[j]);
        assert(!memcmp(a->preDelayLine[j],b->preDelayLine[j],a->preDelayTime*sizeof(float)));
    }
}
int main(void){
    static const unsigned frames[]={0,1,2,3,7,31,32,79,149,150,151,157,159,160};
    static const float delay[]={0,1.0f/32000,32.0f/32000,.033f,.1f};
    unsigned rng=38101,calls=0;unsigned long long samples=0;
    for(unsigned setting=0;setting<15;setting++){
        _SND_REVSTD_WORK a={0},b={0};
        float color=setting%3*.5f,time=.01f+(setting%3)*4.995f;
        assert(ReverbSTDCreate(&a,color,time,.9f,setting%3*.5f,delay[setting%5]));
        assert(ReverbSTDCreate(&b,color,time,.9f,setting%3*.5f,delay[setting%5]));
        for(unsigned block=0;block<4096;block++){
            s32 x[3][160],y[3][160];unsigned n=frames[block%14];
            for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++){
                rng=rng*1664525u+1013904223u;
                x[c][i]=block%512<256?(int)(rng%100001)-50000:0;
            }
            memcpy(y,x,sizeof(x));
            assert(reference_frames(x[0],x[1],x[2],n,&a));
            assert(fh_audio_reverb_std_frames(y[0],y[1],y[2],n,&b));
            assert(!memcmp(x,y,sizeof(x)));samples+=n*3;calls++;
            if(block%64==0)state(&a,&b);
        }
        state(&a,&b);ReverbSTDFree(&a);ReverbSTDFree(&b);
    }
    printf("{\"samples\":%llu,\"calls\":%u,\"full_state_exact\":true}\n",samples,calls);
}
