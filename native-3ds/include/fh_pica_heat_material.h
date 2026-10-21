#ifndef FH_PICA_HEAT_MATERIAL_H
#define FH_PICA_HEAT_MATERIAL_H
#include <citro3d.h>
/* Texture0 displaced colour, texture1 unshifted RGBA history depth. */
void fh_pica_heat_material_init(C3D_TexEnv stages[6]);
void fh_pica_heat_material_threshold(C3D_TexEnv stages[6],unsigned threshold);
#endif
