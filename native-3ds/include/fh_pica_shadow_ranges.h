#ifndef FH_PICA_SHADOW_RANGES_H
#define FH_PICA_SHADOW_RANGES_H
#include "fh_pica_shadow_points.h"
/* Immutable authored bounds only: moving points retain these source filters.
 * Balanced tree over existing route order. Query returns ONE contiguous span;
 * it never changes triangle order or adds draws/vertex copies. GPU retains the
 * exact per-triangle authored, live-position and face tests. */
enum {FH_SHADOW_RANGE_LEAF=16};
typedef struct {int16_t lo[3],hi[3];uint16_t first,count;} FhShadowRangeNode;
typedef struct {unsigned first,count,visited;} FhShadowRange;
unsigned fh_pica_shadow_range_nodes(unsigned count);
int fh_pica_shadow_range_compile(const FhShadowPoint*,unsigned,const uint16_t*,unsigned,
    FhShadowRangeNode*,unsigned);
/* Caller provides a validated immutable tree, and block-local UNWRAPPED query.
 * Returned first/count address route elements (indices for an indexed route).
 * Normal-memory tree stays immutable across producer position updates. */
FhShadowRange fh_pica_shadow_range(const FhShadowRangeNode*,const float[3],const float[3],int16_t);
enum {FH_SHADOW_RANGE_CACHE_SLOTS=8};
typedef struct {
    uint32_t key[6];const FhShadowRangeNode *tree;uint32_t hash;
    uint16_t first,count;int16_t y;
} FhShadowRangeCacheEntry;
typedef struct {FhShadowRangeCacheEntry entry[FH_SHADOW_RANGE_CACHE_SLOTS];unsigned next;} FhShadowRangeCache;
/* Zero at owner creation, destroy with that owner. Trees are immutable for the
 * owner's entire lifetime. Exact key comparison after hash; collisions never
 * substitute another query. A hit reports visited=0, including empty spans. */
FhShadowRange fh_pica_shadow_range_cached(FhShadowRangeCache*,const FhShadowRangeNode*,const float[3],const float[3],int16_t);
#endif
