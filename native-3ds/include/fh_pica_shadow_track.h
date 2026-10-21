#ifndef FH_PICA_SHADOW_TRACK_H
#define FH_PICA_SHADOW_TRACK_H
#include <stdint.h>

/* Load-time inputs borrow the original, native-endian collision arrays. They
 * never replace the game's collision state. groups includes its end sentinel. */
typedef struct {
    uint16_t first;
    int16_t min_x,max_x,min_y,max_y,min_z,max_z;
    uint16_t reserved;
    uint32_t flags;
} FhShadowTrackGroup;
typedef struct {
    const int16_t (*position)[3];
    const uint16_t (*triangle)[4];
    const FhShadowTrackGroup *group;
    unsigned vertices,triangles,groups;
    int16_t y_offset;
} FhShadowTrackSource;
/* Flat source face normals are replicated at load time, allowing the PICA to
 * reject light-facing-away triangles without a CPU triangle walk each frame.
 * Twenty bytes per corner; no skin/material/UV fields or per-frame rebuilding.
 * This compiler does not yet connect these attributes to a gameplay shader. */
typedef struct {int16_t position[4];float normal[3];} FhShadowTrackVertex;
typedef struct {
    unsigned first,count,source_group,flags;
    int16_t lo[3],hi[3];
} FhShadowTrackDraw;
typedef struct {unsigned corners,draws,gpu_bytes,cpu_bytes;} FhShadowTrackSize;
/* Validate/measure with NULL output arrays. When emitting, capacities are in
 * elements. Invalid input/capacity leaves both arrays and size untouched.
 * Original triangle order/winding is preserved. No allocations or GPU calls. */
int fh_pica_shadow_track_compile(const FhShadowTrackSource*,FhShadowTrackVertex*,unsigned,
                                 FhShadowTrackDraw*,unsigned,FhShadowTrackSize*);
typedef struct {FhShadowTrackSize base;unsigned vertices,scratch_bytes;} FhShadowTrackIndexedSize;
/* Exact position+normal sharing, preserving ordered indices and draw ranges.
 * Transient load compiler storage is capped at1MiB, never resident/per-frame.
 * Failure is explicit and cannot publish partial geometry. */
int fh_pica_shadow_track_indexed_compile(const FhShadowTrackSource*,FhShadowTrackVertex*,unsigned,
    uint16_t*,unsigned,FhShadowTrackDraw*,unsigned,FhShadowTrackIndexedSize*);
/* Original big-endian MapBlock payload, before runtime pointer relocation.
 * Decode/validation and compilation run at load time only. Transient input
 * storage is bounded to512KiB in addition to the indexed compiler's1MiB cap.
 * y_offset may be returned for conversion from collision coordinates to the
 * independently positioned render block; callers must not double-apply it. */
int fh_pica_shadow_track_raw_compile(const void*,unsigned,FhShadowTrackVertex*,unsigned,
    uint16_t*,unsigned,FhShadowTrackDraw*,unsigned,FhShadowTrackIndexedSize*,int16_t*);
/* Load-only decoded view; callback may not retain its transient pointers. */
int fh_pica_shadow_track_raw_scratch(const void*,unsigned,unsigned*);
int fh_pica_shadow_track_raw_source(const void*,unsigned,int (*)(const FhShadowTrackSource*,void*),
    void*,unsigned*,int16_t*);
/* One persistent draw per source surface selection and block. The ordinary
 * route borrows all indices. The alternate route borrows them too when all
 * faces qualify, otherwise copies only its ordered subset at load time.
 * index_offset is in a conceptual [original indices, alternate indices] array.
 * Empty routes are explicit. Output/capacity rejection is atomic. */
typedef struct {unsigned index_offset,count;int16_t lo[3],hi[3];} FhShadowTrackRoute;
typedef struct {FhShadowTrackRoute route[2];unsigned extra_indices;} FhShadowTrackRoutes;
int fh_pica_shadow_track_routes(const FhShadowTrackVertex*,unsigned,const uint16_t*,unsigned,
    const FhShadowTrackDraw*,unsigned,uint16_t*,unsigned,FhShadowTrackRoutes*);
int fh_pica_shadow_track_live_routes(const uint16_t*,unsigned,const FhShadowTrackDraw*,unsigned,
    uint16_t*,unsigned,FhShadowTrackRoutes*);
typedef struct {uint16_t source[3];} FhShadowTrackLiveRef;
/* One persistent allocation per loaded block; no per-frame compilation. The
 * scene owner retires previous readers before position writes or release. */
typedef struct {
    void *gpu;
    unsigned bytes,vertices,indices_offset;
    int16_t y_offset;
    FhShadowTrackRoutes surfaces;
    unsigned cpu_bytes,source_vertices,reference_count;
    FhShadowTrackLiveRef references[];
} FhShadowTrackPlan;
/* Mutable terrain keeps a persistent triangle topology. Every corner carries
 * the SAME ordered three points; only selector varies. PICA computes one
 * identical face decision at all corners, preventing partial rejection edges.
 * Position lanes may change after retirement; indices/selectors never do. */
typedef struct {int16_t a[3],selector,b[3],c[3];} FhShadowTrackLiveVertex;
typedef struct {FhShadowTrackSize base;unsigned references,scratch_bytes;} FhShadowTrackLiveSize;
int fh_pica_shadow_track_live_compile(const FhShadowTrackSource*,FhShadowTrackLiveVertex*,unsigned,
    uint16_t*,unsigned,FhShadowTrackDraw*,unsigned,FhShadowTrackLiveRef*,unsigned,FhShadowTrackLiveSize*);
int fh_pica_shadow_track_raw_live_compile(const void*,unsigned,FhShadowTrackLiveVertex*,unsigned,
    uint16_t*,unsigned,FhShadowTrackDraw*,unsigned,FhShadowTrackLiveRef*,unsigned,FhShadowTrackLiveSize*,int16_t*);
/* Trusted load-validated references. Caller owns an unpublished/retired buffer
 * and then flushes [first_byte,last_byte). No allocation, topology change,
 * source interpretation or CPU normals. Returns changed triangle count. */
unsigned fh_pica_shadow_track_live_positions(FhShadowTrackLiveVertex*,const FhShadowTrackLiveRef*,unsigned,
    const int16_t (*)[3],int16_t,unsigned*,unsigned*,FhShadowTrackRoutes*);
#endif
