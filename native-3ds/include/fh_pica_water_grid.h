#ifndef FH_PICA_WATER_GRID_H
#define FH_PICA_WATER_GRID_H
#include <stddef.h>
#include <stdint.h>
/* Load-time prototype, not admitted to gameplay. All data derives from the
 * user's texture. No embedded game pixels or per-frame texture sampling.
 * One SHORT4 attribute, 16B stride/full fetch padding. UV is Q14, signed
 * indirect B/G offsets are Q4. Positions are derived from UV on the GPU. */
typedef struct {int16_t uv_delta[4];uint32_t padding[2];} FhWaterGridVertex;
typedef struct {uint32_t vertices,indices,vertex_bytes,index_bytes;} FhWaterGridSize;
int fh_water_grid_size(unsigned width,unsigned height,unsigned step,FhWaterGridSize *size);
/* RGB5A3 source tiled in 4x4 blocks, big endian; step=1,2,4,8. Clamped
 * bilinear base-level sampling at grid centres and boundaries. Quantization
 * and triangle interpolation are approximations, not GX indirect parity.
 * All arguments/capacities validated before writing either output. */
int fh_water_grid_compile(const void *rgb5a3,size_t bytes,unsigned width,
    unsigned height,unsigned step,FhWaterGridVertex *vertices,size_t vertex_bytes,
    uint16_t *indices,size_t index_bytes);
/* Same preparation, then omit only cells whose entire clamped bilinear alpha
 * support is zero. Compacts referenced vertices; result gives used bytes.
 * Reserve the upper bounds from size(). No opacity threshold/detail loss.
 * 8712B temporary stack at maximum dimensions; no heap allocation. */
int fh_water_grid_compile_visible(const void *rgb5a3,size_t bytes,unsigned width,
    unsigned height,unsigned step,FhWaterGridVertex *vertices,size_t vertex_bytes,
    uint16_t *indices,size_t index_bytes,FhWaterGridSize *result);
/* Procedural ripple source: A/B displacement, G coverage, source UV 127/128.
 * Geometry UV stays 0..1: the GPU mask scale must also include 127/128.
 * This prepares data only; it does not generate pixels or change simulation.
 * Not yet admitted to the gameplay owner. Same capacities/stack as above. */
int fh_water_grid_compile_ripple_visible(const void *rgb5a3,size_t bytes,
    unsigned step,FhWaterGridVertex *vertices,size_t vertex_bytes,
    uint16_t *indices,size_t index_bytes,FhWaterGridSize *result);
/* Experimental load-only radial topology, NOT a gameplay LOD selection.
 * Samples the same source A/B channels and covers the full square, leaving
 * the original G mask to determine coverage. No embedded/generated pixels.
 * segments=24..64 multiple8, rings=12..32 multiple4. No heap allocations;
 * capacities must come from radial_size, not visible-grid size. Quality and
 * GPU cost must be accepted independently before a runtime owner uses it. */
int fh_water_grid_ripple_radial_size(unsigned segments,unsigned rings,FhWaterGridSize *size);
int fh_water_grid_compile_ripple_radial(const void *rgb5a3,size_t bytes,
    unsigned segments,unsigned rings,FhWaterGridVertex *vertices,size_t vertex_bytes,
    uint16_t *indices,size_t index_bytes,FhWaterGridSize *result);
#endif
