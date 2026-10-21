#ifndef FH_PICA_DISTORTION_MESH_H
#define FH_PICA_DISTORTION_MESH_H
#include <stddef.h>
#include <stdint.h>
enum {FH_DISTORTION_FIELD_BYTES=256*256*2,FH_DISTORTION_SEGMENTS=128,
      FH_DISTORTION_RINGS=101,FH_DISTORTION_VERTICES=12929,
      FH_DISTORTION_INDICES=77184};
typedef struct {float uv[2];int16_t offset[2];} FhDistortionVertex;
/* Load-time compiler. Field is alpha/intensity pairs: tiled=0 row-major,
 * tiled=1 original GX IA8 layout. Read source pixels directly without a copy.
 * Caller supplies disjoint buffers; no allocation, SDK commands or publication.
 * Counts are elements. Invalid capacities leave both outputs untouched.
 * Resident GPU resources must be flushed/published by their eventual owner.
 * Outer field domain, clipping and pixel fidelity are not yet GPU validated. */
int fh_pica_distortion_mesh(const uint8_t *field,size_t field_bytes,unsigned tiled,
    FhDistortionVertex *vertices,size_t vertex_capacity,
    uint16_t *indices,size_t index_capacity);
#endif
