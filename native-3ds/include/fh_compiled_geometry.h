#ifndef FH_COMPILED_GEOMETRY_H
#define FH_COMPILED_GEOMETRY_H
#include "fh_graphics.h"
/* CPU staging limit, shared by the decoder and native indexed retention.
 * Cache arena budgets are independent and do not grow with this limit. */
enum { FH_COMPILED_RETAINED_UNIQUE=2048 };
typedef struct { float position[4],color[4],uv[3][3],matrix_index,generated_normal_uv,normal[3]; } FhNativeVertex;
typedef struct { unsigned units,coord[3],generated_units;int primary[4];float scale[3][2];unsigned affine,sphere; } FhNativePacking;
/* Dynamic integer attributes; six loaded inputs plus fixed color/scale.
 * Allocation remains inside the existing fenced vertex stream. */
typedef struct { int16_t position[4];float uv[3][3];int16_t matrix[2];int8_t normal[4]; } FhCompactVertex;
typedef struct { float color[4],position_scale,normal_scale; } FhCompactPacking;
_Static_assert(sizeof(FhCompactVertex)==52,"Compact vertex loader stride");
static inline void fh_compact_expand(FhNativeVertex *out,const FhCompactVertex *in,const FhCompactPacking *pack){
    *out=(FhNativeVertex){0};
    for(unsigned c=0;c<3;c++){out->position[c]=in->position[c]*pack->position_scale;out->normal[c]=in->normal[c]*pack->normal_scale;}
    out->position[3]=in->position[3];
    for(unsigned c=0;c<4;c++)out->color[c]=pack->color[c];
    for(unsigned u=0;u<3;u++)for(unsigned c=0;c<3;c++)out->uv[u][c]=in->uv[u][c];
    out->matrix_index=in->matrix[0];out->generated_normal_uv=in->matrix[1];
}
typedef struct FhCompiledMesh FhCompiledMesh;
/* NULL output queries eligibility only. A failed write publishes nothing;
 * the renderer overwrites the unused stream span through its scalar route.
 * Return2 requests comparison against the original decoder and renderer. */
typedef int (*FhCompiledDecodeNative)(FhNativeVertex *,const FhCompiledMesh *,const FhNativePacking *);
typedef int (*FhCompiledDecodeCompact)(FhCompactVertex *,const FhCompiledMesh *,const FhNativePacking *,FhCompactPacking *);
struct FhCompiledMesh {
    const unsigned char *packets;
    const uint32_t *offsets;
    const uint16_t *indices;
    unsigned unique,count;
    FhCompiledDecodeNative decode_native;
    FhCompiledDecodeCompact decode_compact;
    /* Union of valid PNMTX byte-zero rows in every owned input packet.
     * Used only when the current descriptor proves PNMTXIDX is byte zero.
     * Unknown/invalid values conservatively retain all ten matrices. */
    unsigned matrix_mask;
    /* Borrowed for this synchronous draw only. CPU identity names a template;
     * GPU coefficient keys must additionally own these live matrix values. */
    const float (*fur_post)[4];
};
typedef void (*FhCompiledDecode)(FhVertex *output,const unsigned char *packet);
/* Nonzero result identifies immutable decoded packets, after source validation.
 * With fur_post it identifies the pre-post-transform template; output has the
 * current post applied and its GPU key must include the matrix separately.
 * NULL output probes identity only; a miss must not decode or publish data. */
typedef uint64_t (*FhCompiledDecodeBatch)(FhVertex *output,const FhCompiledMesh *mesh);
const FhCompiledMesh *fh_compiled_mesh(const void *list,unsigned bytes,unsigned stride,unsigned format);
void fh_compiled_reset(void);
void *fh_geometry_compiled_storage(void);
int fh_renderer_draw_compiled(const FhCompiledMesh *mesh,const FhDrawState *state,FhCompiledDecode decode,FhCompiledDecodeBatch batch);
int fh_renderer_compiled_fur_available(const FhDrawState *state);
int fh_renderer_fur_texgen_available(const FhDrawState *state);
const float (*fh_compiled_fur_post(void))[4];
extern unsigned fh_compiled_disabled,fh_compiled_hits,fh_compiled_misses,fh_compiled_unique,fh_compiled_indices;
#endif
