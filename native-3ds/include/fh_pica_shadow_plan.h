#ifndef FH_PICA_SHADOW_PLAN_H
#define FH_PICA_SHADOW_PLAN_H
#include "fh_pica_model.h"
/* Optional immutable position/skin stream for constant-white shadow masks,
 * compiled at asset admission. Triangle order may differ; never use this plan
 * for ordinary materials or fogged/blended planar draws. No
 * material/UV/normal stream, no frame-time gathering or animation changes. */
typedef struct {int16_t position[4];uint8_t skin[4];float offset[3];} FhPicaShadowVertex;
_Static_assert(sizeof(FhPicaShadowVertex)==24,"Shadow vertex ABI");
typedef struct {
    unsigned first_group,groups,vertex_offset,index_offset,vertices,indices,format,bones;
    unsigned palette_changed; /* Relative to preceding draw of this immutable plan. */
    uint16_t bone[FH_MODEL_BONES];
} FhPicaShadowDraw;
typedef struct {unsigned count,bytes;} FhPicaShadowSize;
typedef struct {unsigned count,bytes;void *gpu;FhPicaShadowDraw draw[];} FhPicaShadowPlan;
typedef struct FhPicaShadowPrepared FhPicaShadowPrepared;
/* Admission-only two-phase compiler. Prepare validates and deduplicates once;
 * emit reuses that mapping and a bounded index workspace, without allocation or
 * hashing. Emit is exclusive because its temporary workspace is reused. Source asset and model
 * plan must stay immutable and alive until close. Output must not overlap them.
 * Close on every admission exit, before publishing or retiring source storage. */
FhPicaShadowPrepared *fh_pica_shadow_prepare(const FhSceneAsset*,const FhPicaModelPlan*,FhPicaShadowSize*);
int fh_pica_shadow_emit(FhPicaShadowPrepared*,FhPicaShadowDraw*,unsigned,void*,unsigned);
void fh_pica_shadow_prepared_close(FhPicaShadowPrepared*);
/* NULL outputs measure. Validate all ranges before publishing; callers discard
 * unpublished output on any failure. Source and output must not overlap. */
int fh_pica_shadow_compile(const FhSceneAsset*,const FhPicaModelPlan*,FhPicaShadowSize*,FhPicaShadowDraw*,unsigned,void*,unsigned);
/* Temporary NORMAL memory required by compilation, including its draw layout.
 * Caller checks the existing normal-memory reserve before optional preparation.
 * Zero means invalid source. No allocations or GPU operations in this query. */
unsigned fh_pica_shadow_scratch(const FhSceneAsset*,const FhPicaModelPlan*);
const FhPicaShadowPlan *fh_native_scene_shadow_plan(const FhSceneAsset*);
#endif
