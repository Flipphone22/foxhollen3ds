#ifndef FH_INSTALL_MODEL_REDUCE_H
#define FH_INSTALL_MODEL_REDUCE_H
#include "fh_install_reduce.h"
typedef int (*FhInstallModelReduceProgress)(void*,unsigned done,unsigned total);
typedef struct {
    unsigned char*patch,*model;
    unsigned patch_bytes,model_bytes,lists,changed,alias_kept,kept[6],before,after;
    size_t simplify_scratch;
    double position_error,uv_error,color_error;
    char error[112];
} FhInstallModelReduction;
/* Serialized FHMLD01 for the existing runtime patch owner, plus a privately
 * patched source for immediate native scene compilation. Original stays
 * immutable. No changes => patch/model NULL. Returns1 complete,0 error,-1
 * cancelled between source lists; no partial patch is exposed on failure. */
int fh_install_model_reduce(FhInstallModelReduction*,const void*,unsigned bytes,unsigned id,float ratio,float error,FhInstallModelReduceProgress,void*);
void fh_install_model_reduce_close(FhInstallModelReduction*);
#endif
