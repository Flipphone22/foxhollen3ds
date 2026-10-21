#ifndef FH_GX_DISPATCH_H
#define FH_GX_DISPATCH_H
#include "fh_gx_matrix_block.h"
/* Experimental handoff: mode 1 state only; mode 2 also translates explicitly
 * leased model lists inside the audited fur scope. Scope end retires all CPU
 * readers before the game can change/release the borrowed source ranges.
 * Experimental owned mode 2 can defer that join only if no borrowed draw
 * remains outstanding; synchronous renderer access joins it explicitly. */
/* Copies scope metadata into the command stream. Returns zero when no reader
 * was started, so the caller can establish the synchronous reader scope. */
int fh_gx_dispatch_begin(const void *const addresses[4],const unsigned bytes[4]);
void fh_gx_dispatch_end(void);
void fh_gx_dispatch_barrier(void);
/* Join a prior owned scope before touching synchronous renderer state. */
void fh_gx_dispatch_join_deferred(void);
void fh_gx_dispatch_stop(void);
void fh_gx_dispatch_lease_list(const void *list,unsigned bytes);
/* Joint matrices are copied by GXLoad*; their source is never borrowed. */
void fh_gx_dispatch_matrix_range(const void *matrices,unsigned bytes);
int fh_gx_dispatch_cache_write(const void *address,unsigned bytes);
extern unsigned fh_gx_dispatch_enabled;
extern unsigned fh_gx_matrix_filter_enabled,fh_gx_matrix_filtered_records,fh_gx_matrix_filter_checks;
extern unsigned fh_gx_dispatch_batch_enabled,fh_gx_dispatch_batches,fh_gx_dispatch_batch_records;
extern unsigned fh_gx_dispatch_records,fh_gx_dispatch_scopes,fh_gx_dispatch_waits;
extern unsigned fh_gx_dispatch_draws,fh_gx_dispatch_direct_draws;
extern unsigned fh_gx_dispatch_barrier_counts[64];
void fh_gx_dispatch_stats(unsigned *active,unsigned *core,unsigned *stack_peak);
int fh_gx_dispatch_is_worker(void);
#endif
