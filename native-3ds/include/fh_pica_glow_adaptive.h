#ifndef FH_PICA_GLOW_ADAPTIVE_H
#define FH_PICA_GLOW_ADAPTIVE_H
#include <stdint.h>
#define FH_GLOW_MESH_DEFAULT_TRIANGLES 128u
#define FH_GLOW_MESH_TRIANGLES 256u
#define FH_GLOW_MESH_VERTICES 258u
#define FH_GLOW_MESH_TRAINING 4225u
typedef struct {float u,v,dx,dy;} FhGlowMeshVertex;
typedef struct {
    unsigned vertices,triangles;
    FhGlowMeshVertex vertex[FH_GLOW_MESH_VERTICES];
    uint16_t index[FH_GLOW_MESH_TRIANGLES][3];
} FhGlowMesh;
typedef int (*FhGlowMeshSample)(void *,float,float,float[2]);
/* Load-time only. Caller owns output and immutable training samples; no heap,
 * GPU calls or per-frame work. Output is unpublished until success. On any
 * callback/geometry failure counts are zero. Training excludes no alpha region.
 * Field dx/dy are original source reflection texels. The future GPU owner must
 * flush uploaded bytes and retain them until all readers finish (Melee rule).
 * Default field compilation retains 128 triangles; the larger staging
 * capacity does not increase the default resident or draw budget. */
int fh_glow_mesh_compile(FhGlowMesh *,const FhGlowMeshVertex *,unsigned,
                         unsigned,FhGlowMeshSample,void *);
/* Canonical mesh for full-texture reflections: retain BOTH source diagonals.
 * Budget must be >=4. The draw consumer must mirror geometry parameters, not
 * field offsets, when borrowing this topology for a reflected UV rectangle. */
int fh_glow_mesh_compile_shared(FhGlowMesh *,const FhGlowMeshVertex *,unsigned,
                                unsigned,FhGlowMeshSample,void *);
#endif
