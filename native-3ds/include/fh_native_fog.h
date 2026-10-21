#ifndef FH_NATIVE_FOG_H
#define FH_NATIVE_FOG_H
#include "fh_graphics.h"
/* Borrow current GX fog state only for this synchronous native draw. */
const FhDrawState *fh_gx_native_fog_state(void);
void fh_native_fog_reset(void);
void fh_native_fog_apply(unsigned enable);
void fh_native_fog_apply_state(const FhDrawState *state);
void fh_native_fog_report(void);
#endif
