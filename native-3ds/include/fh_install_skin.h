#ifndef FH_INSTALL_SKIN_H
#define FH_INSTALL_SKIN_H
#include "fh_install_vertex.h"
/* Canonical original CPU-stream coefficients. Missing and an explicitly empty
 * blend are distinct. Weights are NOT normalized: same-joint sums can be510. */
typedef struct {uint16_t joint[2],weight[2];uint8_t present,count;} FhInstallSkinWeights;
_Static_assert(sizeof(FhInstallSkinWeights)==10,"Startup sparse skin map stride");
/* Caller provides one entry per original position/normal. No allocations.
 * On malformed input the required output range is cleared; do not publish it.
 * The compact PICA compiler must separately validate its supported weight sums
 * and independent normal binding before emitting a resident packet. */
int fh_install_skin_stream(const FhInstallVertexSource*,unsigned normal,
                           FhInstallSkinWeights*,unsigned capacity);
#endif
