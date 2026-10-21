#ifndef FH_STACK_STATS_H
#define FH_STACK_STATS_H
typedef struct {
    unsigned slot,bytes,observed_used,canary_ok;
} FhGameStackStats;
/* Active cooperative game threads only. Storage is already included in the
 * game arena/normal heap; these figures must not be added to RAM totals. */
unsigned fh_game_stack_stats(FhGameStackStats *out,unsigned capacity);
#endif
