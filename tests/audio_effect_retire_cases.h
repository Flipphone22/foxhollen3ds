#include "fh_audio_effect_routes.h"
static FhAudioEffectRoutes effect_routes;
static unsigned callback_fault,retire_mask,retire_calls;
#include "retire-owner.c" /* Exact production post-publication coordinator. */
static FhAudioAuxWindow retire_window(unsigned a,unsigned b,unsigned next) {
    FhAudioAuxWindow w={.source={.frames=156,.count=2,.valid=1},.queue_epoch=8,.rate=32000,.has_lookahead=1};
    w.source.spans[0]=(FhAudioSpan){100,a?91:0,0,78,a};
    w.source.spans[1]=(FhAudioSpan){178,b?91:0,78,78,b};
    w.lookahead=(FhAudioSpan){256,next?91:0,156,1,next};return w;
}
static void retire_process(void *ctx,int32_t samples[4][160],uint32_t token,const FhAudioAuxWindow *w) {
    (void)samples;(void)token;unsigned bus=(uintptr_t)ctx;retire_mask|=1u<<bus;
    if(!w)return;
    for(unsigned i=0;i<w->source.count+w->has_lookahead;i++) {
        const FhAudioSpan *s=i<w->source.count?&w->source.spans[i]:&w->lookahead;
        if(!s->effect_token)continue;
        const FhAudioEffectRoute *r=fh_audio_effect_routes_begin(&effect_routes,s->effect_token);
        CHECK(r && r->token==s->effect_token);
        CHECK(fh_audio_effect_routes_end(&effect_routes,s->effect_token));
    }
}
static void retire_completed(void *ctx) {
    CHECK(retire_mask==3);retire_calls++;effect_after_publish(ctx);CHECK(!callback_fault);
}
static void retire_step(unsigned frame,FhAudioAuxWindow w,unsigned expected) {
    ndspFrameId=frame;ndspBufferId=ndspBufferCurId=frame&1;unsigned before=retire_calls;
    fh_audio_dsp_aux_tag_mapped(&owner,999,&w);retire_mask=0;
    CHECK(!fh_audio_dsp_aux_effect_floor(&owner)); /* Not published yet. */
    fh_audio_dsp_aux_service();CHECK(retire_calls==before+1);
    CHECK(effect_routes.epochs.return_floor==expected);
}
static void effect_retire_cases(void) {
    memset(&owner,0,sizeof(owner));memset(&effect_routes,0,sizeof(effect_routes));
    effect_routes.routes[0].token=1;effect_routes.routes[1].token=2;
    CHECK(!fh_audio_effect_publish(&effect_routes.epochs,1,&effect_routes.routes[0]));
    CHECK(!fh_audio_effect_publish(&effect_routes.epochs,2,&effect_routes.routes[1]));
    owner.mapped_process[0]=owner.mapped_process[1]=retire_process;owner.context[1]=(void *)1;
    owner.after_publish=retire_completed;owner.after_publish_context=&owner;
    CHECK(fh_audio_dsp_aux_install(&owner));ndspAuxSetEnable(0,true);ndspAuxSetEnable(1,true);
    retire_step(10,retire_window(1,1,1),0);retire_step(11,retire_window(1,1,1),0);
    retire_step(12,retire_window(1,2,2),1);retire_step(13,retire_window(2,2,2),1);
    uint32_t token;void *payload;
    CHECK(!fh_audio_effect_collect(&effect_routes.epochs,&token,&payload));
    retire_step(14,retire_window(2,2,2),2);
    CHECK(fh_audio_effect_collect(&effect_routes.epochs,&token,&payload) && token==1 && payload==&effect_routes.routes[0]);
    CHECK(!fh_audio_effect_routes_begin(&effect_routes,1));
    for(unsigned i=15;i<40;i++)retire_step(i,retire_window(2,2,2),2);
    CHECK(!fh_audio_effect_collect(&effect_routes.epochs,&token,&payload));
    owner.after_publish=NULL;
    owner.bank_window[0]=retire_window(2,2,2);owner.bank_window[1]=retire_window(1,1,2);
    CHECK(fh_audio_dsp_aux_effect_floor(&owner)==1);
    owner.bank_window[1]=retire_window(2,2,1);CHECK(fh_audio_dsp_aux_effect_floor(&owner)==1);
    owner.bank_window[1]=retire_window(2,2,2);CHECK(fh_audio_dsp_aux_effect_floor(&owner)==2);
    owner.mapped_process[1]=NULL;owner.tagged_process[1]=tagged;CHECK(!fh_audio_dsp_aux_effect_floor(&owner));
    owner.mapped_process[1]=retire_process;
    owner.bank_window[0]=owner.bank_window[1]=retire_window(0,0,0);CHECK(!fh_audio_dsp_aux_effect_floor(&owner));
    fh_audio_dsp_aux_uninstall();CHECK(!fh_audio_dsp_aux_effect_floor(&owner));
}
