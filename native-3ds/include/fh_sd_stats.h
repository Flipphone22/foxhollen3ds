#ifndef FH_SD_STATS_H
#define FH_SD_STATS_H
#include <stdint.h>
typedef struct {uint64_t read_bytes,read_ticks,read_max_ticks,all_ticks;} FhSdDisplay;
void fh_sd_stats_snapshot(FhSdDisplay *out);
void fh_sd_stats_init(void);
void fh_sd_stats_begin_run(void);
void fh_sd_stats_observe(unsigned retrace);
void fh_sd_stats_shutdown(unsigned retrace);
#endif
