#ifndef FH_CPU_LIGHT_CACHE_H
#define FH_CPU_LIGHT_CACHE_H
#include "fh_hash_bytes.h"
#include "fh_memory_bytes.h"
/* Exact-result cache for CPU lighting. Keys own every arithmetic dependency;
 * no game pointer, matrix identity or frame number substitutes for values. */
typedef struct {
    FhChannel channels[4];FhLight lights[8];float ambient[2][4],material[2][4];
    unsigned raster,fpscr;
} FhCpuLightState;
typedef struct {float eye[3],normal[3],color[2][4];} FhCpuLightInput;
enum { FH_CPU_LIGHT_STATES=16,FH_CPU_LIGHT_SETS=256,FH_CPU_LIGHT_WAYS=8 };
typedef struct {FhCpuLightState key;unsigned id,stamp;} FhCpuLightStateEntry;
typedef struct {FhCpuLightInput input;float output[2][4];unsigned id,hash,stamp;} FhCpuLightEntry;
static FhCpuLightStateEntry fh_cpu_light_states[FH_CPU_LIGHT_STATES];
static FhCpuLightEntry fh_cpu_light_values[FH_CPU_LIGHT_SETS*FH_CPU_LIGHT_WAYS];
static unsigned fh_cpu_light_clock,fh_cpu_light_serial;
unsigned fh_cpu_light_hits,fh_cpu_light_misses,fh_cpu_light_state_hits,fh_cpu_light_state_misses;
_Static_assert(sizeof(fh_cpu_light_states)+sizeof(fh_cpu_light_values)<=256*1024,"CPU light cache budget");
static void fh_cpu_light_reset(void){
    memset(fh_cpu_light_states,0,sizeof(fh_cpu_light_states));memset(fh_cpu_light_values,0,sizeof(fh_cpu_light_values));
    fh_cpu_light_clock=fh_cpu_light_serial=0;
}
static unsigned fh_cpu_light_state(const FhCpuLightState *key){
    if(fh_cpu_light_clock>0xffff0000u || fh_cpu_light_serial>0xffff0000u)fh_cpu_light_reset();
    FhCpuLightStateEntry *oldest=fh_cpu_light_states;
    for(unsigned i=0;i<FH_CPU_LIGHT_STATES;i++){
        FhCpuLightStateEntry *s=fh_cpu_light_states+i;
        if(s->id && !fh_memory_compare(&s->key,key,sizeof(*key))){s->stamp=++fh_cpu_light_clock;fh_cpu_light_state_hits++;return s->id;}
        if(s->stamp<oldest->stamp)oldest=s;
    }
    oldest->key=*key;oldest->id=++fh_cpu_light_serial;oldest->stamp=++fh_cpu_light_clock;fh_cpu_light_state_misses++;return oldest->id;
}
static FhCpuLightEntry *fh_cpu_light_probe(unsigned id,const FhCpuLightInput *input,int *hit){
    unsigned hash=fh_hash_bytes(id,input,sizeof(*input));
    /* Float coordinates often have zero low mantissa bits. Fold high bits
     * before indexing sets; equality still checks the complete original key. */
    unsigned bucket=hash^(hash>>16);bucket*=0x85ebca6bu;bucket^=bucket>>13;
    unsigned base=(bucket&(FH_CPU_LIGHT_SETS-1))*FH_CPU_LIGHT_WAYS;
    FhCpuLightEntry *oldest=fh_cpu_light_values+base;
    for(unsigned i=0;i<FH_CPU_LIGHT_WAYS;i++){
        FhCpuLightEntry *v=fh_cpu_light_values+base+i;
        if(v->id==id && v->hash==hash && !fh_memory_compare(&v->input,input,sizeof(*input))){v->stamp=++fh_cpu_light_clock;*hit=1;fh_cpu_light_hits++;return v;}
        if(v->stamp<oldest->stamp)oldest=v;
    }
    *hit=0;fh_cpu_light_misses++;return oldest;
}
static void fh_cpu_light_store(FhCpuLightEntry *slot,unsigned id,const FhCpuLightInput *input,const float output[2][4]){
    slot->input=*input;fh_copy_bytes(slot->output,output,sizeof(slot->output));slot->id=id;
    slot->hash=fh_hash_bytes(id,input,sizeof(*input));slot->stamp=++fh_cpu_light_clock;
}
#endif
