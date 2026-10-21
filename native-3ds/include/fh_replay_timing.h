#ifndef FH_REPLAY_TIMING_H
#define FH_REPLAY_TIMING_H
#include <stdint.h>
#include <string.h>
/* Main-thread wall time, partitioned rather than summed inclusive scopes.
 * Nested scopes restore their caller. Worker CPU and GPU time overlap this
 * partition and must be reported separately. No clocks, allocation or I/O here. */
#define FH_REPLAY_PHASES(X) \
 X(OTHER,other) X(FRAME_SETUP,frame_setup) X(AUDIO_UPDATE,audio_update) \
 X(LOAD_FILES,load_files) X(UI_END,ui_end) X(TEXT,text) X(SFX,sfx) \
 X(UI_START,ui_start) X(OBJECT_UPDATE,object_update) X(ENVIRONMENT,environment) \
 X(COLLISION,collision) X(SCENE_RENDER,scene_render) X(UI_DRAW,ui_draw) \
 X(REFLECTION_SMALL,reflection_small) X(REFLECTION_UPDATE,reflection_update) \
 X(DISTORTION,distortion) X(PARTICLES,particles) X(OBJECT_RENDER,object_render) \
 X(PRESENT,present) X(VBLANK,vblank) X(INPUT,input) X(AUDIO_OBSERVE,audio_observe) \
 X(METRICS,metrics) X(GPU_WAIT,gpu_wait) X(WORKER_WAIT,worker_wait) \
 X(AUDIO_PUMP,audio_pump) X(DVD_CALLBACKS,dvd_callbacks) \
 X(NATIVE_WORLD,native_world) X(NATIVE_MODEL,native_model) \
 X(TEXTURE_RESOLVE,texture_resolve) X(SCENE_SOURCE,scene_source) \
 X(MODEL_REFRESH,model_refresh) X(WORLD_VISIBILITY,world_visibility) \
 X(MODEL_PALETTE,model_palette) X(NATIVE_SUBMIT,native_submit) \
 X(PRESENT_PREP,present_prep) X(PRESENT_COPY,present_copy) \
 X(PRESENT_PUBLISH,present_publish) X(RETRACE_CALLBACKS,retrace_callbacks)
#define FH_PHASE_ENUM(symbol,name) FH_PHASE_##symbol,
enum { FH_REPLAY_PHASES(FH_PHASE_ENUM) FH_PHASE_COUNT };
#undef FH_PHASE_ENUM
typedef struct {
    uint64_t ticks[FH_PHASE_COUNT],stamp;
    unsigned phase,started,errors,calls[FH_PHASE_COUNT];
} FhReplayClock;
static inline unsigned fh_replay_clock_enter(FhReplayClock *c,unsigned next,uint64_t now){
    unsigned previous=c->phase;
    if(next>=FH_PHASE_COUNT || (c->started && now<c->stamp)){c->errors++;return previous;}
    if(c->started)c->ticks[previous]+=now-c->stamp;
    c->started=1;c->stamp=now;c->phase=next;c->calls[next]++;return previous;
}
static inline void fh_replay_clock_snapshot(const FhReplayClock *c,uint64_t now,uint64_t *out){
    memcpy(out,c->ticks,sizeof(c->ticks));
    if(c->started && now>=c->stamp)out[c->phase]+=now-c->stamp;
}
#ifdef FH_REPLAY_BUILD
unsigned fh_replay_phase(unsigned phase);
unsigned fh_replay_phase_at(unsigned phase,uint64_t now);
#elif defined(FH_CPU_PANEL_BUILD)
extern unsigned fh_cpu_panel_enabled;
unsigned fh_cpu_panel_phase(unsigned phase);
unsigned fh_cpu_panel_phase_at(unsigned phase,uint64_t now);
#define fh_replay_phase(p) (__atomic_load_n(&fh_cpu_panel_enabled,__ATOMIC_RELAXED)?fh_cpu_panel_phase(p):FH_PHASE_COUNT)
#define fh_replay_phase_at(p,t) (__atomic_load_n(&fh_cpu_panel_enabled,__ATOMIC_RELAXED)?fh_cpu_panel_phase_at(p,t):FH_PHASE_COUNT)
#endif
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
/* Cleanup restores the caller on every normal/early return. Only replay builds
 * measure; worker calls are ignored by the existing main-owner check. */
static inline void fh_replay_scope_end(unsigned *previous){fh_replay_phase(*previous);}
#define FH_REPLAY_SCOPE(kind) unsigned fh_scope_previous __attribute__((cleanup(fh_replay_scope_end))) = fh_replay_phase(FH_PHASE_##kind)
#else
#define FH_REPLAY_SCOPE(kind) ((void)0)
#endif
#endif
