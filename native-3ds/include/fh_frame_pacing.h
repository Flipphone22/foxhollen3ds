#ifndef FH_FRAME_PACING_H
#define FH_FRAME_PACING_H
#include <stdint.h>
/* One display tick is consumable once. Missed ticks are not a backlog of
 * frames to simulate: game time continues to come from its own stopwatch.
 * Equality also handles the hardware counter wrapping through zero. */
typedef struct { uint32_t consumed; unsigned initialized; } FhFramePacing;
static inline int fh_frame_pacing_wait(const FhFramePacing *p,uint32_t now) {
    return !p->initialized || p->consumed==now;
}
static inline void fh_frame_pacing_consume(FhFramePacing *p,uint32_t now) {
    p->consumed=now;p->initialized=1;
}
#endif
