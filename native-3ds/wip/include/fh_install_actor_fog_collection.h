#ifndef FH_INSTALL_ACTOR_FOG_COLLECTION_H
#define FH_INSTALL_ACTOR_FOG_COLLECTION_H
#include "fh_actor_fog_collection.h"
#include "fh_install_actor_fog_bundle.h"
#include "fh_install_cursor.h"
typedef struct {
    FhInstallTransaction*transaction;
    FhActorFogCollection collection;
    FhInstallCursor cursor;
    unsigned duplicates;
    /* Startup-only hash -> entry index+1. Entries append without reordering;
     * the borrowed collection is private until close/reopen through the normal
     * runtime reader, which restores sorted lookup. Never serialize this index. */
    uint16_t *lookup;
} FhInstallActorFogCollection;
/* Startup-only canonical source/material identity. Original image-width
 * resolution is part of the key even when the model's raw bytes are identical.
 * This does not admit/compile a geometry packet or validate live GPU material. */
int fh_install_actor_fog_input_digest(unsigned char out[32],const void*raw,
    unsigned bytes,unsigned bump,FhActorFogTextureWidth,void*context);
/* One recipe-owned artifact contains the collection. Appends payloads then a
 * fixed footer; never patches or recopies earlier checkpointed records.
 * raw is exact original or post-reduction source. Recipe owns that distinction.
 * Input SHA covers source and resolved material facts (including original image
 * widths), disambiguating duplicate runtime CRC keys before accepting reuse.
 * Zero initialize; caller owns transaction lifetime and generation publication. */
int fh_install_actor_fog_collection_begin(FhInstallActorFogCollection*,FhInstallTransaction*,unsigned artifact);
int fh_install_actor_fog_collection_add(FhInstallActorFogCollection*,
    const void*raw,unsigned bytes,unsigned id,unsigned bump,
    FhActorFogTextureWidth,void*context,unsigned next_file,unsigned next_index);
int fh_install_actor_fog_collection_finish(FhInstallActorFogCollection*);
void fh_install_actor_fog_collection_close(FhInstallActorFogCollection*);
#endif
