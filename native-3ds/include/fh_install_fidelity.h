#ifndef FH_INSTALL_FIDELITY_H
#define FH_INSTALL_FIDELITY_H
#include <stddef.h>
enum {FH_INSTALL_SURFACE_TRIANGLES=32768};
typedef struct {double position[3],uv[4],rgba[4];} FhInstallSurfaceVertex;
typedef struct {const FhInstallSurfaceVertex*vertex;unsigned triangles,uv_channels;} FhInstallSurface;
typedef struct {double position,uv,color,normal_cos;} FhInstallFidelityLimits;
typedef struct {
    unsigned accepted,direction,reason,samples,exact_faces;
    unsigned source_face,target_face;
    double position,uv,color;
    size_t scratch_bytes;
} FhInstallFidelity;
enum {FH_INSTALL_FIDELITY_OK,FH_INSTALL_FIDELITY_INVALID,FH_INSTALL_FIDELITY_MEMORY,
      FH_INSTALL_FIDELITY_DISTANCE,FH_INSTALL_FIDELITY_ATTRIBUTES};
/* Startup-only seven-sample, bidirectional acceptance. A BVH bounds search;
 * exact cyclic face identity includes UVs/RGBA, never reversed winding.
 * This is the existing finite-sample guard, NOT a continuous error proof.
 * Surfaces are borrowed; scratch is bounded and freed on every return.
 * 1 means accepted; 0 means rejected/error, with explicit reason. */
int fh_install_fidelity(const FhInstallSurface*,const FhInstallSurface*,const FhInstallFidelityLimits*,FhInstallFidelity*);
#endif
