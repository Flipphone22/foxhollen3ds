#ifndef FH_INSTALL_ACTOR_FOG_GROUPS_H
#define FH_INSTALL_ACTOR_FOG_GROUPS_H
#include "fh_install_actor_fog.h"
#include "fh_install_groups.h"

/* Startup/load compiler only. Both matrix families share the shader's existing
 * 22-entry palette. Visible rebased poses occupy the first bone_count slots;
 * independently selected source fog matrices follow them. No frame interpreter. */
typedef struct {
    unsigned vertices,indices,bone_count,fog_count,sections,vat,flags,shader,two_uv;
    FhInstallActorFogRef ref[FH_MODEL_VERTICES];
    uint16_t index[FH_MODEL_INDICES],bones[FH_MODEL_BONES];
    FhPicaActorFogMatrix fog[FH_MODEL_BONES];
    FhInstallSection section[FH_INSTALL_GROUP_SECTIONS];
} FhInstallActorFogGroup;
typedef int (*FhInstallActorFogGroupEmit)(void*,const FhInstallActorFogGroup*);
/* Resolve the actual material's fog coordinate for this precompiled light/pass
 * alternative. Descriptor state is independently retained by the loader. */
typedef int (*FhInstallActorFogCoordinate)(void*,const FhInstallDrawSource*,unsigned*);
extern const unsigned fh_install_actor_fog_group_scratch_bytes;
int fh_install_actor_fog_groups(const FhInstallVertexSource*,
    const FhInstallSkinWeights*,const FhInstallSkinWeights*,
    const FhPicaActorFogMatrixBank *initial,unsigned skip_texture_loads,
    FhInstallActorFogRoute,FhInstallActorFogCoordinate,
    FhInstallActorFogGroupEmit,void*,FhInstallGroupResult*);
/* Produce primary 40B and companion 16B immutable GPU streams at load time.
 * Companion rows address the group's source-matrix suffix; their positions
 * are original VAT-scaled positions, without visible bind-tail adjustment.
 * Complete preflight precedes publication. Buffers must be disjoint. */
int fh_install_actor_fog_pack(const FhInstallVertexSource*,
    const FhInstallSkinWeights*,const FhInstallSkinWeights*,
    const FhInstallActorFogGroup*,FhModelCompactVertex*,FhPicaActorFogGpuVertex*,
    unsigned capacity);
#endif
