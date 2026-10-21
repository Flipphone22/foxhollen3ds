#ifndef FH_AUDIO_WORKER_H
#define FH_AUDIO_WORKER_H
void fh_audio_lock(void);
void fh_audio_unlock(void);
int fh_audio_try_lock(void);
void fh_audio_worker_start(int (*step)(void));
int fh_audio_worker_active(void);
void fh_audio_worker_stop(void);
void fh_audio_worker_wake(void);
void fh_audio_worker_stats(unsigned *active,unsigned *jobs,unsigned *stack,unsigned *core);
extern unsigned fh_audio_shared_executor_enabled;
/* Optional immutable callback on the existing core-2 owner. The caller must
 * drain its published work before detaching. A racing empty callback may finish
 * afterward, so its code and empty queue must have process lifetime. */
int fh_audio_worker_attach(int (*step)(void));
void fh_audio_worker_detach(void);
int fh_audio_worker_is_current(void);
void fh_audio_worker_service_current(void);
unsigned fh_audio_worker_stack_bytes(void);
/* Movie state advances on main; immutable movie PCM and ordered synth/copy
 * commands execute on the existing audio owner. No movie state is shared. */
extern unsigned fh_audio_pipeline_enabled,fh_audio_pipeline_worker,fh_audio_pipeline_inline;
extern unsigned fh_audio_pipeline_packets,fh_audio_pipeline_peak;
void fh_audio_movie_mix(short *destination,short *source,unsigned samples,
    void (*mix)(short *,short *,unsigned));
#endif
