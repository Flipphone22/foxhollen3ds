#ifndef FH_PLATFORM_H
#define FH_PLATFORM_H
#include <stddef.h>
#include <stdint.h>
void *fh_platform_alloc(size_t bytes, size_t alignment);
void fh_platform_free(void *memory);
void fh_platform_panic(const char *message) __attribute__((noreturn));
void fh_platform_log(const char *message);
/* Poll Select while GPU ownership is outstanding. Shutdown is deferred until
 * normal frame/input processing resumes; it must not release live GPU memory. */
int fh_platform_capture_requested(void);
void fh_platform_gpu_wait_poll(void);
void fh_platform_gpu_wait_log_flush(void);
uint64_t fh_platform_ticks(void);
void fh_platform_frame(void);
void fh_game_observe(unsigned retrace);
typedef struct {
    uint16_t buttons;
    int8_t x,y,cx,cy;
    uint8_t left,right,analog_a,analog_b;
    int8_t error;
    uint8_t reserved;
    uint32_t extended;
} FhPadState;
void fh_platform_pad(FhPadState *state);
int fh_platform_audio_space(unsigned channel);
unsigned fh_platform_audio_buffered(unsigned channel);
void fh_platform_audio_queue(unsigned channel,const int16_t *pcm,unsigned frames,unsigned rate);
void fh_platform_audio_reset(unsigned channel);
void fh_platform_audio_pause(unsigned channel,int paused);
void fh_platform_audio_finish(unsigned channel);
void fh_platform_audio_observe(unsigned retrace);
void fh_audio_pump(void);
void fh_platform_quit(void) __attribute__((noreturn));
void fh_gx_frame_complete(void);
void fh_gx_cache_range(void *memory,size_t bytes,int cpu_written);
void fh_platform_cache(void *memory, size_t bytes, int cpu_written);
void fh_dvd_pump(void);
void fh_dvd_shutdown(void);
int fh_dvd_mount(const char *path);
void fh_memory_shutdown(void);
size_t fh_memory_peak(void);
size_t fh_memory_used(void);
int fh_card_set_directory(const char *path);
typedef struct {unsigned bytes,retain_limit,cold_mounts,warm_mounts,releases,mounted,loaded;} FhCardStats;
void fh_card_stats(FhCardStats *out);
void fh_card_shutdown(void);
#endif
