#ifndef FH_INSTALL_VERTEX_H
#define FH_INSTALL_VERTEX_H
#include "fh_install_descriptors.h"
#include "fh_native_model.h"
/* Validated, borrowed serialized arrays, for installation only. Source bytes
 * stay immutable until the compiler finishes. No game/GPU resource ownership. */
typedef struct {
    const unsigned char *raw,*positions,*normals,*colors,*uvs,*joints,*blends;
    unsigned bytes,world,rigid,fixed_world;
    unsigned positions_count,normals_count,colors_count,uvs_count,normal_stride;
    unsigned joints_count,blends_count,shaders_count;
} FhInstallVertexSource;
int fh_install_vertex_source(FhInstallVertexSource*,const void*,unsigned bytes,unsigned world);
/* A two-UV draw stores its second UV reference in ref.material, matching the
 * native scene wire format; the draw still owns its actual material ID. */
int fh_install_vertex_reference(const FhInstallVertexSource*,const FhInstallDrawSource*,
                                const void *packet,unsigned bytes,FhModelRef*);
int fh_install_vertex_binding(const FhInstallVertexSource*,unsigned joint,FhModelBinding*);
#endif
