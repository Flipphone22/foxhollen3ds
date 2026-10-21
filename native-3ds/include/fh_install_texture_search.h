#ifndef FH_INSTALL_TEXTURE_SEARCH_H
#define FH_INSTALL_TEXTURE_SEARCH_H
#include "fh_install_texture.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Only the serial texture-stage owner starts/stops; encoder calls join their
 * borrowed worker before returning. Expanded metadata is freed before play. */
int fh_install_texture_search_start(const unsigned char source_sha[32]);
void fh_install_texture_search_stop(void);
int fh_install_texture_search_cmpr(const void*,size_t,unsigned,unsigned,unsigned,unsigned,
 void*,size_t,FhInstallTextureResult*,FhInstallTextureProgress,void*);
int fh_install_texture_search_padded(const void*,size_t,unsigned,unsigned,
 void*,size_t,FhInstallTextureResult*,FhInstallTextureProgress,void*);
#ifdef __cplusplus
}
#endif
#endif
