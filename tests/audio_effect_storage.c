#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "musyx/musyx.h"
#include "fh_audio_effect_routes.h"
extern bool fh_game_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern bool fh_reference_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern unsigned fh_game_reverb_storage_bytes(float,unsigned);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int reference_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int reference_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static unsigned checks,wave_blocks,constructors;
#define CHECK(x) do {checks++;assert(x);} while(0)
void fh_platform_panic(const char *message){fprintf(stderr,"panic: %s\n",message);CHECK(0);abort();}
#include "owner.c"
static unsigned char reference_arena[EFFECT_BYTES];
static size_t reference_used;
void *fh_reference_reverb_alloc(size_t n){n=(n+7)&~(size_t)7;CHECK(n<=EFFECT_BYTES-reference_used);void *p=reference_arena+reference_used;reference_used+=n;return p;}
void fh_reference_reverb_free(void *p){(void)p;}
static Effect *make(unsigned channels,float delay) {
    FhAudioEffectKey k={.source_token=7,.discard_surround=channels==2,.parameters={.5f,3,.9f,.5f,delay}};
    Effect *e=create_effect(NULL,&k);CHECK(e && e->used==e->capacity && e->capacity<=EFFECT_BYTES);constructors++;
    CHECK(e->capacity==fh_game_reverb_storage_bytes(delay,channels));
    for(unsigned i=0;i<6;i++) {
        CHECK((e->work.C[i].inputs!=NULL)==(i<2*channels));
        CHECK((e->work.AP[i].inputs!=NULL)==(i<2*channels));
    }
    if(channels==2)CHECK(!e->work.preDelayLine[2] && !e->work.preDelayPtr[2]);
    return e;
}
int main(void) {
    /* Exercise every predelay allocation boundary and adjacent representable
     * floats against actual constructor allocator calls, not a size-only oracle. */
    for(unsigned channels=2;channels<=3;channels++)for(unsigned sample=0;sample<=3200;sample++) {
        float delay=(float)sample/32000.f;
        float values[3]={delay,nextafterf(delay,0),nextafterf(delay,1)};
        for(unsigned i=0;i<3;i++)if(values[i]<=.1f) {
            Effect *e=make(channels,values[i]);destroy_effect(NULL,e);
        }
    }
    const float delays[]={0,1e-7f,.033f,.1f};uint32_t seed=17;
    for(unsigned channels=2;channels<=3;channels++)for(unsigned d=0;d<4;d++) {
        Effect *e=make(channels,delays[d]);_SND_REVSTD_WORK reference;
        reference_used=0;memset(reference_arena,0,sizeof(reference_arena));
        CHECK(fh_reference_ReverbSTDCreate(&reference,.5f,3,.9f,.5f,delays[d]));
        s32 actual[3][160],expected[3][160];
        for(unsigned block=0;block<600;block++) {
            unsigned frames=1+(block*37)%160;
            for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++) {
                seed=seed*1664525u+1013904223u;actual[c][i]=expected[c][i]=(s32)(seed>>8)-0x7fffff;
            }
            if(channels==2) {
                CHECK(fh_audio_reverb_std_stereo_frames(actual[0],actual[1],frames,&e->work));
                CHECK(reference_audio_reverb_std_stereo_frames(expected[0],expected[1],frames,&reference));
            } else {
                CHECK(fh_audio_reverb_std_frames(actual[0],actual[1],actual[2],frames,&e->work));
                CHECK(reference_audio_reverb_std_frames(expected[0],expected[1],expected[2],frames,&reference));
            }
            CHECK(!memcmp(actual,expected,sizeof(actual)));wave_blocks++;
        }
        if(channels==2)CHECK(e->capacity*3==reference_used*2);
        else CHECK(e->capacity==reference_used);
        destroy_effect(NULL,e);
    }
    CHECK(!fh_game_reverb_storage_bytes(NAN,2) && !fh_game_reverb_storage_bytes(INFINITY,3));
    CHECK(!fh_game_reverb_storage_bytes(-1,2) && !fh_game_reverb_storage_bytes(.2f,3));
    CHECK(!fh_game_reverb_storage_bytes(.033f,0) && !fh_game_reverb_storage_bytes(.033f,4));
    CHECK(fh_game_reverb_storage_bytes(.033f,2)==43504);
    printf("{\"passed\":true,\"checks\":%u,\"constructors\":%u,\"waveform_blocks\":%u,\"dpl2_typical_bytes\":%u,\"std_max_bytes\":%u}\n",
        checks,constructors,wave_blocks,fh_game_reverb_storage_bytes(.033f,2),fh_game_reverb_storage_bytes(.1f,3));
}
