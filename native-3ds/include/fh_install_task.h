#ifndef FH_INSTALL_TASK_H
#define FH_INSTALL_TASK_H
#include "fh_install_bundle.h"
#include "fh_startup_runner.h"

/* Startup-only, borrowed immutable configuration. No game/renderer lifetime.
 * Recipe keys must cover converter sources AND the fixed options below. The
 * expected source identity is supplied by the build, never read from a pending
 * journal. Test fixtures supply their own explicitly computed identity. */
enum {FH_INSTALL_TASK_TEXTURE_EDGE=128,FH_INSTALL_TASK_MOVIE_SCALE=2,
      FH_INSTALL_TASK_CACHE_LEVEL=0};
#define FH_INSTALL_TASK_RATIO .2f
#define FH_INSTALL_TASK_ERROR .035f
typedef struct {
    const char *disc_path,*root;
    FhInstallBundleSpec spec;
    FhInstallBundle selected;
    unsigned busy;
} FhInstallTask;
/* Direct FhStartupTasks callbacks. ready checks disc header/FST/size and the
 * bundle's manifests/file sizes. It does NOT fingerprint the whole ISO. */
unsigned fh_install_task_ready(void*);
/* Full source hash once per Prepare/resume invocation, then four sequential
 * real converters and final bundle verification/publication. Returns 1/0/-1.
 * All files/scratch are retired before returning, including cancellation. */
int fh_install_task_prepare(void*,FhStartupProgress,void*,char*,size_t);
#endif
