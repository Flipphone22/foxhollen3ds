#ifndef FH_PICA_SPECULAR_H
#define FH_PICA_SPECULAR_H
#include "fh_graphics.h"
/* Snapshot of original mode-1 RCP slots, before its group is reset. Small
 * uniforms only; neither geometry nor source GX commands cross this boundary. */
typedef struct {
    float direction[2][4],half_direction[2][4];
    float attenuation[4],material[4],diffuse_alpha[2];
} FhPicaSpecularState;
void fh_pica_specular_prepare(const FhTexture *base);
/* Resident layered-material copy storage; source-boundary prefetch plus miss repair.
 * Allocates a bounded GPU copy target set, never converts pixels on CPU. */
void fh_pica_specular_prepare_half(void);
void fh_pica_specular_update(unsigned mask,const FhPicaSpecularState state[3]);
/* SPECULAR_PREFETCH_V1: call before actor uniforms, never during late binding. */
void fh_pica_specular_require_half(unsigned mask);
void fh_pica_specular_shutdown(void);
#include "fh_native_scene.h"
int fh_pica_material_specular_compile(const FhSceneAsset *asset,unsigned index,unsigned selector,unsigned alpha,unsigned base_modulate);
unsigned fh_pica_scene_specular(unsigned enabled);
#ifdef __3DS__
#include <citro3d.h>
const C3D_Tex *fh_pica_specular_sampler(unsigned slot);
const C3D_Tex *fh_pica_specular_half_sampler(unsigned slot);
#endif
#endif
