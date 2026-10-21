/* Bounded private fixture; the real source kernels are linked unchanged. */
#include "musyx/musyx.h"
#include <string.h>
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int reference_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
struct Fixture {
    _SND_REVSTD_WORK rv;
    float delay[12][2001],pre[3][3200];
    s32 samples[3][160];
    unsigned frames;
} fixture;
void trial_init(unsigned seed,unsigned predelay,unsigned frames){
    memset(&fixture,0,sizeof(fixture));
    _SND_REVSTD_WORK *r=&fixture.rv;fixture.frames=frames;
    r->level=.9f;r->allPassCoeff=.5f;r->damping=.55f;r->preDelayTime=predelay;
    for(unsigned j=0;j<12;j++){
        unsigned len=j<6?(j%2?2001:1791):(j%2?151:435);
        _SND_REVSTD_DELAYLINE *d=j<6?&r->C[j]:&r->AP[j-6];
        d->inputs=fixture.delay[j];d->length=len*4;
        d->inPoint=((seed%2)?(len-1-(seed%7)):(seed%13))*4;
        d->outPoint=(d->inPoint/4+2)%len*4;
        d->lastOutput=(float)((int)seed-13);
        for(unsigned i=0;i<len;i++)fixture.delay[j][i]=(float)((int)((i*971+seed*23)%10001)-5000);
    }
    for(unsigned j=0;j<6;j++)r->combCoef[j]=.4f+j*.04f;
    for(unsigned j=0;j<3;j++){
        r->lpLastout[j]=(float)seed;
        if(predelay){r->preDelayLine[j]=fixture.pre[j];r->preDelayPtr[j]=fixture.pre[j]+(predelay-1-seed%predelay);}
        for(unsigned i=0;i<3200;i++)fixture.pre[j][i]=(float)((int)((i*197+seed)%1001)-500);
        for(unsigned i=0;i<160;i++)fixture.samples[j][i]=(int)((i*991+seed*5)%100001)-50000;
    }
}
int trial_run(unsigned native){
    return native?fh_audio_reverb_std_frames(fixture.samples[0],fixture.samples[1],fixture.samples[2],fixture.frames,&fixture.rv):
        reference_frames(fixture.samples[0],fixture.samples[1],fixture.samples[2],fixture.frames,&fixture.rv);
}
