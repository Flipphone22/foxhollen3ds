#ifndef FH_INSTALL_ACTOR_FOG_ISO_H
#define FH_INSTALL_ACTOR_FOG_ISO_H
#include "fh_install_actor_fog_collection.h"
#include "fh_install_disc.h"
typedef struct {
    unsigned models,world,empty,records,duplicates,topology_applied;
    unsigned texture_banks,texture_definitions,texture_workspace_peak;
} FhInstallActorFogCoverage;
typedef int (*FhInstallActorFogProgress)(void*,unsigned,unsigned);
/* Startup only. Rescan original ISO records; optional prepared topology is
 * reapplied, not recomputed. Compile both bump states into ONE stage artifact.
 * Cursor cannot advance beyond a model until both states have been appended.
 * 1=finished, -1=orderly cancellation/checkpoint, 0=error. Caller owns stage
 * recipe, source fingerprint, transaction close and generation publication. */
int fh_install_actor_fog_iso(FhInstallTransaction*,FhInstallDisc*,unsigned artifact,
    const char*topology,FhInstallActorFogCoverage*,FhInstallActorFogProgress,void*);
#endif
