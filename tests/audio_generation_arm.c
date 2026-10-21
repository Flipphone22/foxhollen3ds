/* Real source controls and SDK voice queues; effect binding is explicitly
 * stubbed here. Waveform/history continuity is covered by the AUX tests. */
#define run original_fixture_run
#include "audio_native_voice_arm.c"
#undef run
#include "fh_audio_aux_input.h"
#include "fh_audio_effect_routes.h"
#include <stdio.h>
#define source native
#define voices dsp
static unsigned initialized=1,source_ready,callback_fault,source_epoch;
static unsigned collected_effect_routes,cpu_reference,source_cycles;
static uint64_t raw_source_frame;
static unsigned raw_source_generation,raw_captured[2],raw_peak[2],raw_consumed[2],raw_failure[10],stream_failure[18];
static FhAudioAuxInput effect_input[2];
static int32_t raw_storage[64*2*160];
static FhAudioEffectRoutes effect_routes;
static unsigned effect_bus_mask=1,bind_calls,route_token=1;
static FhAudioAuxInput saved_input;
static FhAudioDspVoices saved_playback;
static void reset_aux(u8 reason,SND_AUX_INFO *data,void *user){(void)reason;(void)data;(void)user;}
static int bind_effects(void){bind_calls++;return 1;}
static void mapping_failure(uint64_t frame){(void)frame;CHECK(0);}
void fh_platform_log(const char *s){(void)s;}
__attribute__((noreturn)) void fh_platform_panic(const char *s){(void)s;failure=9000;done();}
uint32_t fh_audio_game_dsp_effect_token(void){return route_token;}
#include "generation-owner.c"
#undef source
#undef voices

static void source_setup(void){
    setup();dspStudio[0].auxAHandler=reset_aux;dspStudio[0].auxAUser=&info;
}
static void raw(unsigned generation,unsigned block){
    int32_t p[3][160];
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)p[c][i]=generation*1000+block*160+c*31+i;
    CHECK(fh_audio_game_capture_aux(0,0,block*160,160,p[0],p[1],p[2])==1);
}
static void read_raw(unsigned generation,unsigned block,unsigned token){
    int32_t p[3][160]={{0}};
    CHECK(!fh_audio_aux_input_add(effect_input,generation,token,block*160,160,p[0],p[1],p[2]));
    for(unsigned c=0;c<2;c++)for(unsigned i=0;i<160;i++)
        CHECK(p[c][i]==(int32_t)(generation*1000+block*160+c*31+i));
}
void run(void){
    for(unsigned i=0;i<16;i++)ndspVars[i]=shared[i];
    CHECK(!fh_audio_pool_init(&pool,arena,sizeof(arena)));
    CHECK(!fh_audio_sample_registry_init(&registry,&pool,flush,NULL));
    CHECK(!fh_audio_dsp_voices_init(&dsp,&pool,silence,sizeof(silence),flush,NULL));
    CHECK(fh_audio_aux_input_init(effect_input,raw_storage,sizeof(raw_storage),2));
    source_setup();CHECK(!fh_audio_sample_registry_prepare(&registry,&info,&source_voice.smp_info));
    fh_audio_game_dsp_prepare(0,1);
    CHECK(source_ready && source_epoch==1 && native.epoch==1 && native.aux_bound && bind_calls==1);
    controls(0);s16 pcm[320];salCtrlDsp(pcm);raw(1,0);
    FhAudioPoolToken old=native.bridge.voices[0].token;CHECK(old && dsp.voices[0].lease==old);
    fh_audio_dsp_voices_service_window(&dsp,1,0,32,1);CHECK(dsp.voices[0].live);
    fh_audio_game_dsp_prepare(160,1);raw(1,1);
    saved_playback=dsp;saved_input=effect_input[0];
    source_setup();route_token=2;
    fh_audio_game_dsp_prepare(0,2);
    CHECK(source_epoch==2 && native.epoch==2 && native.aux_epoch==2 && native.aux_bound);
    CHECK(bind_calls==3 && !memcmp(&dsp,&saved_playback,sizeof(dsp)));
    CHECK(!memcmp(effect_input,&saved_input,sizeof(saved_input)));
    raw(2,0);controls(0);salCtrlDsp(pcm);
    FhAudioPoolToken next=native.bridge.voices[0].token;CHECK(next && next!=old);
    fh_audio_dsp_voices_service_window(&dsp,1,32,64,1);
    CHECK(dsp.voices[0].live && !dsp.voices[1].live);
    read_raw(1,0,1);read_raw(1,1,1);read_raw(2,0,2);
    CHECK(effect_input[0].tail==effect_input[0].head && !effect_input[0].read_offset);
    fh_audio_dsp_voices_service_window(&dsp,2,0,32,1);
    CHECK(!dsp.voices[0].live && dsp.voices[1].live);
    CHECK(dsp.voices[0].lease==old && dsp.voices[1].lease==next);
    CHECK(!callback_fault && !native.faults && !native.bridge.faults);
    finished=1;done();
}
