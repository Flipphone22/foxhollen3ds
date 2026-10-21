#ifndef FH_INSTALL_GROUPS_H
#define FH_INSTALL_GROUPS_H
#include "fh_install_compact.h"
enum {FH_INSTALL_GROUP_SECTIONS=256};
typedef struct {
    unsigned list,shader,stream,bit,first,count,ordinal;
} FhInstallSection;
typedef struct {
    unsigned vertices,indices,bone_count,sections,vat,flags,shader,stream,two_uv;
    FhModelRef ref[FH_MODEL_VERTICES];
    uint16_t index[FH_WORLD_INDICES],bones[FH_MODEL_BONES];
    FhInstallSection section[FH_INSTALL_GROUP_SECTIONS];
} FhInstallGroup;
typedef struct {unsigned source_draws,draws,groups,vertices,indices;char error[112];} FhInstallGroupResult;
/* Callback borrows one completed group until return. Installation-only; no GPU
 * work. 1 success,0 invalid/unsupported,-1 consumer cancellation. On failure
 * discard partial emitted output. No missing draw fallback is published. */
typedef int (*FhInstallGroupEmit)(void*,const FhInstallGroup*);
int fh_install_groups(const FhInstallVertexSource*,const FhInstallSkinWeights*,
    const FhInstallSkinWeights*,FhInstallGroupEmit,void*,FhInstallGroupResult*);
#endif
