#ifndef FH_INSTALL_WORKER_H
#define FH_INSTALL_WORKER_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*FhInstallWork)(void*);
typedef struct {unsigned active,core,jobs,stack_bytes,stack_peak;} FhInstallWorkerStats;
/* Startup-only single borrowed job. Producer joins before freeing inputs or
 * outputs. No worker SD/UI/GPU access. Stop joins before game/audio startup. */
#if defined(__3DS__) || defined(FH_INSTALL_WORKER_TEST)
void fh_install_worker_start(unsigned hardware_new);
int fh_install_worker_submit(FhInstallWork,void*);
int fh_install_worker_complete(void);
void fh_install_worker_wait_slice(void);
void fh_install_worker_join(void);
void fh_install_worker_stop(void);
void fh_install_worker_stats(FhInstallWorkerStats*);
#else
/* Portable tools use exactly the same serial codec; host thread tests supply
 * the real owner through the explicit test define instead of this adapter. */
static inline void fh_install_worker_start(unsigned n){(void)n;}
static inline int fh_install_worker_submit(FhInstallWork f,void*p){(void)f;(void)p;return 0;}
static inline int fh_install_worker_complete(void){return 1;}
static inline void fh_install_worker_wait_slice(void){}
static inline void fh_install_worker_join(void){}
static inline void fh_install_worker_stop(void){}
static inline void fh_install_worker_stats(FhInstallWorkerStats*s){s->active=s->jobs=s->stack_bytes=s->stack_peak=0;s->core=~0u;}
#endif
#ifdef __cplusplus
}
#endif
#endif
