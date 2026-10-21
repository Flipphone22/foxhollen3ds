#ifndef FH_PICA_BLUR_MATERIAL_H
#define FH_PICA_BLUR_MATERIAL_H
#include <citro3d.h>
/* Fixed native two-pass decomposition of doBlurFilter. This is not a GX
 * interpreter. Pass zero samples [up,left,right]; pass one binds [partial,
 * centre,borrowed-depth]. UVs/target ownership belong to the caller. */
typedef struct { C3D_TexEnv stages[4][6]; } FhPicaBlurMaterial;
/* depth_alpha selects packed history A; zero selects borrowed D24S8 G. */
void fh_pica_blur_material_init(FhPicaBlurMaterial *material,unsigned depth_alpha);
unsigned fh_pica_blur_material_mode(unsigned one_sided,unsigned strong);
void fh_pica_blur_material_threshold(FhPicaBlurMaterial *material,unsigned threshold);
/* Final pass requires C3D_TexEnvBufUpdate(C3D_Alpha,1): stage-zero positive
 * difference reaches the buffer by stage two. First pass must clear updates. */
#endif
