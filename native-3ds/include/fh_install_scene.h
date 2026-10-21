#ifndef FH_INSTALL_SCENE_H
#define FH_INSTALL_SCENE_H
#include "fh_install_groups.h"
typedef struct {
    void *allocation;unsigned char *data;unsigned bytes,metadata_bytes;
    FhInstallGroupResult groups;char error[112];
} FhInstallScene;
/* Zero-initialize. Compile one original or fidelity-checked reduced source.
 * No source mutation, decimation or filesystem I/O occurs here. The caller
 * owns recipe/source provenance and publishes only a complete generation.
 * A non-rendering source can succeed with bytes=0. Other omissions fail. */
int fh_install_scene(FhInstallScene*,const void*,unsigned bytes,unsigned id);
void fh_install_scene_close(FhInstallScene*);
#endif
