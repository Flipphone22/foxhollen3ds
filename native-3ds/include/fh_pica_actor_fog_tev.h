#ifndef FH_PICA_ACTOR_FOG_TEV_H
#define FH_PICA_ACTOR_FOG_TEV_H
#include <citro3d.h>
/* Load-time variant compiler. Base material owns stages 0..2; only a true
 * passthrough stage 3 can become height fog. Actor colour fade 4 and distance
 * fog 5 retain their original ordering. Output and base can alias. */
int fh_pica_actor_fog_tev(C3D_TexEnv out[4],const C3D_TexEnv base[4]);
#endif
