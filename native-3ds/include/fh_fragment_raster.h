#ifndef FH_FRAGMENT_RASTER_H
#define FH_FRAGMENT_RASTER_H
#include "fh_fragment.h"
typedef struct {
    const FhVertex *input;
    const FhDrawState *state;
    const FhFragmentProgram *program; /* Borrowed through synchronous completion. */
    const float (*matrix)[2][3];
    const float (*screen)[2];
    float area;
    unsigned left,top,width,height,mode;
    uint32_t *pixels;
    unsigned evaluated;
} FhFragmentRaster;
void fh_fragment_raster_rows(FhFragmentRaster *job,unsigned first,unsigned end);
/* Synchronous ownership: input/source lifetimes extend until this returns.
 * Only ordinary CPU memory is touched by the worker. GPU flush/submission is
 * performed by the producer after copying the completed pixels to linear RAM. */
void fh_fragment_worker_start(void);
void fh_fragment_worker_run(FhFragmentRaster *job,void (*service)(void));
void fh_fragment_worker_stop(void);
void fh_fragment_worker_stats(unsigned *active,unsigned *core,unsigned *jobs,unsigned *peak);
#endif
