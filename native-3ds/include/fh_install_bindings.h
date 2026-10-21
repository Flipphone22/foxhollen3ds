#ifndef FH_INSTALL_BINDINGS_H
#define FH_INSTALL_BINDINGS_H
#include "fh_install_bundle.h"
/* Called only by startup after task readiness validates the source disc.
 * Independently reopens the complete bundle and copies paths into static
 * storage. Failed selection retains the preceding selection. No game IO. */
int fh_install_bindings_select(const char*root,const FhInstallBundleSpec*,const char*disc);
/* Irreversible for this process, before renderer/DVD ownership begins. */
void fh_install_bindings_lock(void);
/* NULL when no installation was selected (existing/replay path owners apply).
 * kind uses FH_ASSET_MODEL/SCENE/CACHE/TEXTURE. No allocation or filesystem IO. */
const char*fh_install_bindings_asset(unsigned kind,unsigned original);
const char*fh_install_bindings_movie(void);
const char*fh_install_bindings_disc(void);
#endif
