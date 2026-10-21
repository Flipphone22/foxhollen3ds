#ifndef FH_PICA_SHADOW_SPANS_H
#define FH_PICA_SHADOW_SPANS_H
#include "fh_pica_shadow_ranges.h"
/* Conservative bounded traversal: remove at most three proven-empty gaps.
 * Retain route order, authored filters, and every potentially visible point.
 * No triangle copying, sorting, or live-position assumptions. */
enum { FH_SHADOW_SPAN_CAP=4, FH_SHADOW_SPAN_MIN_GAP=128,
       FH_SHADOW_SPAN_NODE_BUDGET=256 };
typedef struct { uint16_t first,count; } FhShadowSpan;
typedef struct { FhShadowSpan span[FH_SHADOW_SPAN_CAP];
    unsigned count,points,visited; } FhShadowSpans;
typedef struct {
    uint32_t key[6],hash;const FhShadowRangeNode *tree;int16_t y;
    FhShadowSpans result;
} FhShadowSpansEntry;
typedef struct { FhShadowSpansEntry entry[FH_SHADOW_RANGE_CACHE_SLOTS];
    unsigned next; } FhShadowSpansCache;
FhShadowSpans fh_pica_shadow_spans(const FhShadowRangeNode*,const float[3],const float[3],int16_t);
/* Owner lifetime and exact-key rules match the immutable single-range cache. */
FhShadowSpans fh_pica_shadow_spans_cached(FhShadowSpansCache*,const FhShadowRangeNode*,const float[3],const float[3],int16_t);
#endif
