#ifndef FH_INSTALL_TEXTURE_LAYOUT_H
#define FH_INSTALL_TEXTURE_LAYOUT_H
#include "fh_install_texture_exact.h"

typedef struct {
    FhInstallExactTexture image;
    unsigned offset,available,chain_bytes;
} FhInstallTexturePlane;
typedef struct {
    FhInstallTexturePlane planes[2]; /* Source colour; optional independent I4. */
    unsigned count,min_lod,max_lod,chain_complete;
} FhInstallTextureLayout;
/* Borrow a decoded original 96-byte-header record. Every base plane must fit;
 * no allocations, implicit decompression, mip omission or inferred palette.
 * chain_complete additionally requires all declared mip bytes, without spare
 * bytes. Base-only consumers must not present that as full mip preparation.
 * Nested preload wrappers and indexed layouts without a proven palette fail.
 * Output is cleared on failure. Pointers remain valid while record is alive. */
int fh_install_texture_layout(const void *record,size_t bytes,FhInstallTextureLayout*);
#endif
