#ifndef FH_DRAW_CAPTURE_H
#define FH_DRAW_CAPTURE_H
#include "fh_graphics.h"
#ifdef FH_DRAW_CAPTURE
void fh_capture_draw_init(void);
int fh_capture_draw_pending(unsigned frame,unsigned count,unsigned mode);
void fh_capture_draw(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state,unsigned mode);
void fh_capture_draw_timed(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state,unsigned mode,uint64_t ticks);
void fh_capture_failed_draw(unsigned frame,const FhVertex *vertices,unsigned count,const FhDrawState *state);
#endif
#endif
