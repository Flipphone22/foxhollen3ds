#ifndef FH_INSTALL_LOD_GUARDS_H
#define FH_INSTALL_LOD_GUARDS_H
#include "fh_install_skin.h"
#include "fh_install_fidelity.h"
typedef struct {
    const FhInstallVertexSource*source;
    FhInstallSkinWeights *positions,*normals;
    unsigned char*morph;
    char error[112];
} FhInstallLodGuards;
int fh_install_lod_guards_begin(FhInstallLodGuards*,const FhInstallVertexSource*);
void fh_install_lod_guards_close(FhInstallLodGuards*);
/* Unique immutable corner packets plus original triangle indices. Outputs
 * locks[vertices], required_faces[count/3]. Extra required faces are optional.
 * Protects morphs, actual CPU bone blends, matrix/texgen boundaries, alpha
 * gradients and coincident deformation seams. All outputs clear on failure. */
int fh_install_lod_locks(FhInstallLodGuards*,const FhInstallDrawSource*,const void*packets,unsigned vertices,unsigned stride,
                        const unsigned*indices,unsigned count,const unsigned char*extra_required,unsigned char*locks,unsigned char*required_faces);
/* Locks alone cannot preserve rest-pose degenerate animated triangles.
 * This separate oriented multiset guard must pass after simplification.
 * Returns1 preserved,0 rejected,-1 insufficient scratch memory. */
int fh_install_lod_preserved(const unsigned*source,unsigned count,const unsigned char*required,
                             const unsigned*candidate,unsigned candidate_count,unsigned vertices);
#endif
