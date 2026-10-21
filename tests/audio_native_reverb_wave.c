#include "fh_audio_aux_rate.h"
#include "musyx/musyx.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
static unsigned channels=2,allocated;
void *fh_game_reverb_alloc(size_t n){allocated+=(n+7)&~7u;return calloc(1,n);}
void fh_game_reverb_free(void *p){free(p);}
unsigned fh_game_reverb_channels(void){return channels;}
extern bool fh_game_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void fh_game_ReverbSTDFree(_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern bool tuned_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void tuned_ReverbSTDFree(_SND_REVSTD_WORK *);
extern int tuned_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern unsigned tuned_storage_bytes(float,unsigned);
static void process(void *p,int32_t *l,int32_t *r,int32_t *s,unsigned n){(void)s;assert(fh_audio_reverb_std_stereo_frames(l,r,n,p));}
int main(int argc,char **argv){
    assert(argc==2);FILE *f=fopen(argv[1],"wb");assert(f);
    for(unsigned setting=0;setting<3;setting++) {
        _SND_REVSTD_WORK a={0},b={0};FhAudioAuxRate rate={0};
        assert(fh_game_ReverbSTDCreate(&a,.5f,.5f+setting,.8f,.5f,.01f*setting));
        assert(tuned_ReverbSTDCreate(&b,.5f,.5f+setting,.8f,.5f,.01f*setting));
        for(unsigned block=0;block<1024;block++) {
            int32_t old[4][160]={{0}},now[4][160]={{0}};
            for(unsigned i=0;i<160;i++) {
                unsigned at=block*160+i;
                double seconds=(double)at/(16756991.0/512);
                int32_t sample=at==0?20000:seconds<.2?7000*(sin(seconds*440*6.283185307179586)+.3*sin(seconds*1723*6.283185307179586)):0;
                old[0][i]=old[1][i]=now[0][i]=now[1][i]=sample;
            }
            assert(fh_audio_aux_rate_stereo(&rate,old,process,&a));
            assert(tuned_audio_reverb_std_stereo_frames(now[0],now[1],160,&b));
            for(unsigned i=0;i<160;i++){int32_t pair[2]={old[0][i],now[0][i]};assert(fwrite(pair,sizeof(pair),1,f)==1);}
        }
        fh_game_ReverbSTDFree(&a);tuned_ReverbSTDFree(&b);
    }
    for(channels=2;channels<=3;channels++)for(unsigned i=0;i<=1024;i++) {
        float predelay=.1f*i/1024;_SND_REVSTD_WORK rv={0};allocated=0;
        assert(tuned_ReverbSTDCreate(&rv,.5f,1,.8f,.5f,predelay));
        assert(allocated==tuned_storage_bytes(predelay,channels));tuned_ReverbSTDFree(&rv);
    }
    assert(!fclose(f));return 0;
}
