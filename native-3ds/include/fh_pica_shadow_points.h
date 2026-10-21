#ifndef FH_PICA_SHADOW_POINTS_H
#define FH_PICA_SHADOW_POINTS_H
#include "fh_pica_shadow_track.h"

/* One point record per original collision triangle. PICA expands it, rather
 * than fetching the same ordered triangle at all three corners. Positions
 * are source signed fixed-point values shifted by three, BEFORE collision Y
 * offset/narrowing. Bounds retain the authored group and contiguous cell-mask
 * constraints. Offset is a live block uniform. No CPU transformed vertices.
 * Noncontiguous masks are explicitly rejected, never widened across holes. */
typedef struct {
    int16_t a[3],b[3],c[3],lo[3],hi[3],reserved;
} FhShadowPoint;
typedef struct {unsigned points,draws,gpu_bytes,reference_bytes,draw_bytes;} FhShadowPointSize;

/* Draw first/count use point indices, not expanded corner indices. Measure
 * with all output arrays NULL. Failure leaves all output and size unchanged.
 * No allocation; callers enforce resident and scratch caps before emission. */
int fh_pica_shadow_points_compile(const FhShadowTrackSource*,FhShadowPoint*,unsigned,
    FhShadowTrackDraw*,unsigned,FhShadowTrackLiveRef*,unsigned,FhShadowPointSize*);

/* Trusted prepared references; owner must retire readers before mutation and
 * flush [first,last) afterward. Changes only the eighteen position bytes.
 * Authored limits/topology are immutable; no query/normal/geometry generation.
 * This new format is not yet selected by the gameplay resource owner. */
unsigned fh_pica_shadow_points_update(FhShadowPoint*,const FhShadowTrackLiveRef*,unsigned,
    const int16_t (*)[3],unsigned*,unsigned*);

int fh_pica_shadow_points_raw_compile(const void*,unsigned,FhShadowPoint*,unsigned,
    FhShadowTrackDraw*,unsigned,FhShadowTrackLiveRef*,unsigned,FhShadowPointSize*,unsigned*,int16_t*);

/* Normal route is a direct point array. Only a proper alternate subset needs
 * indices. Query bounds stay unwrapped; frustum bounds describe signed16 GPU
 * positions. These differ near collision Y overflow and must not be conflated. */
typedef struct {
    unsigned count,indexed;
    int32_t query_lo[3],query_hi[3];
    int16_t visible_lo[3],visible_hi[3];
} FhShadowPointRoute;
typedef struct {FhShadowPointRoute route[2];unsigned extra_indices;} FhShadowPointRoutes;
int fh_pica_shadow_points_routes(const FhShadowPoint*,unsigned,const FhShadowTrackDraw*,unsigned,
    uint16_t*,unsigned,int16_t,FhShadowPointRoutes*);
void fh_pica_shadow_points_bounds(const FhShadowPoint*,const uint16_t*,int16_t,FhShadowPointRoutes*);
typedef struct {
    void *gpu;
    unsigned bytes,points,indices_offset;
    int16_t y_offset;
    FhShadowPointRoutes surfaces;
    unsigned cpu_bytes,source_vertices,range_offset[2],range_cache_offset;
    FhShadowTrackLiveRef references[];
} FhShadowPointPlan;
#endif
