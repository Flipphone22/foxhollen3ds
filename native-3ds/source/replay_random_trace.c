/* Explicit diagnostic build only. Observe original calls/returns; never seed,
 * substitute results, change inputs, or publish fabricated audio completion. */
#ifdef FH_REPLAY_RANDOM_TRACE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "fh_platform.h"
#include "fh_replay_random_trace.h"
/* This diagnostic is bounded to the 345-second prefix. Preserve the Old
 * 3DS normal-heap reserve rather than allocating an hour of unused rows. */
enum { TRACE_ROWS=2048, TRACE_STRIDE=16 };
enum { EVENT_ROWS=16384, EVENT_BEGIN=17118, EVENT_END=17138 };
typedef struct {
    uint32_t frame,kind,caller,range_caller,arg0,arg1,result,state;
} Event;
typedef struct {
    double tick;
    uint32_t frame,random_calls,seed_calls,random_hash,last_random;
    uint32_t sound_checks,sound_hash,last_sound;
} Row;
static Row current,*rows;
static unsigned count,overflow,active;
static Event *events;
static unsigned event_count,event_overflow;
static uint32_t range_caller,range_lo,range_hi;
static uint32_t mix(uint32_t hash,uint32_t value){return (hash^value)*16777619u;}
static void event(unsigned kind,uintptr_t caller,uint32_t a,uint32_t b,uint32_t result){
    if(!active || current.frame<EVENT_BEGIN || current.frame>=EVENT_END)return;
    if(event_count==EVENT_ROWS){event_overflow=1;return;}
    events[event_count++]=(Event){current.frame,kind,(uint32_t)caller,range_caller,a,b,result,current.last_random};
}
int __real_randomGetRange(int,int);
int __wrap_randomGetRange(int lo,int hi){
    uint32_t saved_caller=range_caller,saved_lo=range_lo,saved_hi=range_hi;
    range_caller=(uintptr_t)__builtin_return_address(0);range_lo=(uint32_t)lo;range_hi=(uint32_t)hi;
    int result=__real_randomGetRange(lo,hi);
    range_caller=saved_caller;range_lo=saved_lo;range_hi=saved_hi;
    return result;
}
uint32_t __real_sfaRand(void);
uint32_t __wrap_sfaRand(void){
    uint32_t result=__real_sfaRand();
    if(active){current.random_calls++;current.last_random=result;
        current.random_hash=mix(mix(current.random_hash,(uintptr_t)__builtin_return_address(0)),result);
        event(1,(uintptr_t)__builtin_return_address(0),range_lo,range_hi,result);}
    return result;
}
void __real_sfaSrand(uint32_t);
void __wrap_sfaSrand(uint32_t seed){
    __real_sfaSrand(seed);
    if(active){current.seed_calls++;current.last_random=seed;
        current.random_hash=mix(mix(current.random_hash,0x53454544u),seed);
        event(2,(uintptr_t)__builtin_return_address(0),seed,0,seed);}
}
uint32_t __real_sndFXCheck(uint32_t);
uint32_t __wrap_sndFXCheck(uint32_t handle){
    uint32_t result=__real_sndFXCheck(handle);
    if(active){current.sound_checks++;current.last_sound=result;
        current.sound_hash=mix(mix(current.sound_hash,handle),result);
        event(3,(uintptr_t)__builtin_return_address(0),handle,0,result);}
    return result;
}
void fh_replay_random_trace_begin(void){
    if(active || rows)fh_platform_panic("Replay random trace reused");
    rows=calloc(TRACE_ROWS,sizeof(*rows));
    events=calloc(EVENT_ROWS,sizeof(*events));
    if(!rows || !events)fh_platform_panic("Replay random trace budget");
    active=1;
    fh_platform_log("Replay determinism trace active: diagnostic timings; original RNG/audio results retained\n");
}
void fh_replay_random_trace_frame(double tick){
    if(!active)return;
    current.tick=tick;
    if(!(current.frame%TRACE_STRIDE)){
        if(count<TRACE_ROWS)rows[count++]=current;else overflow=1;
    }
    current.frame++;
}
int fh_replay_random_trace_finish(const char *directory){
    if(!active)return 1;
    active=0;
    if(count<TRACE_ROWS)rows[count++]=current;else overflow=1;
    char path[256];snprintf(path,sizeof(path),"%s/random-trace.csv",directory);
    FILE *f=fopen(path,"wb");int ok=f!=NULL;
    if(f){
        fputs("tick,clock,random_calls,seed_calls,random_hash,last_random,sound_checks,sound_hash,last_sound\n",f);
        for(unsigned i=0;i<count;i++){
            Row *r=rows+i;
            fprintf(f,"%.9f,%u,%u,%u,%08lx,%08lx,%u,%08lx,%08lx\n",r->tick,r->frame,
                r->random_calls,r->seed_calls,(unsigned long)r->random_hash,(unsigned long)r->last_random,
                r->sound_checks,(unsigned long)r->sound_hash,(unsigned long)r->last_sound);
        }
        if(ferror(f))ok=0;
        if(fclose(f))ok=0;
    }
    snprintf(path,sizeof(path),"%s/random-events.csv",directory);f=fopen(path,"wb");
    if(!f)ok=0;
    else{
        fputs("clock,kind,caller,range_caller,arg0,arg1,result,state\n",f);
        for(unsigned i=0;i<event_count;i++){
            Event *e=events+i;
            fprintf(f,"%u,%u,%08lx,%08lx,%08lx,%08lx,%08lx,%08lx\n",e->frame,e->kind,
                (unsigned long)e->caller,(unsigned long)e->range_caller,(unsigned long)e->arg0,
                (unsigned long)e->arg1,(unsigned long)e->result,(unsigned long)e->state);
        }
        if(ferror(f))ok=0;
        if(fclose(f))ok=0;
    }
    free(rows);rows=NULL;free(events);events=NULL;
    return ok&&!overflow&&!event_overflow;
}
#endif
