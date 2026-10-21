#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef uint64_t u64; typedef int64_t s64; typedef int LightLock;
#define U64_MAX UINT64_MAX
enum { USERBREAK_PANIC, GSPGPU_EVENT_PSC1, GSPGPU_EVENT_VBlank0,
       GSPGPU_EVENT_VBlank1, GSPGPU_EVENT_P3D };
typedef unsigned GSPGPU_Event;
typedef struct {uint32_t data[8];} gxCmdEntry_s;
typedef struct Queue {gxCmdEntry_s *entries;unsigned maxEntries,numEntries,curEntry,lastEntry;
 void (*callback)(struct Queue *);} gxCmdQueue_s;
static unsigned sent,done,allow,callbacks,device[1024],nextId,blocked;
static u64 tick;
static void LightLock_Lock(LightLock *l){assert(*l==1);*l=0;}
static void LightLock_Unlock(LightLock *l){assert(*l==0);*l=1;}
static void svcBreak(int code){(void)code;abort();}
static u64 svcGetSystemTick(void){return ++tick;}
static void gspSubmitGxCommand(const uint32_t *data){
 assert(sent<allow);assert(sent-done<3);assert(data[0]==sent);
 device[sent++]=data[0];
}
void gxCmdQueueInterrupt(GSPGPU_Event irq);
static void gspWaitForAnyEvent(void){
 if(blocked)return;
 assert(done<sent);assert(device[done]==done);done++;
 gxCmdQueueInterrupt(GSPGPU_EVENT_P3D);
}
#include "batch-under-test.inc"
static void callback(gxCmdQueue_s*q){assert(queueLock==1);assert(q->lastEntry<=q->numEntries);callbacks++;}
static uint32_t rng=0x852470b1;
static uint32_t random32(void){rng^=rng<<13;rng^=rng>>17;return rng^=rng<<5;}
static void add(gxCmdQueue_s*q){gxCmdEntry_s e={{nextId++}};gxCmdQueueAdd(q,&e);}
int main(void){
 gxCmdEntry_s entries[1024];gxCmdQueue_s q={.entries=entries,.maxEntries=1024,.callback=callback};
 for(unsigned run=0;run<5000;run++){
  sent=done=nextId=allow=0;fh_gx_queue_hold(&q);
  for(unsigned j=0;j<400;j++){
   unsigned op=random32()%5;
   if(op<3)add(&q);
   else if(op==3){
    allow=q.numEntries;gxCmdQueueRun(&q);
    if((random32()&7)==0){while(done<sent)gspWaitForAnyEvent();assert(done==allow);}
   }
   else if(done<sent)gspWaitForAnyEvent();
   assert(sent<=allow);assert(q.lastEntry==done);assert(q.curEntry==sent);
   gxCmdQueueInterrupt(GSPGPU_EVENT_VBlank0);
   gxCmdQueueInterrupt(GSPGPU_EVENT_PSC1);assert(q.lastEntry==done);
  }
  /* Wait must publish uncommitted tail and cannot return on prefix drain. */
  add(&q);allow=q.numEntries;blocked=1;
  assert(!gxCmdQueueWait(&q,0));assert(q.lastEntry<q.numEntries);
  blocked=0;assert(gxCmdQueueWait(&q,-1));assert(done==nextId);
  gxCmdQueueStop(&q);gxCmdQueueClear(&q);assert(!heldQueue && !q.numEntries);
 }
 /* Non-held SDK queues still dispatch additions automatically after Run. */
 sent=done=nextId=allow=0;
 allow=1;add(&q);assert(!sent);gxCmdQueueRun(&q);gspWaitForAnyEvent();
 allow=2;add(&q);assert(sent==2);assert(gxCmdQueueWait(&q,-1));
 gxCmdQueueStop(&q);gxCmdQueueClear(&q);
 printf("PASS 5000 randomized prefix schedules, tail waits, ignored IRQs, ordered commands, <=3 in-flight; callbacks=%u\n",callbacks);
}
