#ifndef FH_PICA_DISTORTION_H
#define FH_PICA_DISTORTION_H
#include <stddef.h>
#include "fh_pica_distortion_plan.h"
unsigned fh_pica_distortion_enabled(void);
/* Field/radial payloads belong to the game; the platform may compile them at
 * first admitted load, but must not retain source pointers after returning.
 * Reuse immutable native resources thereafter. Source capture admission and
 * platform implementation must be installed together with the game adapter. */
void fh_pica_distortion_render(const FhPicaDistortionPlan *plan,
    const float viewport[4],const unsigned char *field,size_t field_bytes,
    const unsigned char *radial,size_t radial_bytes);
#endif
