#ifndef FH_PICA_RIPPLE_REDUCE_H
#define FH_PICA_RIPPLE_REDUCE_H
#include "fh_pica_water_grid.h"
#ifdef __cplusplus
extern "C" {
#endif
enum {FH_RIPPLE_REDUCE_SCRATCH=1024*1024};
/* Load-only candidate reduction. Caller owns the compiler's mutable temporary
 * buffers, before upload/publication. Never call on GPU-visible/live data.
 * Uses the pinned bounded startup kernel; not reentrant with conversion.
 * Failure leaves all inputs/result unchanged. Zero return is NOT permission
 * to silently drop a ripple; the future owner must report admission failure.
 * The .002 attribute error is a candidate setting, not a fidelity guarantee.
 * Not yet linked/admitted by the live water owner. */
int fh_pica_ripple_reduce(FhWaterGridVertex *vertices,uint16_t *indices,
    FhWaterGridSize *size,size_t *scratch_peak);
#ifdef __cplusplus
}
#endif
#endif
