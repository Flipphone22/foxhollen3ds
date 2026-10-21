/* Adapted from 2gifts/melee-3ds c2ebe7c769b3287f2b3748173e8fe5ec24ef408e.
 * GPL-3.0; see native-3ds/licenses/melee-GPL-3.0.txt and ADAPTATIONS.md. */
#ifndef FH_EARLY_QUEUE_H
#define FH_EARLY_QUEUE_H
#define FH_ASYNC_PRESENTATION
/* In-frame submission. A completed command prefix uses
 * immutable storage; the caller flushes all newly written GPU resources.
 * The adapted SDK publishes all presentation transfers atomically to its
 * completion callback. Original-SDK builds retain the final stop/clear fence. */
extern unsigned fh_early_queue_starts,fh_early_queue_finishes;
extern unsigned fh_early_queue_runs,fh_early_queue_appends,fh_early_queue_capacity;
extern u64 fh_early_queue_wait_ticks,fh_early_queue_span_ticks;
int fh_early_queue_submit(void);
int fh_early_queue_append(void);
void fh_early_queue_finish(void);
#ifdef FH_ASYNC_PRESENTATION
extern unsigned fh_early_queue_deferrals,fh_early_queue_retirements;
void fh_early_queue_defer(void);
void fh_early_queue_retire(void);
void fh_early_queue_capacity_completed(u64 wait_ticks);
#endif
#endif
