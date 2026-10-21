#include <dolphin/os.h>
#include <string.h>
#include "fh_platform.h"
#include "fh_stack_stats.h"

/* Adapted from pinned melee-3ds scheduler.c; all contexts stay little endian. */
typedef struct { u32 core[10]; u64 vfp[8]; } ArmContext;
typedef struct {
    OSThread *thread;
    void *(*entry)(void *);
    void *argument;
    ArmContext context;
    unsigned char *stack_low;
    unsigned stack_bytes,stack_observed,active;
} Task;
static Task tasks[16], *running;
static OSThread main_thread;
static int disabled;
extern void fh_context_swap(ArmContext *,ArmContext *);
extern void fh_thread_trampoline(void);
unsigned fh_game_stack_stats(FhGameStackStats *out,unsigned capacity) {
    unsigned count=0;
    for(unsigned i=1;i<16 && count<capacity;i++) {
        Task *t=&tasks[i];if(!t->active)continue;
        /* Stack ownership ends before cancellation/completion can release its
         * storage. Never inspect stale thread objects or retired stack bytes. */
        unsigned untouched=sizeof(u32);
        while(untouched<t->stack_bytes && t->stack_low[untouched]==0xa5)untouched++;
        unsigned used=t->stack_bytes-untouched;
        if(used>t->stack_observed)t->stack_observed=used;
        out[count++]=(FhGameStackStats){i,t->stack_bytes,t->stack_observed,*(u32*)t->stack_low==OS_THREAD_STACK_MAGIC};
    }
    return count;
}
static void init(void) {
    if(running) return;
    main_thread.state=OS_THREAD_STATE_RUNNING;
    main_thread.priority=main_thread.base=16;
    tasks[0].thread=&main_thread; running=&tasks[0];
}
static Task *find(OSThread *thread) {
    init();
    for(unsigned i=0;i<16;i++) if(tasks[i].thread==thread) return &tasks[i];
    fh_platform_panic("Unknown game thread");
}
static void guard(Task *task) {
    if(task!=&tasks[0] && *(u32*)task->thread->stackEnd!=OS_THREAD_STACK_MAGIC)
        fh_platform_panic("Game thread stack overflow");
}
static void schedule(void) {
    init();
    if(disabled) return;
    guard(running);
    Task *next=NULL;
    for(unsigned pass=0;pass<2 && !next;pass++) {
        if(pass) fh_dvd_pump();
        for(unsigned i=0;i<16;i++) {
            OSThread *t=tasks[i].thread;
            if(t && !t->suspend && (t->state==OS_THREAD_STATE_READY || t->state==OS_THREAD_STATE_RUNNING)
               && (!next || t->priority<next->thread->priority)) next=&tasks[i];
        }
    }
    if(!next) fh_platform_panic("Game scheduler deadlock: no runnable thread");
    if(next!=running) {
        Task *old=running;
        if(old->thread->state==OS_THREAD_STATE_RUNNING) old->thread->state=OS_THREAD_STATE_READY;
        guard(next); running=next; next->thread->state=OS_THREAD_STATE_RUNNING;
        fh_context_swap(&old->context,&next->context);
        guard(running);
    }
}
static void unlink_waiter(OSThread *t) {
    if(!t->queue) return;
    if(t->link.prev) t->link.prev->link.next=t->link.next;
    else t->queue->head=t->link.next;
    if(t->link.next) t->link.next->link.prev=t->link.prev;
    else t->queue->tail=t->link.prev;
    t->queue=NULL; t->link.next=t->link.prev=NULL;
}
void fh_thread_start(Task *task) {
    task->thread->val=task->entry(task->argument);
    task->active=0;
    task->thread->state=OS_THREAD_STATE_MORIBUND;
    OSWakeupThread(&task->thread->queueJoin);
    schedule();
    fh_platform_panic("Exited game thread resumed");
}
BOOL OSCreateThread(OSThread *thread,void *(*entry)(void*),void *argument,void *stack,u32 bytes,OSPriority priority,u16 attributes) {
    init();
    if(!thread || !entry || !stack || bytes<1024 || ((uintptr_t)stack&7) || (bytes&7) || priority<0 || priority>31) return 0;
    for(unsigned i=1;i<16;i++) {
        Task *task=&tasks[i];
        if(task->thread && task->thread->state!=OS_THREAD_STATE_MORIBUND) continue;
        memset(task,0,sizeof(*task)); memset(thread,0,sizeof(*thread));
        task->thread=thread;task->entry=entry;task->argument=argument;
        task->stack_low=(u8*)stack-bytes;task->stack_bytes=bytes;task->active=1;
        thread->state=OS_THREAD_STATE_READY;thread->suspend=1;
        thread->priority=thread->base=priority;thread->attr=attributes;
        thread->stackBase=stack;thread->stackEnd=(u8*)stack-bytes;
        memset(thread->stackEnd,0xa5,bytes); *(u32*)thread->stackEnd=OS_THREAD_STACK_MAGIC;
        task->context.core[0]=(u32)(uintptr_t)task;
        task->context.core[8]=(u32)(uintptr_t)stack;
        task->context.core[9]=(u32)(uintptr_t)fh_thread_trampoline;
        return 1;
    }
    return 0;
}
OSThread *OSGetCurrentThread(void) { init();return running->thread; }
s32 OSResumeThread(OSThread *t) { find(t);s32 old=t->suspend;if(old) --t->suspend;schedule();return old; }
s32 OSSuspendThread(OSThread *t) { find(t);s32 old=t->suspend++;schedule();return old; }
void OSCancelThread(OSThread *t) { find(t)->active=0;unlink_waiter(t);t->state=OS_THREAD_STATE_MORIBUND;OSWakeupThread(&t->queueJoin);if(t==running->thread) schedule(); }
s32 OSDisableScheduler(void) { return disabled++; }
s32 OSEnableScheduler(void) { int old=disabled;if(disabled) --disabled;return old; }
void OSInitThreadQueue(OSThreadQueue *q) { q->head=q->tail=NULL; }
void OSSleepThread(OSThreadQueue *q) {
    OSThread *t=OSGetCurrentThread();
    if(disabled || t->queue) fh_platform_panic("Invalid blocking game thread state");
    t->state=OS_THREAD_STATE_WAITING;t->queue=q;t->link.next=NULL;t->link.prev=q->tail;
    if(q->tail) q->tail->link.next=t;else q->head=t;
    q->tail=t;schedule();
}
void OSWakeupThread(OSThreadQueue *q) {
    while(q->head) { OSThread *t=q->head;unlink_waiter(t);t->state=OS_THREAD_STATE_READY; }
}
void OSInitMessageQueue(OSMessageQueue *q,OSMessage *array,s32 count) {
    if(!q || !array || count<=0) fh_platform_panic("Invalid game message queue");
    memset(q,0,sizeof(*q));q->msgArray=array;q->msgCount=count;
}
BOOL OSSendMessage(OSMessageQueue *q,OSMessage message,s32 flags) {
    while(q->usedCount==q->msgCount) {
        if(!(flags&OS_MESSAGE_BLOCK)) return 0;
        OSSleepThread(&q->queueSend);
    }
    q->msgArray[(q->firstIndex+q->usedCount)%q->msgCount]=message;
    ++q->usedCount;OSWakeupThread(&q->queueReceive);return 1;
}
BOOL OSReceiveMessage(OSMessageQueue *q,OSMessage *message,s32 flags) {
    while(!q->usedCount) {
        fh_dvd_pump();if(q->usedCount) break;
        if(!(flags&OS_MESSAGE_BLOCK)) return 0;
        OSSleepThread(&q->queueReceive);
    }
    if(message) *message=q->msgArray[q->firstIndex];
    q->firstIndex=(q->firstIndex+1)%q->msgCount;
    --q->usedCount;OSWakeupThread(&q->queueSend);return 1;
}
