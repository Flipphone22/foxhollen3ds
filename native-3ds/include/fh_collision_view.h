#ifndef FH_COLLISION_VIEW_H
#define FH_COLLISION_VIEW_H
#include <stdint.h>

/* trackBuildBlockTriangles consumes polygon/vertex arrays synchronously and
 * read-only. Use cacheAllocAndCopy's existing no-staging fallback on ARM,
 * where the GC locked-cache DMA buffers are ordinary memory copies. Preserve
 * its cursor/output ordering and zero queued-transfer result. Do not use for
 * mutable particle/model scratch buffers or GPU-visible resource ownership. */
static inline int fh_collision_source_view(void *source, uint32_t bytes,
        uintptr_t *cursor, uintptr_t *end, uint32_t limit) {
    (void)bytes;
    (void)limit;
    *end = *cursor;
    *cursor = (uintptr_t)source;
    return 0;
}
#endif
