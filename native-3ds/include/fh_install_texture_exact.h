#ifndef FH_INSTALL_TEXTURE_EXACT_H
#define FH_INSTALL_TEXTURE_EXACT_H
#include <stddef.h>
#include <stdint.h>
typedef struct {
    const void *pixels,*palette;
    unsigned width,height,format,palette_format,palette_count;
    size_t pixel_bytes,palette_bytes;
} FhInstallExactTexture;
typedef int(*FhInstallExactProgress)(void*,uint32_t,uint32_t);
/* Exact source precision in PICA tiles; no quantization or resampling.
 * Output is caller-owned startup storage, never a live GPU allocation. */
int fh_install_texture_exact(const FhInstallExactTexture*,void*,size_t,unsigned*format,unsigned*bytes,FhInstallExactProgress,void*);
#endif
