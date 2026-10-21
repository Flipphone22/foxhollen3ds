#ifndef FH_NATIVE_SCENE_H
#define FH_NATIVE_SCENE_H
#include "fh_native_model.h"
#include "fh_world_bounds.h"
#include "fh_animated_bounds.h"
/* World draws have no character scratch-index array. Bound original lists to
 * the existing 64 KiB gather capacity; vertex and allocation caps stay fixed. */
enum { FH_WORLD_INDICES = 32766 };
/* FHNSCN07 static world: original signed coordinates/UVs and RGBA4 lanes.
 * Existing world shader uses scaled view/tint/UV uniforms, no extra ALU. */
typedef struct {
    int16_t position[4];
    uint8_t color[4];
    int16_t uv[2],second_uv[2];
} FhWorldCompactVertex;
_Static_assert(sizeof(FhWorldCompactVertex)==20,"World PICA attribute stride");
/* CPU metadata and GPU storage have separate ownership and budgets. Opening
 * verifies structure only; no source/material/mutation admission is implied. */
typedef struct {
    const unsigned char *meta,*gpu;
    unsigned metadata_bytes,gpu_bytes,id,block,groups,draws,bones,bindings;
    unsigned groups_at,draws_at,sections_at,sections,tails_at,bindings_at;
    unsigned direct; /* FHNSCN03: original UVs and live material, no atlas. */
    unsigned compact; /* Runtime model storage or FHNSCN07 static world. */
} FhSceneAsset;
typedef struct {
    unsigned page,format,cull,first,draws,vertices,indices,bones,first_section;
    const FhModelVertex *vertex;
    const uint16_t *index,*bone_ids;
    const FhModelRef *refs;
    unsigned two_uv; /* v4 world: normal.xy=TEX1; v5 model: refs.material=TEX1. */
} FhSceneGroup;
typedef struct {unsigned list,shader,first,count,texture_crc,texture_extent,texture_wrap,shader_flags;} FhSceneSection;
/* Persistent main-world program. Geometry is borrowed from its owning scene;
 * uniforms, source validation, visibility and material state stay live. */
typedef struct { uint16_t bit,ordinal; uint8_t kind,value,shader,reserved; } FhWorldPlanOp;
typedef struct { unsigned commands,draws; const FhWorldPlanOp *command;
    const FhSceneGroup *draw; } FhWorldDrawPlan;
/* NULL outputs measure/validate. Publication is the caller's responsibility. */
unsigned fh_world_plan_compile(const FhSceneAsset *asset,const void *instructions,unsigned bytes,
    FhWorldPlanOp *commands,unsigned capacity,FhSceneGroup *draws,unsigned draw_capacity);
extern unsigned fh_world_plan_runs,fh_world_plan_draws;
const FhWorldDrawPlan *fh_native_scene_world_plan(const FhSceneAsset *asset);
int fh_native_scene_open(FhSceneAsset *out,const void *meta,unsigned meta_bytes,
    const void *gpu,unsigned gpu_bytes,unsigned id,unsigned source_bytes,unsigned source_crc);
int fh_native_scene_group(const FhSceneAsset *asset,unsigned index,FhSceneGroup *out);
int fh_native_scene_section(const FhSceneAsset *asset,unsigned index,FhSceneSection *out);
int fh_native_scene_draw(const FhSceneAsset *asset,unsigned ordinal,unsigned list,unsigned shader,unsigned *group);
int fh_native_scene_stream_draw(const FhSceneAsset *asset,unsigned stream,unsigned bit,unsigned list,unsigned shader,unsigned *ordinal);
/* Optional registration at asset load, before endian/pointer relocation. The
 * GPU payload is immutable until renderer completion. No frame-time SD reads. */
void fh_native_scene_register(const void *owner,const void *raw,unsigned bytes,unsigned id);
void fh_native_scene_forget(const void *allocation,unsigned bytes);
void fh_native_scene_shutdown(void);
/* Main renderer owner only: optional CPU file cache yields to live assets. */
unsigned fh_native_scene_cache_release(void);
const FhSceneAsset *fh_native_scene_find(const void *owner);
/* Snapshot order: position, normal, color, UV, instructions, original lists,
 * then extra-joint definitions for models. World attributes use native u16
 * byte order after the game's load conversion; model attributes remain BE. */
int fh_native_scene_source(const FhSceneAsset *asset,unsigned index,const void *data,unsigned bytes);
int fh_native_scene_prepare_model(const FhSceneAsset *asset,const FhModelSource *source);
int fh_native_scene_prepare_world_positions(const FhSceneAsset *asset,const void *positions,unsigned count);
/* Mutates only a validated version8 packet, with ownership held by caller. */
int fh_native_scene_refresh_world_positions(const FhSceneAsset *asset,const void *positions,unsigned count);
/* Private unpublished/fenced storage only. On failure discard the record;
 * it may contain partly compacted groups. Returns the used GPU extent. */
unsigned fh_native_scene_compact_model(FhSceneAsset *asset,const FhModelSource *source);
void fh_native_scene_used(const FhSceneAsset *asset);
const FhAnimatedBounds *fh_native_scene_model_bounds(const FhSceneAsset *asset,const FhSceneGroup *group);
const FhWorldBounds *fh_native_scene_world_bounds(const FhSceneAsset *asset,
    const FhSceneGroup *group,unsigned *first,unsigned *total,const FhWorldBounds **parents);
unsigned fh_native_scene_texture_crc(const void *data,unsigned bytes);
void fh_native_scene_texture_written(const void *address,size_t bytes);
extern unsigned fh_native_scene_loaded,fh_native_scene_rejected,fh_native_scene_cpu_bytes,fh_native_scene_gpu_bytes;
extern unsigned fh_native_scene_admission[16];
extern unsigned fh_native_scene_enabled;
#include "fh_pica_shadow_points.h"
extern unsigned fh_pica_shadow_track_enabled;
const FhShadowPointPlan *fh_native_scene_shadow_points(const FhSceneAsset *asset);
#endif
