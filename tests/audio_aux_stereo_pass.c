#include "fh_audio_aux_rate.h"
#include <string.h>
#include <stdint.h>
#include <limits.h>
#ifdef HOST_CHECK
#include <assert.h>
#include <stdio.h>
#endif
extern int reference_stereo(FhAudioAuxRate *,int32_t [4][160],FhAudioAux32Process,void *);
extern int reference_process(FhAudioAuxRate *,int32_t [4][160],FhAudioAux32Process,void *);
typedef struct {
    FhAudioAuxRate rate;
    int32_t samples[4][160];
    unsigned seed,mode,calls,frames,checksum;
    int result;
} Fixture;
Fixture fixture;
static unsigned random_word(void) {
    fixture.seed=fixture.seed*1664525u+1013904223u;return fixture.seed;
}
static void effect(void *context,int32_t *l,int32_t *r,int32_t *s,unsigned frames) {
    Fixture *f=context;f->calls++;f->frames+=frames;
    int32_t *channels[3]={l,r,s};
    for(unsigned c=0;c<f->mode;c++)for(unsigned i=0;i<frames;i++) {
        f->checksum=f->checksum*33u+(uint32_t)channels[c][i];
        /* A deterministic stateful effect, including large signed outputs. */
        channels[c][i]=(int32_t)((uint32_t)channels[c][i]^f->checksum);
    }
}
void trial_input(void) {
    for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++) {
        unsigned x=random_word();
        fixture.samples[c][i]=(i%19==0)?INT32_MIN:(i%23==0)?INT32_MAX:(int32_t)x;
    }
}
int trial_run(unsigned reference) {
    int (*fn)(FhAudioAuxRate *,int32_t [4][160],FhAudioAux32Process,void *)=
        fixture.mode==2?(reference?reference_stereo:fh_audio_aux_rate_stereo):
                         (reference?reference_process:fh_audio_aux_rate_process);
    fixture.result=fn(&fixture.rate,fixture.samples,effect,&fixture);return fixture.result;
}
void trial_init(unsigned seed,unsigned warmup,unsigned mode) {
    memset(&fixture,0,sizeof(fixture));fixture.seed=seed;fixture.mode=mode;
    for(unsigned i=0;i<warmup;i++){trial_input();trial_run(1);}
    trial_input();
}
#ifdef HOST_CHECK
int main(void) {
    static Fixture before,expected,continuing;
    unsigned cases=0;
    for(unsigned mode=2;mode<=3;mode++) {
        trial_init(17,0,mode);continuing=fixture;
        for(unsigned block=0;block<8192;block++) {
            fixture=continuing;trial_input();before=fixture;
            assert(trial_run(1));expected=fixture;
            fixture=before;assert(trial_run(0));assert(!memcmp(&fixture,&expected,sizeof(fixture)));
            continuing=fixture;cases++;
        }
    }
    for(unsigned mode=2;mode<=3;mode++)for(unsigned invalid=0;invalid<4;invalid++) {
        trial_init(83,13,mode);
        if(invalid==0)fixture.rate.output_source=0;
        if(invalid==1)fixture.rate.output_source+=10000;
        if(invalid==2)fixture.rate.initial_silence=0;
        if(invalid==3)fixture.rate.initial_silence=1;
        before=fixture;trial_run(1);expected=fixture;
        fixture=before;trial_run(0);assert(!memcmp(&fixture,&expected,sizeof(fixture)));cases++;
    }
    assert(!fh_audio_aux_rate_stereo(0,fixture.samples,effect,&fixture));
    assert(!fh_audio_aux_rate_stereo(&fixture.rate,0,effect,&fixture));
    assert(!fh_audio_aux_rate_stereo(&fixture.rate,fixture.samples,0,&fixture));
    printf("{\"cases\":%u,\"output_samples\":%u,\"state_exact\":true}\n",cases,cases*640);
}
#endif
