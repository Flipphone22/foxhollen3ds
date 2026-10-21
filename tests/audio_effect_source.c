#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "musyx/musyx.h"
#include "musyx/hardware.h"
#include "musyx/sal.h"
#include "fh_audio_aux_rate.h"
#include "fh_audio_effect_source.h"
#include "fh_audio_effect_epoch.h"
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern bool ReverbSTDModify(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern bool fh_game_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void fh_game_ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
static jmp_buf trap;
static unsigned traps,checks,allocations,deallocations,enabled;
static int expect_trap;
#define CHECK(x) do {checks++;assert(x);} while(0)
void fh_platform_panic(const char *s) {
    if(!expect_trap){fprintf(stderr,"unexpected panic: %s\n",s);abort();}
    CHECK(strstr(s,"lifetime changed")!=NULL);traps++;longjmp(trap,1);
}
void *salMalloc(size_t n){void *p=calloc(1,n);CHECK(p!=NULL);allocations++;return p;}
void salFree(void *p){if(p)deallocations++;free(p);}
void ndspAuxSetFrontBypass(int bus,int bypass){CHECK(bus<2 && !bypass);}
void ndspAuxSetVolume(int bus,float volume){CHECK(bus<2 && volume==1);}
void ndspAuxSetEnable(int bus,int on){CHECK(bus<2 && on);enabled++;}
/* Exact production Effect, source event wrappers, allocation and binding. */
#include "owner.c"

static void bounded(void) {
    FhAudioEffectSources p={0};int identities[FH_AUDIO_EFFECT_SOURCE_SLOTS+1];
    float v[5]={.5f,3,.9f,.5f,.033f};FhAudioEffectSource s;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_SOURCE_SLOTS;i++) {
        CHECK(fh_audio_effect_source_create(&p,&identities[i],v));
        CHECK(fh_audio_effect_source_read(&p,&identities[i],&s) && s.token==i+1);
    }
    FhAudioEffectSources before=p;
    CHECK(!fh_audio_effect_source_create(&p,&identities[FH_AUDIO_EFFECT_SOURCE_SLOTS],v));
    CHECK(!memcmp(&before,&p,sizeof(p)));
    CHECK(fh_audio_effect_source_create(&p,&identities[7],v));
    CHECK(fh_audio_effect_source_read(&p,&identities[7],&s) && s.token==17);
    CHECK(fh_audio_effect_source_destroy(&p,&identities[7]));
    CHECK(!fh_audio_effect_source_read(&p,&identities[7],&s));
    CHECK(fh_audio_effect_source_create(&p,&identities[16],v));
    p.serial=UINT32_MAX;before=p;
    CHECK(!fh_audio_effect_source_create(&p,&identities[16],v));
    CHECK(!memcmp(&before,&p,sizeof(p)));
    CHECK(!fh_audio_effect_source_read(&p,NULL,&s));
}
static void private_effect(Effect *e,float mix) {
    memset(e,0,sizeof(*e));e->arena=calloc(1,EFFECT_BYTES);CHECK(e->arena!=NULL);
    allocating_effect=e;CHECK(fh_game_ReverbSTDCreate(&e->work,.5f,3,mix,.5f,.033f));
    allocating_effect=NULL;e->ready=1;e->discard_surround=1;
}
static void returned_effects(void) {
    /* No callbacks are running in this fixture; reset only the test owner. */
    memset(&effect_epochs,0,sizeof(effect_epochs));effect_route_token=1;callback_fault=0;
    Effect old[2]={0},next[2]={0},old_ref={0},next_ref={0};unsigned bus=0;
    private_effect(&old[0],.9f);private_effect(&old_ref,.9f);
    private_effect(&next[0],.2f);private_effect(&next_ref,.2f);
    CHECK(fh_audio_effect_publish(&effect_epochs,1,old)==FH_AUDIO_EFFECT_OK);
    for(unsigned k=0;k<80;k++) {
        if(k==16) {
            CHECK(fh_audio_effect_publish(&effect_epochs,2,next)==FH_AUDIO_EFFECT_OK);
            effect_route_token=2; /* Producer has advanced; returned input has not. */
        }
        unsigned token=k<24?1:2;Effect *reference=token==1?&old_ref:&next_ref;
        s32 x[4][160],y[4][160];
        for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)
            x[c][i]=y[c][i]=(int32_t)((i*731+c*311+k*193)%12000)-6000;
        effect(&bus,x,token);
        CHECK(fh_audio_aux_rate_stereo(&reference->rate,y,reverb,reference));
        CHECK(!memcmp(x,y,sizeof(x)));CHECK(!callback_fault && !effect_epochs.active_slot);
    }
    s32 silence[4][160]={0};FhAudioAuxRate saved=next[0].rate;
    effect(&bus,silence,0);CHECK(!callback_fault && !memcmp(&saved,&next[0].rate,sizeof(saved)));
    silence[0][0]=1;effect(&bus,silence,0);CHECK(callback_fault==7);callback_fault=0;
    effect(&bus,silence,99);CHECK(callback_fault==8 && !effect_epochs.active_slot);callback_fault=0;
    bus=1;effect(&bus,silence,2);CHECK(callback_fault==9 && !effect_epochs.active_slot);callback_fault=0;
    CHECK(fh_audio_effect_retire_before(&effect_epochs,2));
    uint32_t collected;void *ptr;
    CHECK(fh_audio_effect_collect(&effect_epochs,&collected,&ptr) && collected==1 && ptr==old);
    CHECK(!fh_audio_effect_begin(&effect_epochs,1));
    free(old[0].arena);free(next[0].arena);free(old_ref.arena);free(next_ref.arena);
}
int main(void) {
    bounded();SND_AUX_REVERBSTD r={0};FhAudioEffectSource a,b;
    r.coloration=.5f;r.time=3;r.mix=.9f;r.damping=.5f;r.preDelay=.033f;
    CHECK(ReverbSTDCreate(&r.rv,r.coloration,r.time,r.mix,r.damping,r.preDelay));
    CHECK(fh_audio_effect_source_read(&source_effects,&r.rv,&a) && a.token==1);
    dspStudio[0].state=DSP_STUDIO_STATE_ACTIVE;dspStudio[0].isMaster=1;
    dspStudio[0].type=SND_STUDIO_TYPE_DPL2;
    dspStudio[0].auxAHandler=sndAuxCallbackReverbSTD;dspStudio[0].auxAUser=&r;
    /* Mutable public fields are NOT proof the underlying source was modified. */
    r.time=7;r.mix=.2f;
    CHECK(bind_effects());CHECK(effects[0].source_token==a.token && enabled==1);
    CHECK(!memcmp(a.parameters,effects[0].parameters,sizeof(a.parameters)));
    for(unsigned k=0;k<24;k++) {
        s32 x[3][160],y[3][160];
        for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)
            x[c][i]=y[c][i]=(int32_t)((i*731+c*311+k*193)%12000)-6000;
        ReverbSTDCallback(x[0],x[1],x[2],&r.rv);
        fh_game_ReverbSTDCallback(y[0],y[1],y[2],&effects[0].work);
        CHECK(!memcmp(x,y,sizeof(x)));CHECK(bind_effects());
    }
    CHECK(!ReverbSTDModify(&r.rv,-1,3,.9f,.5f,.033f));
    CHECK(fh_audio_effect_source_read(&source_effects,&r.rv,&b) && b.token==a.token);
    CHECK(ReverbSTDModify(&r.rv,.5f,3,.9f,.5f,.033f));
    CHECK(fh_audio_effect_source_read(&source_effects,&r.rv,&b) && b.token>a.token);
    /* Same pointer + same params still means RESET HISTORY. Existing platform
     * must reject it until output-time handoff is connected, not reuse old tail. */
    expect_trap=1;
    if(!setjmp(trap)){(void)bind_effects();CHECK(0);}
    expect_trap=0;CHECK(traps==1 && effects[0].source_token==a.token && enabled==1);
    ReverbSTDFree(&r.rv);CHECK(!fh_audio_effect_source_read(&source_effects,&r.rv,&b));
    CHECK(ReverbSTDCreate(&r.rv,.5f,3,.9f,.5f,.033f));
    CHECK(fh_audio_effect_source_read(&source_effects,&r.rv,&b) && b.token>a.token);
    /* Original Modify accepts <=100 but Create accepts <=.1. A failed recreate
     * has ALREADY freed old history; it must not retain a valid source token. */
    CHECK(!ReverbSTDModify(&r.rv,.5f,3,.9f,.5f,.5f));
    CHECK(!fh_audio_effect_source_read(&source_effects,&r.rv,&b));
    CHECK(!ReverbSTDCreate(&r.rv,-1,3,.9f,.5f,.033f));
    CHECK(!fh_audio_effect_source_read(&source_effects,&r.rv,&b));
    CHECK(allocations==deallocations);
    free(effects[0].arena);
    returned_effects();
    printf("{\"passed\":true,\"checks\":%u,\"source_waveform_blocks\":24,\"returned_effect_blocks\":80,\"caught_identical_recreation\":%u,\"source_allocations\":%u,\"source_frees\":%u}\n",checks,traps,allocations,deallocations);
}
