#ifndef FH_PICA_SHADOW_RECEIVERS_H
#define FH_PICA_SHADOW_RECEIVERS_H
#include "fh_pica_scene.h"
/* Load-prepared bounds over the actual resident visible-world triangles.
 * No second geometry copy. The scene owner owns both this plan and its borrowed
 * draw/vertex/index pointers; rebuild bounds only on world-position mutation.
 * A release must remove frame references before freeing the owning scene. */
typedef struct {const FhPicaWorldDraw *draw;FhWorldBounds bounds;} FhPicaShadowReceiver;
/* NULL output measures/validates; malformed input never partially publishes. */
unsigned fh_pica_shadow_receivers_compile(const FhSceneAsset*,const FhPicaWorldPlan*,FhPicaShadowReceiver*,unsigned);
/* Build four conservative receiver-local planes for UV inside [0,1]^2.
 * Matrices map decoded receiver-local coordinates, matching the stored bounds.
 * Raw vertex attribute scaling is applied separately to the GPU uniforms. */
void fh_pica_shadow_receiver_planes(FhWorldPlane out[4],const float uv[2][4]);
/* Load-only specialization: combine up to four compatible material groups into
 * position-only batches. Every source triangle and position is retained. */
unsigned fh_pica_shadow_batches_compile(const FhSceneAsset*,const FhPicaWorldPlan*,FhPicaWorldPlan*,void*,unsigned,unsigned*);
#endif
