#ifndef FH_ACTOR_FOG_COLLECTION_H
#define FH_ACTOR_FOG_COLLECTION_H
#include "fh_actor_fog_bundle.h"
enum {FH_ACTOR_FOG_COLLECTION_RECORDS=4096,FH_ACTOR_FOG_COLLECTION_LIMIT=512*1024*1024,
      FH_ACTOR_FOG_COLLECTION_BLOCK=16384,
      FH_ACTOR_FOG_COLLECTION_SCRATCH=2*FH_ACTOR_FOG_COLLECTION_BLOCK+
          ((FH_ACTOR_FOG_BUNDLE_LIMIT+FH_ACTOR_FOG_COLLECTION_BLOCK-1)/FH_ACTOR_FOG_COLLECTION_BLOCK)*8,
      FH_ACTOR_FOG_COLLECTION_FOOTER=FH_ACTOR_FOG_BUNDLE_HEADER+64};
typedef struct {
    unsigned id,source_bytes,source_crc,bump,start,end;
    unsigned char input_sha256[32]; /* raw source + resolved material facts */
} FhActorFogCollectionEntry;
typedef struct {
    FILE*file;FhActorFogCollectionEntry*entries;unsigned count,capacity,bytes;
} FhActorFogCollection;
/* Zero initialize. One-time startup/load indexing; file is borrowed. Failure leaves out unchanged.
 * Validates all linked footers, identities and duplicate keys before publication.
 * Payload CRCs are validated on actual resident publication, or by verify().
 * No file scan, allocation or source interpretation belongs in a draw call. */
int fh_actor_fog_collection_open(FhActorFogCollection*,FILE*);
void fh_actor_fog_collection_close(FhActorFogCollection*);
const FhActorFogCollectionEntry*fh_actor_fog_collection_find(const FhActorFogCollection*,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned bump);
int fh_actor_fog_collection_bundle(FhActorFogBundle*,const FhActorFogCollection*,
    const FhActorFogCollectionEntry*);
typedef struct {FILE*file;FhActorFogBundleEntry entry;unsigned start,end;} FhActorFogCollectionRead;
int fh_actor_fog_collection_read(void*,unsigned offset,void*out,unsigned bytes);
int fh_actor_fog_collection_verify(const FhActorFogCollection*);
#endif
