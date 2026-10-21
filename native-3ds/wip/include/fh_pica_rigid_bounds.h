#ifndef FH_PICA_RIGID_BOUNDS_H
#define FH_PICA_RIGID_BOUNDS_H
#include "fh_native_model.h"
#include "fh_world_bounds.h"
/* Prepared from an owned vertex stream at load. slot_plus_one=0 keeps draws
 * visible. Never reuse after position/backing mutation or for morph/fur output.
 * This stores local positions, not CPU-transformed per-frame geometry. */
typedef struct {FhWorldBounds box;unsigned slot_plus_one;} FhPicaRigidBound;
void fh_pica_rigid_bound_compile(FhPicaRigidBound*,const void*,unsigned,unsigned,unsigned);
/* The caller supplies this rigid slot's live world-space bone/tail transform.
 * Uses homogeneous camera side planes; no near/far or distance-based dropping. */
int fh_pica_rigid_bound_outside(const FhPicaRigidBound*,const float[3][4],const float[4][4]);
#endif
