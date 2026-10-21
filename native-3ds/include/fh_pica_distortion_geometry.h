#ifndef FH_PICA_DISTORTION_GEOMETRY_H
#define FH_PICA_DISTORTION_GEOMETRY_H
#include <citro3d.h>
#include "fh_pica_distortion_mesh.h"
enum {
    FH_DISTORTION_VERTEX_BYTES=FH_DISTORTION_VERTICES*sizeof(FhDistortionVertex),
    FH_DISTORTION_INDEX_OFFSET=(FH_DISTORTION_VERTEX_BYTES+127)&~127,
    FH_DISTORTION_GEOMETRY_BYTES=FH_DISTORTION_INDEX_OFFSET+
        FH_DISTORTION_INDICES*sizeof(uint16_t)
};
typedef struct {
    void *allocation;
    C3D_AttrInfo attributes;
    C3D_BufInfo buffers;
} FhPicaDistortionGeometry;
/* Load-boundary operation, after the enabled check. Owner starts zeroed.
 * Refuses replacing a published allocation; does not retry or allocate on draws.
 * reserve_bytes is the caller's required remaining linear-memory headroom.
 * On any failure, the owner is unchanged and all unpublished storage is freed. */
int fh_pica_distortion_geometry_prepare(FhPicaDistortionGeometry *owner,
    const uint8_t *field,size_t field_bytes,unsigned tiled,size_t reserve_bytes);
/* Caller MUST complete every GPU reader and retire buffer bindings first.
 * Never call this on timeout. This helper does not fabricate a completed fence. */
void fh_pica_distortion_geometry_destroy_after_fence(FhPicaDistortionGeometry *owner);
static inline const uint16_t *fh_pica_distortion_geometry_indices(
    const FhPicaDistortionGeometry *owner)
{
    return (const uint16_t*)((const char*)owner->allocation+FH_DISTORTION_INDEX_OFFSET);
}
#endif
