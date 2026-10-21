#ifndef FH_NATIVE_SKY_H
#define FH_NATIVE_SKY_H
#include "fh_graphics.h"
/* The untextured star tier keeps all original RNG calls and point order.
 * Only its repeated material/draw submission is replaced. */
int fh_gx_sky_begin(const void *const *lists,const uint16_t *sizes,unsigned count);
void fh_renderer_sky_color(unsigned group,unsigned r,unsigned g,unsigned b,unsigned a);
int fh_renderer_sky_begin(const void *const *lists,const uint16_t *sizes,unsigned count,
    const FhDrawState *state,const float view[3][4]);
#endif
