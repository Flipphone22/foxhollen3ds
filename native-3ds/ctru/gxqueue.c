/* Altered libctru gxqueue.c: explicitly committed native-renderer prefixes.
 * Original: devkitPro/libctru, libctru/source/gpu/gxqueue.c.
 * See native-3ds/licenses/libctru-zlib.txt. Compiled only into private libctru.
 * Ordinary SDK queues retain automatic append dispatch. A held queue stages
 * additions until Run; Wait commits its complete tail before joining it.
 * Storage, order, three in-flight slots, IRQ forwarding and callbacks remain.
 */
#include <stdlib.h>
#include <string.h>
#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/synchronization.h>
#include <3ds/gpu/gx.h>
#include <3ds/services/gspgpu.h>
#define MAX_PARALLEL_CMDS 3

static gxCmdQueue_s *curQueue, *heldQueue;
static unsigned committed;
static bool isActive, isRunning, shouldStop;
static LightLock queueLock = 1;

static unsigned queueLimit(void) {
    return curQueue == heldQueue ? committed : curQueue->numEntries;
}
static void gxCmdQueueDoCommands(void) {
    if (shouldStop) return;
    int batchSize = curQueue->lastEntry + MAX_PARALLEL_CMDS - curQueue->curEntry;
    while (curQueue->curEntry < queueLimit() && batchSize--) {
        gxCmdEntry_s *entry = &curQueue->entries[curQueue->curEntry++];
        gspSubmitGxCommand(entry->data);
    }
}
void fh_gx_queue_hold(gxCmdQueue_s *queue) {
    LightLock_Lock(&queueLock);
    if (heldQueue && heldQueue != queue) svcBreak(USERBREAK_PANIC);
    if (!heldQueue) {
        heldQueue = queue;
        /* Never retract commands that were already submitted. */
        committed = queue->curEntry;
    }
    LightLock_Unlock(&queueLock);
}
void gxCmdQueueInterrupt(GSPGPU_Event irq) {
    if (irq == GSPGPU_EVENT_PSC1 || irq == GSPGPU_EVENT_VBlank0 ||
        irq == GSPGPU_EVENT_VBlank1) return;
    gxCmdQueue_s *runCb = NULL;
    LightLock_Lock(&queueLock);
    if (!isRunning) { LightLock_Unlock(&queueLock); return; }
    curQueue->lastEntry++;
    if (shouldStop) {
        curQueue = NULL; isActive = false; isRunning = false; shouldStop = false;
    } else if (curQueue->lastEntry < queueLimit()) {
        gxCmdQueueDoCommands();
    } else {
        runCb = curQueue; isRunning = false;
    }
    LightLock_Unlock(&queueLock);
    if (runCb && runCb->callback) runCb->callback(runCb);
}
void gxCmdQueueClear(gxCmdQueue_s *queue) {
    LightLock_Lock(&queueLock);
    if ((queue == curQueue && isRunning) ||
        (queue == heldQueue && queue->lastEntry != queue->numEntries))
        svcBreak(USERBREAK_PANIC);
    queue->numEntries = queue->curEntry = queue->lastEntry = 0;
    if (queue == heldQueue) { heldQueue = NULL; committed = 0; }
    LightLock_Unlock(&queueLock);
}
void gxCmdQueueAdd(gxCmdQueue_s *queue, const gxCmdEntry_s *entry) {
    if (queue->numEntries == queue->maxEntries) svcBreak(USERBREAK_PANIC);
    memcpy(&queue->entries[queue->numEntries], entry, sizeof(gxCmdEntry_s));
    LightLock_Lock(&queueLock);
    queue->numEntries++;
    if (queue != heldQueue && queue == curQueue && isActive && !isRunning) {
        isRunning = true; gxCmdQueueDoCommands();
    }
    LightLock_Unlock(&queueLock);
}
void gxCmdQueueRun(gxCmdQueue_s *queue) {
    LightLock_Lock(&queueLock);
    if (isRunning && curQueue != queue) {
        LightLock_Unlock(&queueLock); return;
    }
    if (queue == heldQueue) committed = queue->numEntries;
    curQueue = queue; isActive = true;
    isRunning = queue->lastEntry < queueLimit();
    if (isRunning) gxCmdQueueDoCommands();
    LightLock_Unlock(&queueLock);
}
void gxCmdQueueStop(gxCmdQueue_s *queue) {
    (void)queue;
    LightLock_Lock(&queueLock);
    if (curQueue) {
        if (!isRunning) { curQueue = NULL; isActive = false; }
        else shouldStop = true;
    }
    LightLock_Unlock(&queueLock);
}
bool gxCmdQueueWait(gxCmdQueue_s *queue, s64 timeout) {
    /* A drained prefix is not a completed frame or permission to free data. */
    if (queue == heldQueue) gxCmdQueueRun(queue);
    u64 deadline = U64_MAX;
    if (timeout >= 0) deadline = svcGetSystemTick() + timeout;
    for (;;) {
        LightLock_Lock(&queueLock);
        bool running = isRunning;
        LightLock_Unlock(&queueLock);
        if (!running) return true;
        if (timeout >= 0 && (s64)(u64)(svcGetSystemTick()-deadline) >= 0) return false;
        gspWaitForAnyEvent();
    }
}
