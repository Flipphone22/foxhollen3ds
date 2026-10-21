#include "fh_audio_aux_rate.h"
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
extern int fh_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int reference_rate(FhAudioAuxRate *,int32_t [4][160],FhAudioAux32Process,void *);
static void full_discard(void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){
    assert(fh_audio_reverb_std_frames(l,r,s,n,p));memset(s,0,n*sizeof(*s));
}
static void stereo(void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){
    (void)s;assert(fh_audio_reverb_std_stereo_frames(l,r,n,p));
}
static void full(void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){
    assert(fh_audio_reverb_std_frames(l,r,s,n,p));
}
int main(void){
    unsigned random=0x23abcdefu;uint64_t compared=0;
    for(unsigned mode=0;mode<2;mode++)for(unsigned setting=0;setting<4;setting++){
        _SND_REVSTD_WORK old={0},now={0};FhAudioAuxRate a={0},b={0};
        assert(ReverbSTDCreate(&old,.2f+setting*.2f,.5f+setting,.8f,.3f,setting*.01f));
        assert(ReverbSTDCreate(&now,.2f+setting*.2f,.5f+setting,.8f,.3f,setting*.01f));
        for(unsigned block=0;block<2048;block++){
            int32_t x[4][160],y[4][160];
            for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++){
                random=random*1664525u+1013904223u;
                x[c][i]=block%128<64?(int)(random%100001)-50000:0;
            }
            memcpy(y,x,sizeof(x));
            assert(reference_rate(&a,x,mode?full_discard:full,&old));
            assert(mode?fh_audio_aux_rate_stereo(&b,y,stereo,&now):fh_audio_aux_rate_process(&b,y,full,&now));
            assert(!memcmp(x,y,sizeof(x)));compared+=640;
            assert(a.source_frames==b.source_frames && a.output_source==b.output_source);
            assert(a.input_phase==b.input_phase && a.output_phase==b.output_phase && !b.faults);
        }
        for(unsigned i=0;i<(mode?4u:6u);i++){
            assert(old.AP[i].inPoint==now.AP[i].inPoint && old.AP[i].outPoint==now.AP[i].outPoint);
            assert(old.C[i].inPoint==now.C[i].inPoint && old.C[i].outPoint==now.C[i].outPoint);
            assert(!memcmp(old.AP[i].inputs,now.AP[i].inputs,old.AP[i].length));
            assert(!memcmp(old.C[i].inputs,now.C[i].inputs,old.C[i].length));
        }
        for(unsigned c=0;c<(mode?2u:3u);c++)assert(old.lpLastout[c]==now.lpLastout[c]);
        if(mode){assert(now.lpLastout[2]==0);for(unsigned i=0;i<162;i++)assert(b.work[2][i]==0);}
        ReverbSTDFree(&old);ReverbSTDFree(&now);
    }
    printf("{\"samples_compared\":%llu,\"stereo_exact\":true,\"surround_route_exact\":true}\n",(unsigned long long)compared);
}
