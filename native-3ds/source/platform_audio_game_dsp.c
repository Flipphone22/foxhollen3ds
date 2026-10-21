/* Local replay candidate, deliberately not part of installed game archives.
 * Unsupported live transitions fail the experiment rather than pretending
 * software readers remained current after DSP waveform admission. */
#include <3ds.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "fh_platform.h"
#include "fh_audio_game_dsp.h"
#include "fh_audio_cost.h"
#include "fh_audio_pcm_queue.h"
#include "fh_audio_native_voice.h"
#include "fh_audio_dsp_aux.h"
#include "fh_audio_aux_rate.h"
#include "fh_audio_aux_native.h"
#include "fh_audio_aux_spans.h"
#include "fh_audio_aux_shared.h"
#include "fh_audio_balanced_mix.h"
#include "fh_audio_return_delay.h"
#include "fh_audio_aux_output_delay.h"
#include "fh_audio_bench_mode.h"
#include "fh_audio_voice_window.h"
#include "fh_audio_effect_source.h"
#include "fh_audio_effect_routes.h"
#include "musyx/musyx.h"
#include "musyx/hardware.h"

enum { SAMPLE_BYTES=1024*1024,EFFECT_BYTES=96*1024 };
typedef struct {
    _SND_REVSTD_WORK work;
    float parameters[5];
    unsigned ready,discard_surround,source_token;
    void *arena;
    size_t used,capacity;
} Effect;
static FhAudioSamplePool pool;
static FhAudioSampleRegistry registry;
static FhAudioDspVoices voices;
static FhAudioNativeVoices source;
static FhAudioDspAux aux;
static Effect *allocating_effect;
static FhAudioEffectSources source_effects; /* Synth owner only. */
static FhAudioEffectRoutes effect_routes;
static unsigned effect_bus_mask;
static FhAudioReturnDelay effect_return_delay; /* SDK callback-owned; never producer-reset. */
static FhAudioAuxOutputDelay dry_output_delay; /* Completed values, no live source references. */
static unsigned effect_ready[2]; /* Release-published after raw ring initialization. */
static FhAudioAuxOutputDelay wet_output_delay; /* Preserve established dry/wet two-device-sample alignment. */
static unsigned effect_mode[2];
static FhAudioAuxStream effect_streams[2];
static unsigned stream_failure[18],effect_bypass[2],effect_bypass_blocks[2];
static unsigned collected_effect_routes,deferred_effect_routes;
static FhAudioAuxInput effect_input[2];
static uint64_t raw_source_frame;
static unsigned raw_source_generation,raw_captured[2],raw_peak[2],raw_consumed[2],raw_failure[10];
static unsigned effect_route_token; /* Release-published after initial binding. */
static unsigned effect_bus[2]={0,1}; /* Immutable SDK callback contexts. */

/* Called by the actual source constructor/modify/free, including identical-
 * parameter recreation. Never called by our separately named native copy. */
void fh_audio_game_effect_created(const void *work,float coloration,float time,
    float mix,float damping,float predelay) {
    const float parameters[5]={coloration,time,mix,damping,predelay};
    FhAudioEffectSource previous;
    int existed=fh_audio_effect_source_read(&source_effects,work,&previous);
    if(!fh_audio_effect_source_create(&source_effects,work,parameters))
        fh_platform_panic("Native source effect registry exhausted or invalid");
    if(existed)(void)fh_audio_effect_routes_source_end(&effect_routes,previous.token);
}
void fh_audio_game_effect_destroyed(const void *work) {
    FhAudioEffectSource previous;
    if(fh_audio_effect_source_read(&source_effects,work,&previous))
        (void)fh_audio_effect_routes_source_end(&effect_routes,previous.token);
    (void)fh_audio_effect_source_destroy(&source_effects,work);
}
static void *sample_arena;
static int16_t *silence;
static unsigned initialized,source_ready,callback_fault,source_epoch;
/* SDK-callback owned. Preparation can already belong to a later generation;
 * a gap must hold the last observed playback generation, not cancel it. */
static unsigned playback_epoch;
static uint32_t playback_effect_token;
static unsigned source_cycles;
static unsigned cpu_reference;
static unsigned callback_windows,callback_gaps;
static unsigned returned_windows[2],returned_unknown[2],returned_unmapped[2],returned_lookahead[2],returned_boundaries[2];
/* Experimental failure journal: bounded atomic scalar records, no callback IO.
 * It records mapping changes and every16 submissions. Timings from this
 * diagnostic candidate are excluded from acceptance comparisons. */
enum { JOURNAL_ROWS=32,JOURNAL_WORDS=20 };
static unsigned journal[JOURNAL_ROWS][JOURNAL_WORDS+1],journal_head,journal_previous=~0u;
static void mapping_journal(const FhAudioPcmQueue *q,const FhAudioSourceWindow *w) {
    const FhAudioPcmChannel *p=&q->channels[0];
    unsigned tick=ndspGetFrameCount(),dropped=ndspGetDroppedFrames();
    unsigned state=w->valid|(p->output_clock.valid<<1)|(p->prebuffer<<2)|(p->hardware_paused<<3);
    if(state==journal_previous && (tick&15))return;
    journal_previous=state;
    unsigned serial=journal_head+1,*row=journal[journal_head%JOURNAL_ROWS];
    /* The odd marker must precede subsequent field writes on ARM11 too. */
    (void)__atomic_exchange_n(row,serial*2-1,__ATOMIC_ACQ_REL);
    unsigned words[JOURNAL_WORDS]={tick,dropped,state,w->count,
        w->count?w->spans[0].generation:0,w->count?w->spans[0].offset:0,
        w->count?w->spans[0].frames:0,w->count?(unsigned)w->spans[0].source_frame:0,
        (unsigned)p->output_clock.frame,(unsigned)p->cursor.frame,p->cursor.available,
        p->submitted,p->underruns,p->active_epoch,__atomic_load_n(&p->requested_epoch,__ATOMIC_ACQUIRE),
        p->output_clock.last_tick,p->output_clock.last_dropped,(unsigned)w->input_first,w->frames,p->output_clock.fraction};
    for(unsigned i=0;i<JOURNAL_WORDS;i++)__atomic_store_n(&row[i+1],words[i],__ATOMIC_RELAXED);
    __atomic_store_n(row,serial*2,__ATOMIC_RELEASE);
    __atomic_store_n(&journal_head,serial,__ATOMIC_RELEASE);
}
static void mapping_failure(uint64_t frame) {
    char text[512];unsigned end=__atomic_load_n(&journal_head,__ATOMIC_ACQUIRE);
    snprintf(text,sizeof(text),"native DSP queue failure source=%llu admitted=%u segments=%u stops=%u\n",
        (unsigned long long)frame,source.admitted,source.segments,source.stops);fh_platform_log(text);
    for(unsigned i=0;i<FH_AUDIO_DSP_CHANNELS;i++)if(source.voices[i].token) {
        FhAudioDspVoice *v=&voices.voices[i];
        snprintf(text,sizeof(text),"native DSP pending channel=%u head=%u tail=%u due=%llu source_voice=%u\n",
            i+FH_AUDIO_DSP_FIRST_CHANNEL,v->head,__atomic_load_n(&v->tail,__ATOMIC_ACQUIRE),
            (unsigned long long)v->last_due,source.voices[i].source_index);fh_platform_log(text);
    }
    for(unsigned serial=end>JOURNAL_ROWS?end-JOURNAL_ROWS:0;serial<end;serial++) {
        unsigned *row=journal[serial%JOURNAL_ROWS],before=__atomic_load_n(row,__ATOMIC_ACQUIRE),words[JOURNAL_WORDS];
        if(before!=(serial+1)*2)continue;
        for(unsigned i=0;i<JOURNAL_WORDS;i++)words[i]=__atomic_load_n(&row[i+1],__ATOMIC_RELAXED);
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
        if(before!=__atomic_load_n(row,__ATOMIC_ACQUIRE))continue;
        int at=snprintf(text,sizeof(text),"native DSP mapping row=%u",serial);
        for(unsigned i=0;i<JOURNAL_WORDS;i++)at+=snprintf(text+at,sizeof(text)-(unsigned)at,",%u",words[i]);
        snprintf(text+at,sizeof(text)-(unsigned)at,"\n");fh_platform_log(text);
    }
}
extern bool fh_game_ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern unsigned fh_game_reverb_storage_bytes(float,unsigned);
extern int fh_audio_reverb_std_frames(s32 *,s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);
extern int fh_audio_reverb_std_stereo_frames(s32 *,s32 *,unsigned,_SND_REVSTD_WORK *);

/* Private native reverb constructor uses a sized, checked arena. No source
 * allocator, game effect object or allocation is used by the SDK callback. */
void *fh_game_reverb_alloc(size_t bytes) {
    Effect *e=allocating_effect;
    bytes=(bytes+7)&~(size_t)7;
    if(!e || e->used>e->capacity || bytes>e->capacity-e->used)fh_platform_panic("Native effect arena overflow");
    void *p=(unsigned char *)e->arena+e->used;e->used+=bytes;return p;
}
void fh_game_reverb_free(void *p) {(void)p;}
unsigned fh_game_reverb_channels(void) {
    if(!allocating_effect)fh_platform_panic("Native effect constructor has no owner");
    return allocating_effect->discard_surround?2:3;
}
static void *create_effect(void *context,const FhAudioEffectKey *key) {
    (void)context;Effect *e=calloc(1,sizeof(*e));if(!e)return NULL;
    const float *p=key->parameters;e->discard_surround=key->discard_surround;
    e->capacity=fh_game_reverb_storage_bytes(p[4],e->discard_surround?2:3);
    if(!e->capacity || e->capacity>EFFECT_BYTES){free(e);return NULL;}
    e->arena=calloc(1,e->capacity);if(!e->arena){free(e);return NULL;}
    allocating_effect=e;
    int ok=fh_game_ReverbSTDCreate(&e->work,p[0],p[1],p[2],p[3],p[4]);
    allocating_effect=NULL;
    if(!ok || e->used!=e->capacity){free(e->arena);free(e);return NULL;}
    memcpy(e->parameters,p,sizeof(e->parameters));
    e->source_token=key->source_token;e->discard_surround=key->discard_surround;e->ready=1;
    return e;
}
static void destroy_effect(void *context,void *payload) {
    (void)context;Effect *e=payload;free(e->arena);free(e);
}
static int effect_admit(void *context,void *payload,unsigned bus) {
    if(bus!=*(const unsigned *)context || bus>=2)return 0;
    if(!payload)return 1; /* Explicit source-unknown/no-handler span. */
    const Effect *e=payload;
    return e->ready && e->discard_surround==effect_mode[bus];
}
static int effect_apply(void *context,void *payload,int32_t *l,int32_t *r,int32_t *a,unsigned frames) {
    unsigned bus=*(const unsigned *)context;
    if(bus!=0 || !effect_mode[0])return 0;
    /* Source rotates INPUT before calling this cycle's handler. Delaying the
     * processed output instead changes effect identity at attachment boundaries. */
    if(!fh_audio_return_delay(&effect_return_delay,l,r,a,frames))return 0;
    if(!payload) {
        memset(l,0,frames*sizeof(*l));memset(r,0,frames*sizeof(*r));memset(a,0,frames*sizeof(*a));
    } else {
        Effect *e=payload;
        if(!fh_audio_reverb_std_stereo_frames(l,r,frames,&e->work))return 0;
    }
    return 1;
}
void fh_audio_game_output_mix(unsigned channel,const float mix[12]) {
    float routed[12];
    if(cpu_reference){memcpy(routed,mix,sizeof(routed));ndspChnSetMix(channel,routed);return;}
    if(!fh_audio_balanced_mix(routed,mix))fh_platform_panic("Native balanced output bus conflict");
    ndspChnSetMix(channel,routed);
}
static void effect_mapped(void *context,int32_t samples[4][160],uint32_t token,const FhAudioAuxWindow *window) {
    (void)token; /* Full returned spans, never the producer/scalar token, own routing. */
    unsigned bus=*(const unsigned *)context;
    if(bus>=2) {__atomic_store_n(&callback_fault,12,__ATOMIC_RELEASE);return;}
    /* B retains mapped-bank ownership and carries finished dry PCM. Match
     * the two output samples retained by A's rate adapter, without changing
     * either signal's level or touching source/effect/voice ownership. */
    if(bus==1 && !cpu_reference) {
        if(!fh_audio_aux_output_delay(&dry_output_delay,samples))
            __atomic_store_n(&callback_fault,15,__ATOMIC_RELEASE);
        return;
    }
    {extern unsigned fh_hardware_is_new,fh_graphics_original_active;
     if(!fh_hardware_is_new && !fh_graphics_original_active && !effect_bypass[bus])__atomic_store_n(&effect_bypass[bus],2,__ATOMIC_RELEASE);}
    if(__atomic_load_n(&effect_bypass[bus],__ATOMIC_ACQUIRE)) {
        FhAudioAuxInput *q=&effect_input[bus];
        q->read_offset=0;__atomic_store_n(&q->tail,__atomic_load_n(&q->head,__ATOMIC_ACQUIRE),__ATOMIC_RELEASE);
        memset(samples,0,sizeof(int32_t[4][160]));effect_bypass_blocks[bus]++;return;
    }
    unsigned ready=__atomic_load_n(&effect_ready[bus],__ATOMIC_ACQUIRE);
    if(!ready) {__atomic_store_n(&callback_fault,11,__ATOMIC_RELEASE);return;}
    int result=fh_audio_aux_native_shared(&effect_streams[bus],&effect_routes,window,
        bus,effect_mode[bus],&effect_input[bus],samples,effect_admit,effect_apply,context);
    if(result==FH_AUDIO_AUX_INPUT_PENDING)result=0;
    if(result==FH_AUDIO_AUX_PROCESSED) {
        raw_consumed[bus]=(unsigned)(effect_streams[bus].next_input-effect_streams[bus].origin);
        if(!fh_audio_aux_output_delay(&wet_output_delay,samples))result=0;
    }
    if(!result) {
        const FhAudioAuxInput *q=&effect_input[bus];
        unsigned head=__atomic_load_n(&q->head,__ATOMIC_ACQUIRE),tail=q->tail;
        const FhAudioAuxInputStamp *stamp=head!=tail?&q->stamps[tail%FH_AUX_INPUT_SLOTS]:NULL;
        unsigned values[10]={bus,head,tail,q->read_offset,stamp?(unsigned)stamp->first:0,
            stamp?stamp->generation:0,stamp?stamp->effect:0,
            window&&window->source.count?(unsigned)window->source.spans[0].source_frame:0,
            window&&window->source.count?window->source.spans[0].generation:0,
            window&&window->source.count?window->source.spans[0].effect_token:0};
        for(unsigned i=0;i<10;i++)__atomic_store_n(&raw_failure[i],values[i],__ATOMIC_RELAXED);
    }
    if(!result) {
        if(!__atomic_load_n(&callback_fault,__ATOMIC_ACQUIRE)) {
            FhAudioAuxStream *stream=&effect_streams[bus];
            unsigned values[18]={bus,window!=NULL,window?window->source.valid:0,
                window?(unsigned)window->source.input_first:0,window?window->source.frames:0,
                window?window->phase:0,window?window->queue_epoch:0,
                stream->next_phase,(unsigned)(stream->next_input-stream->origin),returned_windows[bus],
                stream->initialized,stream->epoch,(unsigned)stream->next_input,stream->next_phase,
                window?window->source.count:0,window&&window->source.count?window->source.spans[0].generation:0,
                window&&window->source.count?window->source.spans[0].effect_token:0,
                window?window->has_lookahead:0};
            for(unsigned i=0;i<18;i++)__atomic_store_n(&stream_failure[i],values[i],__ATOMIC_RELAXED);
            FhAudioAuxInput *q=&effect_input[bus];
            q->read_offset=0;__atomic_store_n(&q->tail,__atomic_load_n(&q->head,__ATOMIC_ACQUIRE),__ATOMIC_RELEASE);
            memset(samples,0,sizeof(int32_t[4][160]));
            __atomic_store_n(&effect_bypass[bus],1,__ATOMIC_RELEASE);
        }
        return;
    }
    if(window) {
        unsigned known=0;
        for(unsigned i=0;i<window->source.count;i++)known+=window->source.spans[i].generation!=0;
        returned_windows[bus]++;returned_unknown[bus]+=!known;
        returned_lookahead[bus]+=window->has_lookahead!=0;
    } else returned_unmapped[bus]++;
}
static void effect_after_publish(void *context) {
    if(__atomic_load_n(&callback_fault,__ATOMIC_ACQUIRE))return;
    uint32_t floor=fh_audio_dsp_aux_effect_floor(context);
    /* The actual PCM producer rejects backwards effect tokens before copying
     * or publishing any data. Together with both queued mapped banks and ALL
     * successful bus callbacks, this prevents a future old-token lookup. */
    if(floor>__atomic_load_n(&effect_routes.epochs.return_floor,__ATOMIC_RELAXED) && floor<=effect_routes.completed)
        if(!fh_audio_effect_routes_retire_before(&effect_routes,floor))
            __atomic_store_n(&callback_fault,14,__ATOMIC_RELEASE);
}
int fh_audio_game_dsp_preinit(void) {
    cpu_reference=!fh_audio_bench_latch();
    fh_platform_log(cpu_reference?"Audio comparison mode: CPU reference\n":"Audio comparison mode: native DSP\n");
    for(unsigned i=0;i<2;i++){aux.mapped_process[i]=effect_mapped;aux.context[i]=&effect_bus[i];}
    aux.after_publish=effect_after_publish;aux.after_publish_context=&aux;
    return fh_audio_dsp_aux_install(&aux);
}
int fh_audio_game_dsp_initialize(FhAudioPoolFlush flush,void *context) {
    if(initialized)return 0;
    sample_arena=linearMemAlign(SAMPLE_BYTES,32);silence=linearMemAlign(2048,32);
    if(!sample_arena || !silence)return 0;
    if(fh_audio_pool_init(&pool,sample_arena,SAMPLE_BYTES) ||
       fh_audio_sample_registry_init(&registry,&pool,flush,context) ||
       fh_audio_dsp_voices_init(&voices,&pool,silence,2048,flush,context))return 0;
    if(!cpu_reference) {
        ndspAuxSetFrontBypass(1,false);ndspAuxSetVolume(1,1);ndspAuxSetEnable(1,true);
    }
    fh_audio_native_samples=&registry;initialized=1;
    return 1;
}
static int bind_effects(void) {
    DSPstudioinfo *s=&dspStudio[0];
    if(s->state!=DSP_STUDIO_STATE_ACTIVE || !s->isMaster ||
       (s->type!=SND_STUDIO_TYPE_STD && s->type!=SND_STUDIO_TYPE_DPL2) || s->numInputs)return 0;
    /* This branch reserves B for dry transport. Do not admit a STD
     * two-effect studio and silently lose its second effect. */
    if(!cpu_reference && s->type!=SND_STUDIO_TYPE_DPL2)return 0;
    SND_AUX_CALLBACK callbacks[2]={s->auxAHandler,s->auxBHandler};
    void *users[2]={s->auxAUser,s->auxBUser};
    FhAudioEffectKey keys[2]={0};unsigned mask=0,changed=0;
    const FhAudioEffectRoute *current=effect_routes.current?&effect_routes.routes[effect_routes.current-1]:NULL;
    for(unsigned i=0;i<2;i++) {
        if(i && s->type==SND_STUDIO_TYPE_DPL2)continue;
        if(!callbacks[i])continue;
        if(callbacks[i]!=sndAuxCallbackReverbSTD || !users[i])return 0;
        const SND_AUX_REVERBSTD *r=users[i];
        if(r->tempDisableFX)return 0;
        FhAudioEffectSource snapshot;
        if(!fh_audio_effect_source_read(&source_effects,&r->rv,&snapshot))return 0;
        const float *p=snapshot.parameters;
        keys[i].source_token=snapshot.token;keys[i].discard_surround=s->type==SND_STUDIO_TYPE_DPL2;
        memcpy(keys[i].parameters,p,sizeof(keys[i].parameters));mask|=1u<<i;
        /* Effect identity changes do not reset continuous bus-rate history.
         * Queued PCM retains its own route; publish a separately owned new one. */
        if(current) {
            if(effect_mode[i]!=keys[i].discard_surround)
                fh_platform_panic("Native effect mix mode changed: bus handoff incomplete");
            Effect *e=current->bus[i];
            if(!e || e->source_token!=snapshot.token || memcmp(p,e->parameters,sizeof(e->parameters)) ||
               e->discard_surround!=keys[i].discard_surround)
                changed=1;
        }
    }
    /* fhMusyxStartOutput precedes source handler installation. A worker can
     * reach this entry in between. Preserve software production until the
     * complete single-reverb graph is known; publish no empty route/rate state. */
    if(!cpu_reference && !current && mask!=1)return 0;
    if(mask==3 && keys[0].source_token==keys[1].source_token)
        fh_platform_panic("Native shared AUX object needs source-block scheduling");
    if(current) {
        if(mask!=effect_bus_mask)fh_platform_panic("Native effect lifetime changed: bus handoff incomplete");
        if(!changed)return 1; /* No allocation on unchanged source blocks. */
    }
    if(!effect_routes.create && !fh_audio_effect_routes_init(&effect_routes,create_effect,destroy_effect,NULL))
        fh_platform_panic("Native effect owner initialization failed");
    uint32_t token;
    int status=fh_audio_effect_routes_prepare(&effect_routes,keys,&token);
    if(status==FH_AUDIO_EFFECT_BUSY && current){deferred_effect_routes++;return 1;}
    if(status!=FH_AUDIO_EFFECT_OK)
        fh_platform_panic(status==FH_AUDIO_EFFECT_BUSY?"Native effect route capacity: producer backpressure incomplete":"Native effect publication failed");
    effect_bus_mask=mask;
    for(unsigned i=0;!current && i<2;i++)if(mask&(1u<<i)) {
        effect_mode[i]=keys[i].discard_surround;
        if(!cpu_reference) {
            unsigned channels=effect_mode[i]?2:3;size_t bytes=fh_audio_aux_input_bytes(channels);
            int32_t *storage=malloc(bytes);
            if(!storage || !fh_audio_aux_input_init(&effect_input[i],storage,bytes,channels))
                fh_platform_panic("Native raw AUX allocation failed");
        }
        __atomic_store_n(&effect_ready[i],1,__ATOMIC_RELEASE);
        ndspAuxSetFrontBypass(i,false);ndspAuxSetVolume(i,1);ndspAuxSetEnable(i,!cpu_reference);
    }
    __atomic_store_n(&effect_route_token,token,__ATOMIC_RELEASE);
    return 1;
}
void fh_audio_game_dsp_prepare(uint64_t frame,unsigned generation) {
    raw_source_frame=frame;raw_source_generation=generation;
    if(!initialized)fh_platform_panic("Native audio owner not initialized");
    collected_effect_routes+=fh_audio_effect_routes_collect(&effect_routes);
    unsigned fault=__atomic_load_n(&callback_fault,__ATOMIC_ACQUIRE);
    for(unsigned b=0;b<2;b++)if(__atomic_load_n(&effect_bypass[b],__ATOMIC_ACQUIRE)==1) {
        __atomic_store_n(&effect_bypass[b],2,__ATOMIC_RELEASE);
        char text[384];int at=snprintf(text,sizeof(text),"native DSP effect bypass bus=%u frame=%llu",b,(unsigned long long)frame);
        for(unsigned i=0;i<18;i++)at+=snprintf(text+at,sizeof(text)-(unsigned)at,",%u",__atomic_load_n(&stream_failure[i],__ATOMIC_RELAXED));
        snprintf(text+at,sizeof(text)-(unsigned)at,"\n");fh_platform_log(text);
    }
    if(fault==13) {
        char raw[256];int used=snprintf(raw,sizeof(raw),"native DSP raw failure");
        for(unsigned i=0;i<10;i++)used+=snprintf(raw+used,sizeof(raw)-(unsigned)used,",%u",__atomic_load_n(&raw_failure[i],__ATOMIC_RELAXED));
        snprintf(raw+used,sizeof(raw)-(unsigned)used,"\n");fh_platform_log(raw);
        char text[384];int at=snprintf(text,sizeof(text),"native DSP stream failure");
        for(unsigned i=0;i<18;i++)at+=snprintf(text+at,sizeof(text)-(unsigned)at,",%u",__atomic_load_n(&stream_failure[i],__ATOMIC_RELAXED));
        snprintf(text+at,sizeof(text)-(unsigned)at,"\n");fh_platform_log(text);
    }
    if(fault==3 || fault==5){
        static unsigned recovered[2];unsigned kind=fault==5;
        if(!recovered[kind]++){char text[96];snprintf(text,sizeof(text),"Native audio callback fault=%u recovered source_frame=%llu\n",fault,(unsigned long long)frame);fh_platform_log(text);}
        __atomic_store_n(&callback_fault,0,__ATOMIC_RELEASE);fault=0;
    }
    if(fault){char text[96];snprintf(text,sizeof(text),"Native audio callback fault=%u source_frame=%llu",fault,(unsigned long long)frame);fh_platform_panic(text);}
    uint32_t completed;
    if(fh_audio_dsp_voices_poll(&voices,&completed))fh_platform_panic("Native audio retirement fault");
    /* Original MusyX already owns and applies CPU-mode effects.
     * Do not clone/publish an empty native graph during startup. */
    if(cpu_reference)return;
    if(!source_ready) {
        if(!bind_effects())return;
        if(!fh_audio_native_voices_init(&source,&voices,&registry,generation,dspStudio[0].type==SND_STUDIO_TYPE_DPL2?4:3) ||
           !fh_audio_native_voices_bind_aux(&source,0))fh_platform_panic("Native audio source admission failed");
        __atomic_store_n(&source_epoch,generation,__ATOMIC_RELEASE);
        fh_audio_source_bridge=&source.bridge;source_ready=1;
    } else {
        if(generation!=source_epoch) {
            /* Preparation can run ahead of playback. Reset source mappings only;
             * old DSP leases, raw AUX and effect routes remain queued under their
             * own identities until the consumer reaches the new generation. */
            if(!fh_audio_native_voices_reset(&source,generation))
                fh_platform_panic("Native audio source generation invalid");
            if(!bind_effects() || !fh_audio_native_voices_bind_aux(&source,0))
                fh_platform_panic("Native audio source reset route unsupported");
            __atomic_store_n(&source_epoch,generation,__ATOMIC_RELEASE);
        } else if(!bind_effects())fh_platform_panic("Native audio route changed: integration incomplete");
        if((source.aux_handlers[0]!=dspStudio[0].auxAHandler ||
            source.aux_handlers[1]!=dspStudio[0].auxBHandler || source.aux_users[0]!=dspStudio[0].auxAUser ||
            source.aux_users[1]!=dspStudio[0].auxBUser) && !fh_audio_native_voices_refresh_aux(&source,0))
            fh_platform_panic("Native source AUX rebinding failed");
    }
    if(!fh_audio_native_voices_available(&source)) {
        mapping_failure(frame);fh_platform_panic("Native audio control queue full: integration incomplete");
    }
    if(++source_cycles%256==0) {
        char text[256];snprintf(text,sizeof(text),"native DSP source cycles=%u admitted=%u declined=%u segments=%u stops=%u peak_pending=%u registry_prepared=%u registry_declined=%u\n",
            source_cycles,source.admitted,source.declined,source.segments,source.stops,source.peak_pending,registry.prepared,registry.declined);
        fh_platform_log(text);
    }
}
uint32_t fh_audio_game_dsp_effect_token(void) {
    /* Source owner only. SDK playback reads queued PCM provenance instead. */
    return __atomic_load_n(&effect_route_token,__ATOMIC_RELAXED);
}
static void native_source_window(FhAudioSourceWindow *w) {
    if(w->count>FH_AUDIO_MAP_SPANS)return; /* Existing validators reject it. */
    for(unsigned i=0;i<w->count;i++)if(!w->spans[i].effect_token) {
        /* Token zero was produced by the original mixer before admission.
         * Its completed dry/wet PCM remains on the common output bus. */
        w->spans[i].generation=0;w->spans[i].source_frame=0;
    }
}
static void native_aux_window(FhAudioAuxWindow *w) {
    native_source_window(&w->source);
    if(w->has_lookahead && !w->lookahead.effect_token) {
        w->lookahead.generation=0;w->lookahead.source_frame=0;
    }
}
void fh_audio_game_dsp_service(const FhAudioPcmQueue *q) {
    if(cpu_reference)return;
    FhAudioSourceWindow w=fh_audio_pcm_source_window(q,0);
    mapping_journal(q,&w);
    native_source_window(&w);
    FhAudioSpan region;
    int mapped=fh_audio_voice_window(&w,&region);
    if(mapped<0)__atomic_store_n(&callback_fault,3,__ATOMIC_RELEASE);
    const FhAudioSpan *known=mapped>0?&region:NULL;
    if(known && !__atomic_load_n(&callback_fault,__ATOMIC_ACQUIRE)) {
        playback_epoch=known->generation;
        playback_effect_token=known->effect_token;
        callback_windows++;
        fh_audio_dsp_voices_service_region(&voices,known->generation,known->source_frame,
            known->source_frame+known->frames,known->offset,known->effect_token!=0);
    } else {
        callback_gaps++;
        /* No non-atomic source-owner access from the SDK thread. */
        fh_audio_dsp_voices_service_region(&voices,playback_epoch,0,0,0,0);
    }
    /* The PCM buffer carried this route from its actual source render, through
     * movie mixing and queueing. Source preparation can already be many blocks
     * ahead. Unknown output keeps the last playback route; it cannot advance or
     * retire effects. Continuous same-source effect boundaries share ONE voice
     * interval; AUX retains every original effect span and lookahead. Interior
     * gaps/mixed source generations remain unsupported. Gap/tail waveform
     * parity and full source transitions remain separate gates. */
    FhAudioAuxWindow aux_window=fh_audio_pcm_aux_window(q,0);
    native_aux_window(&aux_window);
    fh_audio_dsp_aux_tag_mapped(&aux,playback_effect_token,&aux_window);
    if(voices.faults || voices.prefix_faults || aux.faults)
        __atomic_store_n(&callback_fault,voices.prefix_faults?5:voices.faults?4:6,__ATOMIC_RELEASE);
}
int fh_audio_game_capture_aux(unsigned studio,unsigned bus,uint64_t frame,unsigned frames,
    const int32_t *left,const int32_t *right,const int32_t *surround) {
    if(cpu_reference || !source_ready || studio!=0)return 0;
    if(bus>=2 || !(effect_bus_mask&(1u<<bus)) || frames!=160 || frame!=raw_source_frame ||
       raw_source_generation!=source_epoch)
        fh_platform_panic("Native raw AUX source identity failed");
    FhAudioAuxInput *q=&effect_input[bus];
    unsigned token=fh_audio_game_dsp_effect_token();
    int status=fh_audio_aux_input_push(q,raw_source_generation,token,frame,left,right,surround);
    if(status!=FH_AUX_INPUT_OK)fh_platform_panic(status==FH_AUX_INPUT_BUSY?
        "Native raw AUX queue capacity reached":"Native raw AUX publication failed");
    raw_captured[bus]++;
    unsigned pending=q->head-__atomic_load_n(&q->tail,__ATOMIC_ACQUIRE);
    if(pending>raw_peak[bus])raw_peak[bus]=pending;
    return 1;
}
void fh_audio_game_dsp_report(void) {
    /* Called only after SDK thread join, so counters are no longer changing. */
    unsigned raw_bytes=0;
    for(unsigned i=0;i<2;i++) {
        raw_bytes+=(unsigned)fh_audio_aux_input_bytes(effect_input[i].channels);
        char line[200];snprintf(line,sizeof(line),"native DSP raw bus=%u captured=%u consumed=%u peak=%u retained=%u\n",
            i,raw_captured[i],raw_consumed[i],raw_peak[i],(unsigned)(effect_input[i].head-effect_input[i].tail));fh_platform_log(line);
    }
    char raw_line[96];snprintf(raw_line,sizeof(raw_line),"native DSP raw normal_bytes=%u metadata_bytes=%u\n",
        raw_bytes,(unsigned)(sizeof(effect_input)+sizeof(raw_source_frame)+sizeof(raw_source_generation)+sizeof(raw_captured)+sizeof(raw_peak)+sizeof(raw_consumed)+sizeof(raw_failure)));fh_platform_log(raw_line);
    fh_platform_log("Native AUX clock: DSP-rate effects, held software sends, retained two-sample output alignment\n");
    unsigned effect_bytes=0;
    for(unsigned i=0;i<FH_AUDIO_EFFECT_OBJECTS;i++)if(effect_routes.objects[i].payload) {
        const Effect *e=effect_routes.objects[i].payload;effect_bytes+=sizeof(*e)+e->capacity;
    }
    char text[512];snprintf(text,sizeof(text),"native DSP audio admitted=%u declined=%u segments=%u stops=%u source_faults=%u callback_fault=%u windows=%u gaps=%u registry_prepared=%u registry_declined=%u owner_bytes=%u normal_effect_bytes=%u linear_bytes=%u\n",
        source.admitted,source.declined,source.segments,source.stops,source.faults,callback_fault,
        callback_windows,callback_gaps,registry.prepared,registry.declined,
        (unsigned)(sizeof(pool)+sizeof(registry)+sizeof(voices)+sizeof(source)+sizeof(aux)+sizeof(effect_routes)+sizeof(effect_bus_mask)+sizeof(effect_ready)+sizeof(wet_output_delay)+sizeof(source_effects)+sizeof(effect_route_token)+sizeof(effect_bus)+sizeof(effect_mode)+sizeof(effect_streams)+sizeof(stream_failure)+sizeof(collected_effect_routes)),
        effect_bytes,SAMPLE_BYTES+2048);fh_platform_log(text);
    snprintf(text,sizeof(text),"native DSP controls published=%u retained=%u curve_shortages=%u\n",
        voices.controls,voices.retained_controls,voices.curve_shortages);fh_platform_log(text);
    snprintf(text,sizeof(text),"native DSP effects retired_floor=%u collected_routes=%u deferred_routes=%u\n",
        (unsigned)effect_routes.epochs.return_floor,collected_effect_routes,deferred_effect_routes);fh_platform_log(text);
    for(unsigned i=0;i<2;i++) {
        snprintf(text,sizeof(text),"native DSP returned bus=%u windows=%u unknown=%u unmapped=%u lookahead=%u next_route=%u\n",
            i,returned_windows[i],returned_unknown[i],returned_unmapped[i],returned_lookahead[i],returned_boundaries[i]);
        fh_platform_log(text);
    }
}
void fh_audio_game_dsp_shutdown(void) {
    fh_audio_source_bridge=NULL;fh_audio_native_samples=NULL;
    fh_audio_dsp_aux_uninstall();fh_audio_game_dsp_report();fh_audio_cost_report();
    /* SDK is joined; no DSP or callback can refer to these buffers again. */
    linearFree(sample_arena);linearFree(silence);
    fh_audio_effect_routes_shutdown(&effect_routes);
    for(unsigned i=0;i<2;i++){effect_ready[i]=0;free(effect_input[i].samples);memset(&effect_input[i],0,sizeof(effect_input[i]));}
    initialized=0;
}
