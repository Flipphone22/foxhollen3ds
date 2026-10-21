#ifndef STUB_3DS_H
#define STUB_3DS_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
typedef uint64_t u64;typedef uint32_t u32;typedef uint16_t u16;
typedef int32_t s32;
#define CUR_THREAD_HANDLE 0
#define RESET_ONESHOT 0
static inline void svcGetThreadPriority(s32*p,int h){(void)h;*p=0x30;}
typedef struct {pthread_mutex_t mutex;pthread_cond_t cond;int signaled;} LightEvent;
static inline void LightEvent_Init(LightEvent*e,int reset){(void)reset;pthread_mutex_init(&e->mutex,NULL);pthread_cond_init(&e->cond,NULL);e->signaled=0;}
static inline void LightEvent_Signal(LightEvent*e){pthread_mutex_lock(&e->mutex);e->signaled=1;pthread_cond_signal(&e->cond);pthread_mutex_unlock(&e->mutex);}
static inline void LightEvent_WaitTimeout(LightEvent*e,uint64_t ns){
 struct timespec end;clock_gettime(CLOCK_REALTIME,&end);end.tv_sec+=ns/1000000000;end.tv_nsec+=ns%1000000000;if(end.tv_nsec>=1000000000){end.tv_sec++;end.tv_nsec-=1000000000;}
 pthread_mutex_lock(&e->mutex);while(!e->signaled)if(pthread_cond_timedwait(&e->cond,&e->mutex,&end))break;e->signaled=0;pthread_mutex_unlock(&e->mutex);
}
#define GFX_BOTTOM 1
static inline void gfxScreenSwapBuffers(int screen,bool stereo){(void)screen;(void)stereo;}
#define U64_MAX UINT64_MAX
struct Worker {pthread_t id;void(*fn)(void*);void*arg;};typedef struct Worker*Thread;
static void*entry(void*p){Thread t=p;t->fn(t->arg);return 0;}
static inline Thread threadCreate(void(*fn)(void*),void*arg,size_t stack,int pri,int core,bool detached){(void)stack;(void)pri;(void)core;(void)detached;Thread t=calloc(1,sizeof(*t));t->fn=fn;t->arg=arg;if(pthread_create(&t->id,0,entry,t)){free(t);return 0;}return t;}
static inline int threadJoin(Thread t,u64 ns){(void)ns;return pthread_join(t->id,0);}
static inline void threadFree(Thread t){free(t);}
static inline u64 svcGetSystemTick(void){struct timespec s;clock_gettime(CLOCK_MONOTONIC,&s);return (u64)s.tv_sec*268111856+(u64)s.tv_nsec*268111856/1000000000;}
static inline void svcSleepThread(uint64_t ns){struct timespec s={ns/1000000000,ns%1000000000};nanosleep(&s,0);}
#endif
