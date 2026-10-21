#ifndef FH_INSTALL_ACTOR_FOG_BUNDLE_H
#define FH_INSTALL_ACTOR_FOG_BUNDLE_H
#include "fh_actor_fog_bundle.h"
#include "fh_install_actor_fog_profile.h"
/* Startup transaction sink. append receives a borrowed, bounded packet; patch
 * finalizes the header at offset zero only after every payload was accepted.
 * The sink owns staging/rollback/publication. Failure must discard its partial
 * output. No checkpoint may publish the unfinished header. */
typedef struct {
    int (*append)(void*,const void*,unsigned);
    int (*header)(void*,const void*,unsigned);
    void *context;
} FhInstallActorFogSink;
int fh_install_actor_fog_bundle_write(const FhInstallActorFogSink*,
    const void*source,unsigned source_bytes,unsigned id,unsigned bump_enabled,
    FhActorFogTextureWidth,void*context);
/* Writes an EMPTY caller-owned temporary file, one compiled variant at a time.
 * No game bytes shipped: source and texture dimensions come from the user's
 * assets. Header publishes last. Caller must close/validate/transactionally
 * rename after success, and discard the temporary file on any failure.
 * Does not create directories, modify a live pack or run during drawing. */
int fh_install_actor_fog_bundle(FILE*,const void*source,unsigned source_bytes,
    unsigned id,unsigned bump_enabled,FhActorFogTextureWidth,void*context);
#endif
