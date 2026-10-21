#ifndef FH_ACTOR_FOG_RESIDENT_H
#define FH_ACTOR_FOG_RESIDENT_H
#include "fh_native_actor_fog.h"
/* Embedded in a scene owner, zero initialized. Game-thread mutations only.
 * Published storage is immutable. Consumers may retain the asset until the
 * existing scene-storage join; replacing/releasing performs that join itself.
 * This does not own the caller's conversion scratch or source model. */
typedef struct {
    FhActorFogAsset asset;
    void *metadata,*geometry;
    unsigned source_bytes,source_crc,geometry_allocation_bytes;
} FhActorFogResident;
typedef struct {
    unsigned cpu_bytes,gpu_bytes,cpu_peak,gpu_peak,records;
    unsigned publications,releases,rejected,allocation_failures;
    uint64_t copied_bytes;
    uint64_t streamed_bytes; /* Bytes in successfully published direct loads. */
} FhActorFogResidentStats;
enum {FH_ACTOR_FOG_RESIDENT_CPU_LIMIT=1024*1024,
      FH_ACTOR_FOG_RESIDENT_GPU_LIMIT=2*1024*1024};
/* Load/publication only, never per draw. Old generation stays live on failure.
 * Budgets include BOTH generations while preparing a replacement. Returns a
 * direct immutable view; no source validation/copy occurs in find or draw. */
int fh_actor_fog_resident_publish(FhActorFogResident*,const void *metadata,
    unsigned metadata_bytes,const void *geometry,unsigned geometry_bytes,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned variant);
/* Read exactly bytes at a packet-relative offset. 1 means complete; 0 includes
 * short reads/errors. Destination is unpublished final normal/linear storage.
 * Callback must not retain it, reenter the renderer or publish GPU work. */
typedef int (*FhActorFogRead)(void *context,unsigned offset,void *out,unsigned bytes);
/* Load prepared [metadata][geometry] directly into final storage, checking the
 * complete payload CRC and existing semantic/source identity before flush and
 * publication. Same budgets/fences as publish; old resident survives failure.
 * No whole-packet staging allocation or extra resource copy. Load-time only. */
int fh_actor_fog_resident_load(FhActorFogResident*,unsigned metadata_bytes,
    unsigned geometry_bytes,unsigned packet_crc,FhActorFogRead,void *context,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned variant);
const FhActorFogAsset *fh_actor_fog_resident_find(const FhActorFogResident*,
    unsigned id,unsigned source_bytes,unsigned source_crc,unsigned variant);
void fh_actor_fog_resident_release(FhActorFogResident*);
/* Contiguous variants of one owner: one completed scene join covers all their
 * pending consumers. Empty banks do not wait. */
void fh_actor_fog_resident_release_many(FhActorFogResident*,unsigned count);
FhActorFogResidentStats fh_actor_fog_resident_stats(void);
#endif
