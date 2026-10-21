#ifndef FH_PICA_OPTIONS_H
#define FH_PICA_OPTIONS_H
#include <citro3d.h>
#include "fh_graphics_options.h"
/* Immutable neutral sampler; owners and original assets remain resident. */
C3D_Tex *fh_pica_options_texture(const C3D_Tex *texture,unsigned disabled_bit);
#endif
