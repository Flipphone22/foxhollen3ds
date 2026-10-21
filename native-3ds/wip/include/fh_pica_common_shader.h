#ifndef FH_PICA_COMMON_SHADER_H
#define FH_PICA_COMMON_SHADER_H
#include <3ds.h>
enum {FH_COMMON_ACTOR,FH_COMMON_GLOW,FH_COMMON_GLOW_GEOMETRY,FH_COMMON_GLOW_DYNAMIC,
      FH_COMMON_POINTS,FH_COMMON_GEOMETRY,FH_COMMON_HUD,FH_COMMON_UI,FH_COMMON_HEIGHT,
      FH_COMMON_CLEAR,FH_COMMON_PLANE,FH_COMMON_COUNT};
/* Each caller owns its shader instance. Shared code/tables survive until all
 * consumers release after the existing scene GPU retirement fence. */
DVLE_s *fh_pica_common_shader_acquire(unsigned entry);
void fh_pica_common_shader_release(unsigned entry);
unsigned fh_pica_common_shader_instructions(void);
unsigned fh_pica_common_shader_bytes(void);
#endif
