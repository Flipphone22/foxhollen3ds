#ifndef FH_INSTALL_REDUCE_H
#define FH_INSTALL_REDUCE_H
#include "fh_install_lod_guards.h"
enum {FH_REDUCE_ERROR=0,FH_REDUCE_KEEP=1,FH_REDUCE_CHANGED=2};
enum {FH_REDUCE_SMALL,FH_REDUCE_CONTEXT,FH_REDUCE_NO_GAIN,FH_REDUCE_PROTECTED,FH_REDUCE_CAPACITY,FH_REDUCE_SURFACE};
typedef struct {
    unsigned char*data;
    unsigned bytes,before,after,contexts,components,reason;
    unsigned attempts,fidelity_rejected;
    float accepted_error;
    size_t simplify_scratch;
    double position_error,uv_error,color_error;
    char message[112];
} FhInstallReduction;
/* Startup only. Guards borrow the validated original model. Processes one
 * source list, all its material contexts, and independently bounded connected
 * features. Only CHANGED exposes a candidate; it fits original storage.
 * KEEP retains the source unchanged and names why; ERROR aborts preparation.
 * Ratio/error are simplifier targets, not visual acceptance thresholds. */
int fh_install_reduce_list(FhInstallReduction*,FhInstallLodGuards*,unsigned list,float ratio,float error);
void fh_install_reduce_close(FhInstallReduction*);
#endif
