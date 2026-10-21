#ifndef FH_PICA_IMAGE_H
#define FH_PICA_IMAGE_H
#include <citro3d.h>
#include "fh_graphics.h"
#include "fh_pica_scene.h"
/* Native resident image pool shared by material plans and native HUD pages.
 * Acquire may allocate/convert/read; never call it from a draw. */
unsigned fh_pica_image_acquire(const FhTexture*);
/* Optional reflected-particle mask; load boundary only. 0 means unsupported/budget failure. */
unsigned fh_pica_image_acquire_glow_mask(const FhTexture*);
void fh_pica_image_release(unsigned);
void fh_pica_image_sampler(unsigned,const FhTexture*,C3D_Tex*,float scale[2]);
const C3D_Tex *fh_pica_hud_glow_mask_sampler(unsigned page);
/* Borrow resident samplers only; caller must retain the owning asset/page. */
const C3D_Tex *fh_pica_material_projected_sampler(const FhPicaMaterialSet*,unsigned,float scale[2]);
const C3D_Tex *fh_pica_hud_sampler(unsigned,float scale[2],unsigned *intensity);
const C3D_Tex *fh_pica_hud_sampler_layer(unsigned,unsigned layer,float scale[2],unsigned *intensity);
#endif
