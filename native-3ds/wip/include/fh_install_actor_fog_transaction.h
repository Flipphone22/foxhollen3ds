#ifndef FH_INSTALL_ACTOR_FOG_TRANSACTION_H
#define FH_INSTALL_ACTOR_FOG_TRANSACTION_H
#include "fh_install_transaction.h"
#include "fh_install_actor_fog_bundle.h"
/* Writes one bundle as a recipe-owned artifact, directly into the existing
 * transaction. Caller supplies a recipe/source identity for these exact inputs.
 * Requires a fresh/zero checkpoint or a complete pending file whose finish
 * was interrupted; never rewrites checkpointed bytes or changes live files.
 * On interrupted work, transaction_open truncates back to that checkpoint.
 * Completes/validates the artifact; only the outer owner publishes a generation.
 * Not a complete collection converter or a gameplay-time preparation hook. */
int fh_install_actor_fog_artifact(FhInstallTransaction*,unsigned artifact,
    const void*source,unsigned source_bytes,unsigned id,unsigned bump_enabled,
    FhActorFogTextureWidth,void*context);
#endif
