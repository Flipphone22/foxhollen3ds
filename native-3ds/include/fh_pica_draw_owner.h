#ifndef FH_PICA_DRAW_OWNER_H
#define FH_PICA_DRAW_OWNER_H
#include "fh_pica_draw_stream.h"
/* Implemented inside the isolated pinned SDK owner, not the application loop.
 * Caller reserves capacity and selects a validated ordinary world shader,
 * target, camera, scissor and lighting/fog owners before begin. No SDK setters,
 * raw register writes or resource mutation may occur inside a batch. End first
 * to change camera, material family, target or shader; then begin again.
 * SDK context updates/splits/restoration also end the batch defensively.
 * A zero draw result is not permission to discard geometry: close the batch,
 * reserve/flush using the existing queue owner, then republish and retry.
 * Referenced geometry/images/samplers remain resident through the GPU fence. */
int fh_pica_stream_begin(FhPicaStreamState *);
int fh_pica_stream_environment(FhPicaDrawLive *);
unsigned fh_pica_stream_draw(const FhPicaResidentDraw *,const FhPicaDrawLive *,
                            unsigned flush_textures);
void fh_pica_stream_end(void);
#endif
