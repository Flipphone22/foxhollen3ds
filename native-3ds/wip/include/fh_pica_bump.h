#ifndef FH_PICA_BUMP_H
#define FH_PICA_BUMP_H
#include "fh_native_model.h"

/* Two independent authored axes. The normal and bone weights remain in the
 * existing vertex stream; neither orthogonalization nor normalization is valid
 * for the original indirect environment lookup. Padding is fetched explicitly. */
typedef struct {int8_t binormal[4],tangent[4];} FhPicaBumpBasis;
_Static_assert(sizeof(FhPicaBumpBasis)==8,"PICA bump basis fetch extent");

/* Load-time compiler, no allocation or retained source pointers. The caller
 * proves the source backing extent. Invalid input never partially publishes.
 * Output must be disjoint from refs/source. NULL output validates/measures. */
int fh_pica_bump_basis(FhPicaBumpBasis *out,unsigned capacity,
    const FhModelRef *refs,unsigned count,const FhModelSource *source);

typedef struct {
    float basis_scale; /* Original post-texgen scale, not a normal-map strength. */
    unsigned extra_texgen_scale; /* 0 inherits the original base coordinate. */
    unsigned indirect_scale_log2; /* GX_ITS exponent; NOT a linear divisor. */
    unsigned writes_indirect_scale; /* Original no-write preserves caller state. */
} FhPicaBumpParameters;

/* Pure compilation of addEnvMapBumpStages parameters. Disabled returns zero
 * without publishing output. Invalid/OOB source lookup returns -1. */
int fh_pica_bump_parameters(FhPicaBumpParameters *out,unsigned enabled,
    unsigned aux_width,unsigned base_width,unsigned packed);

/* Load-time fragment-field candidate. Linear RGBA8 input comes from the
 * validated auxiliary-image decoder; output is linear RGBA8 for the normal
 * texture packer. RGB encodes (2*strength*S/256,2*strength*T/256,1)/decode_scale,
 * where S=alpha-128 and T=blue-128. Decode as (2*RGB/255-1)*decode_scale.
 * Do not reconstruct Z or normalize the decoded vector: that changes the field.
 * This field is NOT by itself equivalent to GX's dependent environment lookup.
 * A consumer still needs a measured authored-basis and lighting implementation.
 * Caller proves backing extents and disjoint buffers; invalid input publishes
 * neither pixels nor scale. No allocation, retained pointers or per-frame work.
 */
int fh_pica_bump_field(uint8_t *out,size_t capacity,float *decode_scale,
    const uint8_t *rgba,unsigned pixels,unsigned packed);
/* Direct native RGB8 Morton packing of the same field: three bytes per texel,
 * no full-image intermediate. Dimensions must be PICA power-of-two 8..1024.
 * The eventual owner must allocate aligned GPU-visible storage, flush it and
 * retire it through the existing queue before reuse/free; this pure compiler
 * does not transfer ownership. Original source coordinates/wrap are unchanged. */
int fh_pica_bump_texture(uint8_t *out,size_t capacity,float *decode_scale,
    const uint8_t *rgba,unsigned width,unsigned height,unsigned packed);
#endif
