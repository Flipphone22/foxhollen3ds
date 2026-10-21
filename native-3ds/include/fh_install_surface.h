#ifndef FH_INSTALL_SURFACE_H
#define FH_INSTALL_SURFACE_H
#include "fh_install_fidelity.h"
#include "fh_install_vertex.h"
typedef struct {FhInstallSurface surface;void*allocation;char error[112];} FhInstallSurfaceOwner;
/* Decode one original material context and either original/candidate index
 * list to bind-space triangles for the native fidelity guard. Startup-only;
 * positions/UVs/colours and bone definitions stay borrowed and unchanged.
 * Zero initialize; call close before freeing the source. */
int fh_install_surface(FhInstallSurfaceOwner*,const FhInstallVertexSource*,const FhInstallDrawSource*,const void*list,unsigned bytes);
void fh_install_surface_close(FhInstallSurfaceOwner*);
#endif
