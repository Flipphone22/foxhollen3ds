#ifndef FH_PICA_DISTORTION_DRAW_H
#define FH_PICA_DISTORTION_DRAW_H
#include "fh_pica_distortion_geometry.h"
#include "fh_pica_distortion_uniforms.h"
/* Source updateReflectionTextures downsamples 640x480 to 320x240.
 * Native capture storage dimensions must not change indirect texel strength. */
enum { FH_DISTORTION_SOURCE_WIDTH=320,FH_DISTORTION_SOURCE_HEIGHT=240 };
/* Caller owns an active target, enabled/admission checks, completed capture
 * publication, shader/resource lifetimes and auxiliary state invalidation.
 * Textures MUST be detached snapshots, never the active target. Initialize the
 * six material stages once with fh_pica_distortion_material_prepare at load.
 * This path retains the known alpha precision limitation; not shipping yet. */
unsigned fh_pica_distortion_draw(FhPicaDistortionGeometry *geometry,
    shaderProgram_s *program,C3D_TexEnv material[6],
    const FhPicaDistortionPlan *plan,const float viewport[4],
    C3D_Tex *color,C3D_Tex *depth,C3D_Tex *radial);
#endif
