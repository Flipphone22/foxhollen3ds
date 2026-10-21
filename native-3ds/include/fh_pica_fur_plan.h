#ifndef FH_PICA_FUR_PLAN_H
#define FH_PICA_FUR_PLAN_H
#include "fh_pica_model.h"
#include "fh_pica_fur_groom.h"
/* CPU-only immutable parameters, owned by the parent scene record. They are
 * copied into SDK uniform storage; the GPU never reads these allocations. */
typedef struct {float pivot_tail[3],log_growth[2];} FhPicaFurShape;
typedef struct {unsigned count,wrap_changed;FhPicaFurGroomPlan*groom;FhPicaFurShape shape[];} FhPicaFurPlan;
const FhPicaFurPlan *fh_native_scene_fur_plan(const FhSceneAsset*);
/* Zero means retain the existing native shell path. No draw is omitted.
 * NULL output validates/measures; failure does not publish partial output. */
unsigned fh_pica_fur_compile(const FhSceneAsset*,const FhPicaModelPlan*,
    const void *source,unsigned bytes,FhPicaFurShape*,unsigned capacity);
#endif
