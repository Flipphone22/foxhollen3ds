#ifndef FH_PICA_ACTOR_FOG_H
#define FH_PICA_ACTOR_FOG_H
#include <stdint.h>
#include "fh_native_model.h"

/* Original GX position, before its independently selected texture-matrix
 * transform. Native visible geometry may contain rebased/skinned positions;
 * those are not an interchangeable input. Prepared at asset load only.
 * CPU-deformed/morphed source arrays require their live producer, separately. */
typedef struct { int16_t position[3]; int16_t padding; } FhPicaActorFogVertex;
/* Companion stream for the native fog shader. Position has its source VAT
 * scale and selected bind-space offset applied at load; skin stores palette
 * row IDs and the original quarter weight. Morph owners must update it from
 * their actual source positions. This helper does not retain source arrays. */
typedef struct { float position[3]; uint8_t skin[4]; } FhPicaActorFogGpuVertex;
typedef struct { float offset[3]; uint8_t first,second,weight,reserved; } FhPicaActorFogBinding;
int fh_pica_actor_fog_gpu_vertices(FhPicaActorFogGpuVertex *out,unsigned capacity,
    const int16_t (*positions)[3],unsigned position_count,const uint16_t *references,
    const FhPicaActorFogBinding *bindings,unsigned count,float scale,unsigned palette_bones);

/* Load-time source packet routing. -1 selects the texgen's default matrix;
 * nonnegative lanes index the packet's direct matrix prefix. The value in
 * that lane is a GX matrix ID, not a native bone index. Prepare the required
 * light/projection configurations at load; do not interpret GX per frame. */
typedef struct {
    int8_t matrix_lane[8];
    uint8_t packet_prefix_bytes;
    uint8_t texture_loads; /* Original usesTextureMatrix, not presence of TEXn. */
} FhPicaActorFogRouting;
/* Load-time symbolic source matrix bank, NOT a GPU palette. Texture and
 * position loads share the original bank; entries 10/11 in the source load
 * tables alias slot zero. Normal keys retain their distinct scale/translation
 * semantics for the eventual palette publisher. Zero initialization is unknown,
 * not identity. The caller seeds explicit pre-stream world loads separately. */
enum {FH_ACTOR_FOG_UNKNOWN,FH_ACTOR_FOG_POSITION,FH_ACTOR_FOG_NORMAL,
      FH_ACTOR_FOG_WORLD_POSITION,FH_ACTOR_FOG_WORLD_NORMAL};
typedef struct {uint16_t joint;uint8_t kind,reserved;} FhPicaActorFogMatrix;
typedef struct {FhModelRef visible;FhPicaActorFogMatrix fog;} FhPicaActorFogRef;
_Static_assert(sizeof(FhPicaActorFogRef)==16,"Actor fog reference wire size");
typedef struct {FhPicaActorFogMatrix slot[20];} FhPicaActorFogMatrixBank;
int fh_pica_actor_fog_matrix_valid(const FhPicaActorFogMatrix*,unsigned source_poses,unsigned allow_world);
/* Original ordinary vertex-animation preload: position slot9 and texture
 * slot19. Joint index zero here is a reserved tag, never source joint zero.
 * Subsequent original matrix loads may overwrite either entry normally. */
int fh_pica_actor_fog_matrix_world(FhPicaActorFogMatrixBank*);
/* Prepare one changed source palette entry, never a vertex array. Source pose
 * is original/unrebased (including an already-produced extra-joint matrix).
 * POSITION is view*pose. NORMAL follows the actual original cache mode:
 * cached clears translation after adjustment, uncached before adjustment.
 * This distinction matters for translated adjustment matrices. It must be
 * selected by the game adapter, not inferred from the native uniform cache.
 * Existing changed-slot publication owns caching/fences. Output may alias an
 * input; invalid/nonfinite results leave it unchanged. Uses the existing ARM
 * C_MTXConcat implementation, rather than a second matrix arithmetic backend. */
int fh_pica_actor_fog_pose(float out[3][4],unsigned kind,const float pose[12],
    const float view[3][4],const float adjustment[3][4],unsigned cached_normal);
/* Resolve a required original extra-joint matrix directly from live bones.
 * Same bind-tail translation and quarter blend as modelCalcVtxGroupMtxs.
 * This avoids restoring its bulk cache/extra-joint preparation. Call only for
 * selected keys; no source-array mutation or per-vertex CPU transformation. */
int fh_pica_actor_fog_blend_pose(float out[3][4],const float a[12],const float b[12],
    const float tail_a[3],const float tail_b[3],unsigned quarter_weight);
int fh_pica_actor_fog_matrix_load(FhPicaActorFogMatrixBank*,
    const uint8_t *joints,unsigned count,unsigned texture_loads,unsigned skip);
int fh_pica_actor_fog_matrix_select(FhPicaActorFogMatrix*,
    const FhPicaActorFogMatrixBank*,const FhPicaActorFogRouting*,unsigned coordinate,
    const uint8_t *packet,unsigned packet_bytes);
int fh_pica_actor_fog_routing(FhPicaActorFogRouting *out,unsigned joints,
    unsigned has_environment,unsigned has_auxiliary,unsigned texture_matrices,
    unsigned projected_lights,unsigned pass_mask,int first_projection_mode);

/* Ordinary source material coordinate count before renderHeavyFog. Environment
 * maps share one coordinate; bump adds 0, 2, or 3. projected_coordinates is
 * the selected source projection family's count (not its light count).
 * Precompile required alternatives at load. This does not admit a PICA material
 * or translate TEV stages. Failure leaves *out unchanged. */
int fh_pica_actor_fog_coordinate(unsigned *out,unsigned environment,
    unsigned bump_coordinates,unsigned projected_coordinates,unsigned layers,
    unsigned textured_layers,unsigned decal);
int fh_pica_actor_fog_vertices(FhPicaActorFogVertex *out,unsigned capacity,
    const int16_t (*positions)[3],unsigned position_count,
    const uint16_t *references,unsigned count);

/* Compose source fog planes with a source camera matrix. The caller must
 * resolve the actual texture matrix: default PNMTX0 or TEXnMTXIDX override,
 * not the first matrix in a regrouped native palette. Two rows are sufficient
 * for GPU dot products with the original source position. No vertex loop.
 * Output is source S,T; atlas orientation needs separate GPU validation.
 * Invalid input leaves output untouched. Input/output may alias. */
int fh_pica_actor_fog_rows(float out[2][4],const float planes[2][4],
    const float pnmtx0[3][4]);
#endif
