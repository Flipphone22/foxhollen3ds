#ifndef FH_AUDIO_BENCH_MODE_H
#define FH_AUDIO_BENCH_MODE_H
/* Startup thread only, before audio workers exist. Zero is the CPU mixer. */
unsigned fh_audio_bench_mode(void);
int fh_audio_bench_select(unsigned native_dsp);
unsigned fh_audio_bench_latch(void);
/* Startup-only opt-in; 0 shares the game core, 1 requests 30% system-core time.
 * Old hardware consumes this selection. The default and New core2 are unchanged. */
unsigned fh_audio_bench_old_core(void);
int fh_audio_bench_select_old_core(unsigned core);
#endif
