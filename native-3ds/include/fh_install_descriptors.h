#ifndef FH_INSTALL_DESCRIPTORS_H
#define FH_INSTALL_DESCRIPTORS_H
#include <stdint.h>
/* Installation-time source descriptions for the native geometry compiler.
 * Palette values remain source indices, not live matrices or GPU bindings. */
typedef struct {
    unsigned list,shader,stream,bit_offset;
    unsigned layout[6]; /* matrix-byte count, POS/NRM/COL/UV widths, UV layers */
    unsigned palette[15];
} FhInstallDrawSource;
typedef int (*FhInstallDrawEmit)(void*,const FhInstallDrawSource*);
/* 1 complete, 0 invalid/unsupported, -1 cancelled. Validates all streams before
 * emitting. Source remains immutable; no allocations or game-frame calls. */
int fh_install_descriptors(const void*,uint32_t bytes,unsigned world,
                          FhInstallDrawEmit,void*,unsigned *draws);
#endif
