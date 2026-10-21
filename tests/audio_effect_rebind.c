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
#include "fh_audio_effect_routes.h"
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern bool ReverbSTDModify(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern bool fh_game_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern unsigned fh_game_reverb_storage_bytes(float,unsigned);
extern void fh_game_ReverbSTDCallback(s32 *,s32 *,s32 *,_SND_REVSTD_WORK *);
static unsigned checks,enabled,source_allocations,source_frees;
static jmp_buf trap;
static const char *expected_panic;
#define CHECK(x) do {checks++;assert(x);} while(0)
void fh_platform_panic(const char *message) {
    if(!expected_panic){fprintf(stderr,"unexpected panic: %s\n",message);abort();}
    CHECK(strstr(message,expected_panic));longjmp(trap,1);
}
void *salMalloc(size_t bytes){void *p=calloc(1,bytes);CHECK(p);source_allocations++;return p;}
void salFree(void *p){if(p){source_frees++;free(p);}}
void ndspAuxSetFrontBypass(int bus,int bypass){CHECK(bus<2 && !bypass);}
void ndspAuxSetVolume(int bus,float volume){CHECK(bus<2 && volume==1);}
void ndspAuxSetEnable(int bus,int on){CHECK(bus<2 && on);enabled++;}
void sndAuxCallbackReverbSTD(u8 reason,SND_AUX_INFO *data,void *user){(void)reason;(void)data;(void)user;}
#include "owner.c"
static Effect *effect(unsigned bus){return effect_routes.routes[effect_routes.current-1].bus[bus];}
static unsigned char history[EFFECT_BYTES];
static FhAudioEffectRoutes before_busy;
static void expect_failure(const char *message) {
    before_busy=effect_routes;uint32_t token=effect_route_token;
    expected_panic=message;if(!setjmp(trap)){(void)bind_effects();CHECK(0);}expected_panic=NULL;
    CHECK(!memcmp(&before_busy,&effect_routes,sizeof(effect_routes)) && token==effect_route_token);
}
static void collect_after(uint32_t token) {
    CHECK(fh_audio_effect_routes_begin(&effect_routes,token));CHECK(fh_audio_effect_routes_end(&effect_routes,token));
    /* Emulate the independently tested completed-bank retirement permission. */
    CHECK(fh_audio_effect_routes_retire_before(&effect_routes,token));
    CHECK(fh_audio_effect_routes_collect(&effect_routes)==1);
}
int main(void) {
    for(unsigned mode=0;mode<3;mode++) {
        SND_AUX_REVERBSTD r[2]={{0}};memset(dspStudio,0,sizeof(dspStudio));
        memset(&source_effects,0,sizeof(source_effects));effect_bus_mask=effect_route_token=enabled=0;
        for(unsigned i=0;i<2;i++) {
            r[i].coloration=.5f;r[i].time=3;r[i].mix=.9f;r[i].damping=.5f;r[i].preDelay=.033f;
            CHECK(ReverbSTDCreate(&r[i].rv,.5f,3,.9f,.5f,.033f));
        }
        DSPstudioinfo *s=&dspStudio[0];s->state=DSP_STUDIO_STATE_ACTIVE;s->isMaster=1;
        s->type=mode==1?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
        s->auxAHandler=sndAuxCallbackReverbSTD;s->auxAUser=&r[0];
        if(mode==0){s->auxBHandler=sndAuxCallbackReverbSTD;s->auxBUser=&r[1];}
        r[0].mix=.2f; /* Mutable wrapper fields are not immutable constructor state. */
        CHECK(bind_effects() && effect_route_token==1 && effect(0)->parameters[2]==.9f);
        Effect *old=effect(0),*retained=effect(1);unsigned initial_enabled=enabled;
        size_t retained_bytes=retained?retained->capacity:0;
        FhAudioAuxRate *rates[2]={effect_rates[0],effect_rates[1]},saved_rates[2];
        for(unsigned i=0;i<2;i++)if(rates[i]){rates[i]->blocks=17;rates[i]->input_phase=-100;saved_rates[i]=*rates[i];}
        if(retained){s32 pcm[3][160];for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)pcm[c][i]=1000;
            fh_game_ReverbSTDCallback(pcm[0],pcm[1],pcm[2],&retained->work);memcpy(history,retained->arena,retained_bytes);}
        CHECK(ReverbSTDModify(&r[0].rv,.5f,3,.9f,.5f,.033f));
        CHECK(bind_effects() && effect_route_token==2 && effect(0)!=old && enabled==initial_enabled);
        CHECK(fh_audio_effect_routes_begin(&effect_routes,1)->bus[0]==old);
        CHECK(fh_audio_effect_routes_end(&effect_routes,1));
        /* Third source update before playback frees either route: do not
         * overwrite queued state or spin waiting inside the source lock. */
        CHECK(ReverbSTDModify(&r[0].rv,.5f,3,.9f,.5f,.033f));expect_failure("capacity");
        collect_after(2);CHECK(bind_effects() && effect_route_token==3);collect_after(3);
        for(unsigned turn=4;turn<=12;turn++) {
            CHECK(ReverbSTDModify(&r[0].rv,.5f,3,.9f,.5f,.033f));
            CHECK(bind_effects() && effect_route_token==turn);collect_after(turn);
            CHECK(effect(1)==retained && enabled==initial_enabled);
            if(retained)CHECK(retained->capacity==retained_bytes && !memcmp(history,retained->arena,retained_bytes));
            for(unsigned i=0;i<2;i++)if(rates[i])CHECK(effect_rates[i]==rates[i] && !memcmp(rates[i],&saved_rates[i],sizeof(saved_rates[i])));
        }
        s->type=mode==1?SND_STUDIO_TYPE_STD:SND_STUDIO_TYPE_DPL2;expect_failure("mix mode");
        s->type=mode==1?SND_STUDIO_TYPE_DPL2:SND_STUDIO_TYPE_STD;
        s->auxAHandler=NULL;expect_failure("bus handoff");s->auxAHandler=sndAuxCallbackReverbSTD;
        CHECK(bind_effects() && effect_route_token==12 && enabled==initial_enabled);
        for(unsigned i=0;i<2;i++)ReverbSTDFree(&r[i].rv);
        fh_audio_effect_routes_shutdown(&effect_routes);
        for(unsigned i=0;i<2;i++){free(effect_rates[i]);effect_rates[i]=NULL;}
    }
    CHECK(source_allocations==source_frees);
    printf("{\"passed\":true,\"checks\":%u,\"route_publications\":36,\"capacity_rejections\":3}\n",checks);
}
