#ifndef FH_PICA_GLOW_PACKED_H
#define FH_PICA_GLOW_PACKED_H
#include <stddef.h>
#include "fh_pica_glow_adaptive.h"

/* CPU metadata outside linear RAM; pointers borrow one immutable GPU block.
 * The owner flushes that block before publishing this view, and retires all
 * GPU readers before freeing it. Packing itself does not publish or flush. */
typedef struct {
    unsigned vertices,triangles;
    const FhGlowMeshVertex *vertex;
    const uint16_t (*index)[3];
} FhGlowMeshView;

/* Load-time validation only. Zero means invalid input. Storage is 16-aligned;
 * only populated vertices/indices and deterministic alignment padding remain.
 * On failure both destination bytes and output view are unchanged. */
size_t fh_glow_mesh_packed_bytes(const FhGlowMesh *);
int fh_glow_mesh_pack(void *,size_t,const FhGlowMesh *,FhGlowMeshView *);
#endif
