#ifndef FH_INSTALL_TRIANGLES_H
#define FH_INSTALL_TRIANGLES_H
#include <stdint.h>
/* Startup compiler only. Preserves the original ordered vertex-reference
 * packets (including normals, UVs and matrix indices); never transforms or
 * simplifies geometry. The native scene compiler owns palette/material joins. */
typedef struct {uint32_t triangles;unsigned vat;} FhInstallTriangles;
typedef int (*FhInstallTriangleEmit)(void*,const unsigned char*,const unsigned char*,const unsigned char*,unsigned);
enum {FH_INSTALL_TRI_INVALID=0,FH_INSTALL_TRI_OK=1,FH_INSTALL_TRI_CANCEL=-1,
      FH_INSTALL_TRI_NO_VAT=8,FH_INSTALL_TRI_LIST_CAP=65535,FH_INSTALL_TRI_STRIDE_CAP=32};
/* Validates the complete list before any callbacks. Empty/padding-only lists
 * have NO_VAT. Callback zero cancels; no partial result is reported as complete.
 * Caller-owned source must remain immutable through the entire call. */
int fh_install_triangles(const void*,uint32_t bytes,unsigned stride,
                         FhInstallTriangleEmit,void*,FhInstallTriangles*);
#endif
