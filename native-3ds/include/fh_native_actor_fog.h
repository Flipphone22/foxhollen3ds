#ifndef FH_NATIVE_ACTOR_FOG_H
#define FH_NATIVE_ACTOR_FOG_H
#include "fh_pica_actor_fog.h"
#include "fh_pica_actor_fog_plan.h"

/* FHAFG002: bounded CPU metadata plus independently owned GPU storage. Opening
 * is load-time validation only. It neither admits live material alternatives nor
 * grants write/retirement ownership; the scene owner must retain both regions
 * until the GPU has joined. No interpreter or validation in the draw accessor. */
enum {FH_ACTOR_FOG_META_CAP=256*1024,FH_ACTOR_FOG_GPU_CAP=2*1024*1024,
      FH_ACTOR_FOG_HEADER_BYTES=96,FH_ACTOR_FOG_GROUP_BYTES=240,
      FH_ACTOR_FOG_SECTION_BYTES=32};
typedef struct {
    const unsigned char *meta,*gpu;
    unsigned metadata_bytes,gpu_bytes,id,variant,groups,sections,sections_at;
    unsigned pose_plans,plans_at,source_cache_eligible; /* count-based source admission, not live cache state */
} FhActorFogAsset;
typedef struct {
    unsigned vertex_offset,fog_offset,index_offset,vertices,indices,bones,fog_matrices;
    unsigned first_section,sections,format,flags,material,two_uv;
    const uint16_t *bone;
    const FhPicaActorFogMatrix *fog;
    const FhPicaActorFogRef *refs;
    const uint16_t *pose_plan;
} FhActorFogDraw;
int fh_native_actor_fog_open(FhActorFogAsset*,const void *metadata,unsigned metadata_bytes,
    const void *gpu,unsigned gpu_bytes,unsigned id,unsigned source_bytes,
    unsigned source_crc,unsigned variant);
int fh_native_actor_fog_draw(const FhActorFogAsset*,unsigned group,FhActorFogDraw*);
/* Direct immutable recipe access after open; no source decoding or lookup. */
const FhPicaActorFogPosePlan *fh_native_actor_fog_pose_plan(const FhActorFogAsset*,unsigned index);
#endif
