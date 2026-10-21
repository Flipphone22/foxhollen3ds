#pragma once
#include <citro3d.h>
enum { FH_MATERIAL_SHADER_ACTOR, FH_MATERIAL_SHADER_DIFFUSE, FH_MATERIAL_SHADER_HEIGHT_FOG, FH_MATERIAL_SHADER_COUNT };
/* Owners acquire once at preparation and release after their GPU completion
 * fence. Both entries share one DVLP allocation: Citro3D can retain code when
 * switching entries. This does not relax uniform/state publication. */
shaderProgram_s *fh_pica_material_shader_acquire(unsigned entry);
void fh_pica_material_shader_release(unsigned entry);
unsigned fh_pica_material_shader_instructions(void);
