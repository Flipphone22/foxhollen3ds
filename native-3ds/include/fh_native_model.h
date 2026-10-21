#ifndef FH_NATIVE_MODEL_H
#define FH_NATIVE_MODEL_H
#include <stdint.h>
#include <stddef.h>
/* Compiler/renderer bridge under construction; no game admission yet.
 * Original animation owns the pose. GPU packets own their resolved vertices
 * and palette until the existing renderer fence completes. */
enum { FH_MODEL_BONES=22,FH_MODEL_VERTICES=8192,FH_MODEL_INDICES=16384 };
typedef struct {uint16_t position,normal,color,uv,joint,material;} FhModelRef;
typedef struct {uint8_t a,b,weight,bind_space;} FhModelBinding;
typedef struct {
    float position[4],color[4],uv[2],normal[3],skin[3];
} FhModelVertex;
/* PICA fetch converts integer attributes; bind-tail addition stays in the
 * vertex shader. CPU animation retains its original, separately owned arrays. */
typedef struct {
    int16_t position[4];
    uint8_t color[4];
    int16_t uv[4];
    int8_t normal[4];
    uint8_t skin[4];
    float tail[3];
} FhModelCompactVertex;
_Static_assert(sizeof(FhModelCompactVertex)==40,"Compact PICA attribute stride");
/* FHNSCN0B uses this same stride with both skin weights in byte/128 units.
 * Offline preparation verifies independently decoded normal bindings match. */
_Static_assert(sizeof(FhModelRef)==12,"Native model reference wire size");
_Static_assert(sizeof(FhModelVertex)==64,"Native weighted attribute stride");
typedef struct {
    const unsigned char *positions,*normals,*colors,*uvs;
    unsigned position_count,normal_count,color_count,uv_count,normal_stride;
    const FhModelBinding *bindings;unsigned binding_count;
    const float (*tails)[3];unsigned bone_count;
    unsigned format; /* Original GX format 3,5,6,7; arrays remain big endian. */
    unsigned secondary_uv; /* Direct v5 groups: refs.material is TEX1 index. */
} FhModelSource;
/* All counts/indices are checked before writing any output. Callers must
 * validate the actual backing allocation ranges when constructing sources.
 * These functions allocate nothing and retain no game-owned pointers. */
int fh_native_model_vertices(FhModelVertex *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *source,
    const uint16_t *bones,unsigned bone_count);
enum { FH_MODEL_POSITION=1,FH_MODEL_NORMAL=2,FH_MODEL_COLOR=4,FH_MODEL_UV=8 };
/* Update only mutated source attributes on an already validated resident mesh.
 * Topology, bindings, tails and palette must remain those of the admitted asset.
 * Validate every referenced changed attribute before touching any output byte;
 * unchanged attributes (including skin weights) remain byte-identical. */
int fh_native_model_refresh(FhModelVertex *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *source,unsigned changed);
/* In-place compaction of a validated float packet, before any draw uses it.
 * The original allocation/index offsets remain unchanged. */
int fh_native_model_compact(FhModelVertex *storage,const FhModelRef *refs,
    unsigned count,const FhModelSource *source);
int fh_native_model_refresh_compact(FhModelCompactVertex *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *source,unsigned changed);
/* Produce view * animated_joint * translate(-bind_tail). The caller supplies
 * original 4x4 game matrices (16-float stride), and a 3x4 view matrix.
 * Rigid vertices add their tail once; blend vertices are already bind-space. */
int fh_native_model_palette(float out[FH_MODEL_BONES][3][4],
    const uint16_t *bones,unsigned count,const float *poses,unsigned pose_count,
    const float (*tails)[3],const float view[3][4]);
#endif
