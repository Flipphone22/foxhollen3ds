#ifndef FH_REPLAY_RANDOM_TRACE_H
#define FH_REPLAY_RANDOM_TRACE_H
#ifdef FH_REPLAY_RANDOM_TRACE
void fh_replay_random_trace_begin(void);
void fh_replay_random_trace_frame(double);
int fh_replay_random_trace_finish(const char*);
#else
static inline void fh_replay_random_trace_begin(void){}
static inline void fh_replay_random_trace_frame(double tick){(void)tick;}
static inline int fh_replay_random_trace_finish(const char *path){(void)path;return 1;}
#endif
#endif
