#ifndef FH_PAD_DPAD_CSTICK_H
#define FH_PAD_DPAD_CSTICK_H
#include "fh_platform.h"
/* Old 3DS has no built-in C-stick. Map physical D-pad directions to the raw
 * GameCube substick, then let the existing PADClamp/game repeat logic run.
 * Touch-held passthrough retains original D-pad access (world-map zoom/debug).
 * Other buttons and the movement stick are never consumed or synthesized. */
static inline void fh_pad_dpad_cstick(FhPadState *pad,int passthrough){
    if(passthrough)return;
    unsigned directions=pad->buttons&15u;
    pad->cx=127*((int)((directions&2u)!=0)-(int)((directions&1u)!=0));
    pad->cy=127*((int)((directions&8u)!=0)-(int)((directions&4u)!=0));
    pad->buttons&=~15u;
}
#endif
