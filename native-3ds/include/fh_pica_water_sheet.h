#ifndef FH_PICA_WATER_SHEET_H
#define FH_PICA_WATER_SHEET_H
#include <stdint.h>
#include "fh_graphics.h"
/* Original immutable quad copied once at load. Source x/y/z and s/t are S16;
 * UV uses the original seven fraction bits. No source colors are discarded:
 * these water quads use uniform live tint and opacity in their material. */
void fh_pica_water_sheet_init(void *shader_entry);
/* Setup/load-time program ownership and shared native scene capture. */
int fh_pica_water_sheet_prepare(void);
const void *fh_pica_water_sheet_capture(void);
/* Returns0 with no retained resource on optional-memory pressure. The caller
 * must mark the effect unavailable and skip its draws; never retry per frame. */
int fh_pica_water_sheet_load(const int16_t positions[4][3],
    const int16_t uv[4][2],const uint16_t indices[6],const FhTexture *mask);
/* capture is a C3D_Tex borrowed from an ordered native color snapshot.
 * The caller admits enabled/active sheets and captures BEFORE the first one.
 * Both draws must use this same immutable snapshot; no recapture here.
 * reflected matrices operate on eye-space positions, as in original
 * loadReflectionTexMtxs. mask alpha currently omits GX's filtered5bit step.
 * Returns 0 for a clipped target, otherwise 2 native draws. */
unsigned fh_pica_water_sheet_draw(const void *capture,
    const float model_view[3][4],const float projection[4][4],
    const float reflected[2][3][4],const unsigned scissor[4],
    const uint8_t tint[4],uint32_t reflection_mix,unsigned heavy_fog);
void fh_pica_water_sheet_release(void);
/* Shutdown caller has already retired the common command queue. */
/* Second resident plan; original generated64x64 RGB5A3 A/B/G source,
 * 127/128 UV endpoints. Reduced once before upload, never during draw.
 * Shared release/shutdown retires both plans. Gameplay adapter is separate. */
int fh_pica_ripple_load(const int16_t positions[4][3],const int16_t uv[4][2],
    const uint16_t indices[6],const FhTexture *mask);
unsigned fh_pica_ripple_draw(const void *capture,const float model_view[3][4],
    const float projection[4][4],const float reflected[2][3][4],
    const unsigned scissor[4],const uint8_t tint[4],uint32_t reflection_mix,
    unsigned heavy_fog);
typedef struct {float view[3][4];uint8_t alpha;} FhWaterSheetInstance;
/* Up to30 immutable pose/opacity snapshots. No per-frame geometry. The owner
 * keeps source pass order, and restores full state after each queue reset. */
unsigned fh_pica_water_sheet_batch(unsigned ripple,const void *capture,
    const FhWaterSheetInstance *instances,unsigned count,const float projection[4][4],
    const float reflected[2][3][4],const unsigned scissor[4],const uint8_t tint[4],
    uint32_t reflection_mix,unsigned heavy_fog);
void fh_pica_water_sheet_shutdown(void);
#endif
