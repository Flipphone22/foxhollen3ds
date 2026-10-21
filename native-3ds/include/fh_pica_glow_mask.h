#ifndef FH_PICA_GLOW_MASK_H
#define FH_PICA_GLOW_MASK_H
#include <stddef.h>
#include <stdint.h>

/* Optional approximation: quantize source alpha before filtering instead of
 * GX's after-filter quantization. Input is decoded RGBA8, top row first.
 * Output is PICA A8, Morton tiled, same row order as fh_texture_convert.
 * Do not add another Y flip: the existing particle UV shader owns that step.
 * Dimensions must be powers of two in [8,1024]. No allocation or publication;
 * caller owns storage and flushes it before GPU use. Invalid input writes none.
 * Nearest texel error <= 0.5/255; ideal bilinear error <= 8/248 + 0.5/255.
 * GPU filtering/rounding and visual quality need separate validation.
 * Never describe this as exact GX_ALPHA_BUMPN after filtering. */
int fh_pica_glow_mask_compile(uint8_t *destination, size_t capacity,
                              const uint8_t *rgba, size_t source_bytes,
                              unsigned width, unsigned height, size_t row_bytes);
/* Same approximation directly from source GX RGB5A3 tiles: no RGBA staging.
 * This is the format of all five currently resolved reflection-mask images. */
int fh_pica_glow_mask_rgb5a3(uint8_t *destination, size_t capacity,
                            const uint8_t *source, size_t source_bytes,
                            unsigned width, unsigned height);
#endif
