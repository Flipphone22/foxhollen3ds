#ifndef FH_ACTOR_FOG_BUNDLE_H
#define FH_ACTOR_FOG_BUNDLE_H
#include <stdio.h>
#include "fh_pica_actor_fog_variants.h"
enum {FH_ACTOR_FOG_BUNDLE_HEADER=1504,FH_ACTOR_FOG_BUNDLE_LIMIT=128*1024*1024};
_Static_assert(sizeof(FhPicaActorFogVariants)==164,"Actor fog file plan layout");
typedef struct {unsigned offset,metadata,geometry,crc;} FhActorFogBundleEntry;
typedef struct {
    FhPicaActorFogVariants plan;
    FhActorFogBundleEntry entries[FH_ACTOR_FOG_VARIANT_CELLS];
    unsigned bytes;
} FhActorFogBundle;
/* Exact source identity and original bump-routing setting are required.
 * Caller owns FILE. Reads bounded header only; payload integrity is checked by
 * resident_load before GPU publication. Failure leaves output unchanged. */
int fh_actor_fog_bundle_open(FhActorFogBundle*,FILE*,unsigned id,
    unsigned source_bytes,unsigned source_crc,unsigned bump_enabled);
/* Shared immutable header decoder for a standalone file or collection record.
 * length is the logical bundle length including its header. */
int fh_actor_fog_bundle_decode(FhActorFogBundle*,const void*header,unsigned length,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned bump_enabled);
typedef struct {FILE*file;FhActorFogBundleEntry entry;} FhActorFogBundleRead;
/* Resident reader callback: packet-relative offsets, no staging allocation. */
int fh_actor_fog_bundle_read(void*,unsigned offset,void*out,unsigned bytes);
#endif
