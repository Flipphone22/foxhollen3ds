#ifndef FH_PERF_DISPLAY_H
#define FH_PERF_DISPLAY_H
#include <stdint.h>
typedef struct {
    unsigned generation,gpu_generation;
    unsigned heap_used,heap_limit,linear_used,linear_limit,vram_used;
    unsigned audio_ms[2],audio_underruns[2],audio_valid;
    unsigned read_kib_s,read_ms,read_max_ms,fs_ms;
    unsigned gpu_valid,gpu_failed,gpu_words[4],gpu_ipc_us;
    unsigned frame_us,gpu_queue_us,gpu_timing_valid;
    unsigned cpu_valid,cpu_total_us,cpu_us[16];
} FhPerfDisplay;
extern FhPerfDisplay fh_perf_display;
void fh_perf_display_poll(void);
void fh_perf_display_shutdown(void);
void fh_perf_display_report(void);
/* Nonblocking observation: does not initialize/reset/refill NDSP. */
int fh_audio_display_snapshot(unsigned milliseconds[2],unsigned underruns[2]);
int fh_replay_gpu_display(unsigned words[4],unsigned *failed,unsigned *ipc_us);
#endif
