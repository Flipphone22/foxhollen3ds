#include "fh_audio_effect_epoch.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "musyx/musyx.h"
extern bool ReverbSTDCreate(_SND_REVSTD_WORK *,float,float,float,float,float);
extern bool ReverbSTDModify(_SND_REVSTD_WORK *,float,float,float,float,float);
extern void ReverbSTDFree(_SND_REVSTD_WORK *);
extern void ReverbSTDCallback(int32_t *,int32_t *,int32_t *,_SND_REVSTD_WORK *);
void *salMalloc(size_t n){void *p=calloc(1,n);assert(p);return p;}
void salFree(void *p){free(p);}
static unsigned blocks,assertions;
#define CHECK(x) do {assertions++;assert(x);} while(0)
static void exercise_reverb(FhAudioEffectEpochs *pool,unsigned token,_SND_REVSTD_WORK *ref,unsigned seed) {
    int32_t a[3][160],b[3][160];
    for(unsigned c=0;c<3;c++)for(unsigned i=0;i<160;i++)a[c][i]=b[c][i]=(int32_t)((i*743+c*311+seed*193)%12000)-6000;
    _SND_REVSTD_WORK *rv=fh_audio_effect_begin(pool,token);CHECK(rv!=NULL);
    CHECK(!fh_audio_effect_begin(pool,token));
    CHECK(!fh_audio_effect_retire_before(pool,token+1));
    uint32_t collected=0;void *payload=NULL;
    CHECK(!fh_audio_effect_collect(pool,&collected,&payload));
    ReverbSTDCallback(a[0],a[1],a[2],ref);ReverbSTDCallback(b[0],b[1],b[2],rv);
    CHECK(!memcmp(a,b,sizeof(a)));CHECK(!fh_audio_effect_end(pool,token+1));
    CHECK(fh_audio_effect_end(pool,token));blocks++;
}
static _SND_REVSTD_WORK *create(void) {
    _SND_REVSTD_WORK *p=calloc(1,sizeof(*p));assert(p);
    assert(ReverbSTDCreate(p,.6f,1.2f,.65f,.4f,.013f));return p;
}
static void destroy(void *p){ReverbSTDFree(p);free(p);}
static void deterministic(void) {
    FhAudioEffectEpochs pool={0};uint32_t token;void *payload;
    _SND_REVSTD_WORK *old=create(),*next=create(),*ref_old=create(),*ref_next=create();
    CHECK(fh_audio_effect_publish(&pool,1,old)==FH_AUDIO_EFFECT_OK);
    CHECK(fh_audio_effect_publish(&pool,2,old)==FH_AUDIO_EFFECT_INVALID);
    for(unsigned i=0;i<40;i++)exercise_reverb(&pool,1,ref_old,i);
    /* Source re-creation with IDENTICAL parameters resets delay history; a
     * synth-clock-only reset does not call this lifecycle operation. */
    int32_t scratch[3][160];memset(scratch,0,sizeof(scratch));scratch[0][0]=12000;
    ReverbSTDCallback(scratch[0],scratch[1],scratch[2],ref_next);
    CHECK(ReverbSTDModify(ref_next,.6f,1.2f,.65f,.4f,.013f));
    CHECK(fh_audio_effect_publish(&pool,2,next)==FH_AUDIO_EFFECT_OK);
    FhAudioEffectEpochs unchanged=pool;
    CHECK(fh_audio_effect_publish(&pool,3,ref_next)==FH_AUDIO_EFFECT_BUSY);
    CHECK(!memcmp(&pool,&unchanged,sizeof(pool)));
    /* New producer effect is prepared, but three old returned blocks still
     * need the first effect, including its accumulated tail. */
    for(unsigned i=40;i<43;i++)exercise_reverb(&pool,1,ref_old,i);
    CHECK(!fh_audio_effect_begin(&pool,0));CHECK(!fh_audio_effect_retire_before(&pool,0));
    exercise_reverb(&pool,2,ref_next,43);
    CHECK(fh_audio_effect_retire_before(&pool,2));
    CHECK(fh_audio_effect_publish(&pool,3,old)==FH_AUDIO_EFFECT_INVALID);
    CHECK(fh_audio_effect_collect(&pool,&token,&payload) && token==1 && payload==old);destroy(payload);
    CHECK(!fh_audio_effect_collect(&pool,&token,&payload));CHECK(!fh_audio_effect_begin(&pool,1));
    CHECK(fh_audio_effect_publish(&pool,1,ref_old)==FH_AUDIO_EFFECT_INVALID);
    CHECK(!fh_audio_effect_retire_before(&pool,1));
    old=create();destroy(ref_old);ref_old=create();
    CHECK(fh_audio_effect_publish(&pool,3,old)==FH_AUDIO_EFFECT_OK);
    for(unsigned i=44;i<55;i++)exercise_reverb(&pool,2,ref_next,i);
    CHECK(fh_audio_effect_retire_before(&pool,3));
    CHECK(fh_audio_effect_collect(&pool,&token,&payload) && token==2 && payload==next);destroy(payload);
    CHECK(!fh_audio_effect_begin(&pool,2));
    for(unsigned i=55;i<80;i++)exercise_reverb(&pool,3,ref_old,i);
    CHECK(fh_audio_effect_retire_before(&pool,UINT32_MAX));
    CHECK(fh_audio_effect_collect(&pool,&token,&payload) && token==3 && payload==old);destroy(payload);
    CHECK(fh_audio_effect_publish(&pool,4,ref_old)==FH_AUDIO_EFFECT_INVALID);
    destroy(ref_old);destroy(ref_next);
}

enum { ITERATIONS=20000,Q=8 };
typedef struct { uint32_t token,guard,values[64],seen; } Payload;
static FhAudioEffectEpochs concurrent;
static unsigned head,tail,entries[Q],consumed;
static void *consumer(void *unused) {
    (void)unused;
    for(unsigned n=1;n<=ITERATIONS;n++) {
        unsigned end;
        do {end=__atomic_load_n(&head,__ATOMIC_ACQUIRE);if(end==tail)sched_yield();}while(end==tail);
        unsigned token=entries[tail%Q];assert(token==n);
        Payload *p=fh_audio_effect_begin(&concurrent,token);assert(p && p->token==token && p->guard==0xa5a55a5a);
        for(unsigned i=0;i<64;i++){assert(p->values[i]==(token^i));p->values[i]+=token;}
        if(!(n%37))sched_yield();
        p->seen=1;assert(fh_audio_effect_end(&concurrent,token));
        assert(fh_audio_effect_retire_before(&concurrent,token));
        __atomic_store_n(&tail,tail+1,__ATOMIC_RELEASE);
    }
    assert(fh_audio_effect_retire_before(&concurrent,ITERATIONS+1));
    return NULL;
}
static void collect(void) {
    uint32_t token;void *ptr;
    while(fh_audio_effect_collect(&concurrent,&token,&ptr)) {
        Payload *p=ptr;assert(p->token==token && p->guard==0xa5a55a5a && p->seen==1);
        for(unsigned i=0;i<64;i++)assert(p->values[i]==(token^i)+token);
        memset(p,0x5a,sizeof(*p));free(p);consumed++;
    }
}
static void threaded(void) {
    pthread_t thread;assert(!pthread_create(&thread,NULL,consumer,NULL));
    for(unsigned n=1;n<=ITERATIONS;n++) {
        Payload *p=calloc(1,sizeof(*p));assert(p);p->token=n;p->guard=0xa5a55a5a;
        for(unsigned i=0;i<64;i++)p->values[i]=n^i;
        int result;
        do {collect();result=fh_audio_effect_publish(&concurrent,n,p);assert(result!=FH_AUDIO_EFFECT_INVALID);
            if(result==FH_AUDIO_EFFECT_BUSY)sched_yield();}while(result==FH_AUDIO_EFFECT_BUSY);
        assert(head-__atomic_load_n(&tail,__ATOMIC_ACQUIRE)<Q);entries[head%Q]=n;
        __atomic_store_n(&head,head+1,__ATOMIC_RELEASE);
    }
    assert(!pthread_join(thread,NULL));collect();assert(consumed==ITERATIONS);
}
int main(void) {
    deterministic();threaded();
    printf("{\"passed\":true,\"assertions\":%u,\"reverb_blocks\":%u,\"reverb_samples\":%u,\"threaded_payloads\":%u,\"host_owner_bytes\":%u}\n",
        assertions,blocks,blocks*480,consumed,(unsigned)sizeof(FhAudioEffectEpochs));
    return 0;
}
