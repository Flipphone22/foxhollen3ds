#ifndef TEST_CTRU_IO_H
#define TEST_CTRU_IO_H
#define _GNU_SOURCE
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
typedef uint64_t u64;typedef uint32_t u32;typedef int32_t s32;
#define SYSCLOCK_ARM11 1000000000ULL
#define CUR_THREAD_HANDLE 0
#define RESET_ONESHOT 0
#define R_FAILED(r) ((r)!=0)
typedef enum {APTHOOK_ONSUSPEND,APTHOOK_ONSLEEP,APTHOOK_ONRESTORE,APTHOOK_ONWAKEUP} APT_HookType;
typedef struct aptHookCookie {void (*fn)(APT_HookType,void*);void *arg;struct aptHookCookie *next;} aptHookCookie;
extern aptHookCookie *test_hooks;
extern void (*test_apt_callback)(APT_HookType,void*);
static inline void dispatch_hooks(APT_HookType type,void *unused){(void)unused;for(aptHookCookie *h=test_hooks;h;h=h->next)h->fn(type,h->arg);}
static inline void aptHook(aptHookCookie *c,void (*fn)(APT_HookType,void*),void *arg){c->fn=fn;c->arg=arg;c->next=test_hooks;test_hooks=c;test_apt_callback=dispatch_hooks;}
static inline void aptUnhook(aptHookCookie *c){aptHookCookie **h=&test_hooks;while(*h && *h!=c)h=&(*h)->next;assert(*h);*h=c->next;if(!test_hooks)test_apt_callback=NULL;}
static inline u64 svcGetSystemTick(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (u64)t.tv_sec*SYSCLOCK_ARM11+t.tv_nsec;}
static inline int svcGetThreadPriority(s32 *p,int h){(void)h;*p=0x30;return 0;}
typedef struct {pthread_mutex_t mutex;pthread_cond_t cond;int initialized,signal;} LightEvent;
static inline struct timespec deadline(u64 ns){struct timespec t;clock_gettime(CLOCK_REALTIME,&t);u64 n=t.tv_nsec+ns;t.tv_sec+=n/1000000000;t.tv_nsec=n%1000000000;return t;}
static inline void LightEvent_Init(LightEvent *e,int mode){(void)mode;
    if(e->initialized){pthread_mutex_destroy(&e->mutex);pthread_cond_destroy(&e->cond);}
    pthread_mutex_init(&e->mutex,NULL);pthread_cond_init(&e->cond,NULL);e->signal=0;e->initialized=1;}
static inline void LightEvent_Signal(LightEvent *e){pthread_mutex_lock(&e->mutex);e->signal=1;pthread_cond_signal(&e->cond);pthread_mutex_unlock(&e->mutex);}
static inline void LightEvent_WaitTimeout(LightEvent *e,u64 ns){struct timespec end=deadline(ns);pthread_mutex_lock(&e->mutex);
    while(!e->signal){if(pthread_cond_timedwait(&e->cond,&e->mutex,&end)==ETIMEDOUT)break;}
    e->signal=0;pthread_mutex_unlock(&e->mutex);}
typedef struct TestThread{pthread_t thread;void (*fn)(void*);void *arg;}*Thread;
static inline void *entry(void *p){Thread t=p;t->fn(t->arg);return NULL;}
static inline Thread threadCreate(void (*fn)(void*),void *arg,unsigned stack,int priority,int core,bool detached){
    (void)stack;(void)priority;(void)core;assert(!detached);Thread t=calloc(1,sizeof(*t));assert(t);t->fn=fn;t->arg=arg;
    if(pthread_create(&t->thread,NULL,entry,t)){free(t);return NULL;}return t;}
static inline int threadJoin(Thread t,u64 ns){if(ns==UINT64_MAX)return pthread_join(t->thread,NULL);struct timespec end=deadline(ns);return pthread_timedjoin_np(t->thread,NULL,&end);}
static inline void threadFree(Thread t){free(t);}
#endif
