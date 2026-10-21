#ifndef FH_INSTALL_TEXTURE_PADDED_H
#define FH_INSTALL_TEXTURE_PADDED_H
#include "fh_install_texture.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Startup-only edge-clamped CMPR padding followed by the existing high-quality
 * encoder. Output dimensions are next powers of two, minimum8; no resizing.
 * Scratch is at most512KiB, freed before return. Error counts only visible
 * source pixels (padding cannot dilute the quality gate). Same 1/0/-1 result
 * convention as fh_install_texture_cmpr; caller initializes the codec first.
 * Not yet selected by an installation recipe. */
int fh_install_texture_padded_cmpr(const void*,size_t,unsigned,unsigned,
    void*,size_t,FhInstallTextureResult*,FhInstallTextureProgress,void*);
int fh_install_texture_padded_cmpr_modes(const void*,size_t,unsigned,unsigned,
    void*,size_t,FhInstallTextureResult*,FhInstallTextureProgress,void*,const unsigned char*,size_t);
#ifdef __cplusplus
}
#endif
#endif
