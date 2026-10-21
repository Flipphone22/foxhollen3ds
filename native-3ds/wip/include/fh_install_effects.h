#ifndef FH_INSTALL_EFFECTS_H
#define FH_INSTALL_EFFECTS_H
#include "fh_install_task.h"
typedef struct {
    FhInstallTask base;
    FhInstallGeneration selected;
    unsigned busy;
} FhInstallEffectsTask;
/* One independent generation with original/performance collections (artifacts
 * 0/1), bound to the converter, source and both base topology recipes/identities.
 * Existing base manifests and expensive stages remain byte-compatible. */
int fh_install_effects_root(const char*,char out[FH_INSTALL_TX_PATH]);
int fh_install_effects_key(const FhInstallBundleSpec*,const FhInstallBundle*,unsigned char out[32]);
int fh_install_effects_open(const char*,const FhInstallBundleSpec*,const FhInstallBundle*,FhInstallGeneration*);
unsigned fh_install_effects_ready(void*);
int fh_install_effects_prepare(void*,FhStartupProgress,void*,char*,size_t);
#endif
