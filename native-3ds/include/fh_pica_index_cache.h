#ifndef FH_PICA_INDEX_CACHE_H
#define FH_PICA_INDEX_CACHE_H
#include <stdint.h>
/* Admission-time triangle reordering for explicitly order-independent passes.
 * No vertices, winding or triangle multiplicities change. This is NOT a
 * general material optimizer: blending/equal-depth colour can depend on order.
 * Scratch, input and output must be disjoint, aligned and caller-owned.
 * No allocation, SDK calls or retained pointers. Cache size is a model only. */
unsigned fh_pica_index_cache_scratch(unsigned vertices,unsigned indices);
int fh_pica_index_cache_order(uint16_t *out,const uint16_t *in,unsigned vertices,
    unsigned indices,unsigned cache_size,void *scratch,unsigned scratch_bytes);
#endif
