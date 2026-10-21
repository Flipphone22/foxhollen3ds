#ifndef FH_PICA_HEIGHT_FOG_H
#define FH_PICA_HEIGHT_FOG_H
#include <stdint.h>
enum { FH_HEIGHT_FOG_SIZE=64, FH_HEIGHT_FOG_BYTES=4096,
       FH_HEIGHT_FOG_STATES=65, FH_HEIGHT_FOG_ATLAS_BYTES=65536 };
/* Eight immutable 64x128 A8 pages encode all source camera-height states.
 * Each 64x64 view begins on a complete tile-row boundary, preserving sampler
 * clamp/filtering at that view's edges. No per-frame pixel conversion. */
void fh_pica_height_fog_atlas_prepare(uint8_t *destination);
unsigned fh_pica_height_fog_atlas_offset(unsigned intensity);
/* Source post-texture matrix. Apply AFTER source PNMTX0, not the vertex's
 * independently selected skinning matrix. Returns zero for invalid ranges. */
int fh_pica_height_fog_planes(float planes[2][4],const float inverse_view[3][4],
    float depth_scale,float depth_offset,float top,float bottom);
void fh_pica_height_fog_world_set(const uint8_t *source,unsigned intensity,
    const float inverse_view[3][4],float depth_scale,float depth_offset,
    float top,float bottom,unsigned color,unsigned indirect);
/* Clear the last block's snapshot without reading camera/source parameters. */
void fh_pica_height_fog_world_disable(void);
/* Original selector updates; no redundant source 4 KiB rasterization. */
extern unsigned fh_pica_height_fog_source_updates;
unsigned fh_pica_height_fog_world_active(void);
void fh_pica_height_fog_world_skipped(void);
void fh_pica_height_fog_world_prepare(void);
void fh_pica_height_fog_world_bind(unsigned enabled);

/* Shader/owner changes invalidate uniforms and TEV; material writes to stage3
 * invalidate only the TEV override. Texture binding remains explicit. */
void fh_pica_height_fog_invalidate(void);
void fh_pica_height_fog_stage_invalidate(void);
/* Resource owner is deliberately separate from material selection. Calling
 * update admits the first resident allocation; unimplemented families must
 * not call it just to populate unused textures. The selector is the original
 * camera-height intensity (0..64), not a frame number or pointer identity. */
void fh_pica_height_fog_update(const uint8_t *source,unsigned intensity);
#ifdef __3DS__
#include <citro3d.h>
const C3D_Tex *fh_pica_height_fog_sampler(void);
#endif
/* Queue must already be retired, as for native morph/splash shutdown. */
void fh_pica_height_fog_shutdown(void);
#endif
