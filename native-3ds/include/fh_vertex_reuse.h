#ifndef FH_VERTEX_REUSE_H
#define FH_VERTEX_REUSE_H
#include <stdint.h>
/* Identities describe byte-identical decoded vertices within one immutable
 * submission. Zero is uncacheable. No game/source pointers reach the GPU. */
typedef struct { uint32_t id[16];uint16_t index[16]; } FhVertexReuse;
static inline unsigned fh_vertex_reuse(FhVertexReuse *cache,unsigned id,unsigned fresh) {
    if(!id)return fresh;
    unsigned slot=id&15;
    if(cache->id[slot]==id)return cache->index[slot];
    cache->id[slot]=id;cache->index[slot]=fresh;return fresh;
}
#endif
