#include "fh_pica_glow_packed.h"
#include <math.h>
#include <stdint.h>
#include <string.h>

_Static_assert(sizeof(FhGlowMeshVertex)==16,"GPU vertex ABI");
static int separate(const void *a,size_t na,const void *b,size_t nb){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    return x<=UINTPTR_MAX-na&&y<=UINTPTR_MAX-nb&&
        (x+na<=y||y+nb<=x);
}
size_t fh_glow_mesh_packed_bytes(const FhGlowMesh *m){
    if(!m||m->vertices<3||m->vertices>FH_GLOW_MESH_VERTICES||
       !m->triangles||m->triangles>FH_GLOW_MESH_TRIANGLES)return 0;
    for(unsigned i=0;i<m->vertices;i++){
        const FhGlowMeshVertex *v=m->vertex+i;
        if(!isfinite(v->u)||!isfinite(v->v)||!isfinite(v->dx)||!isfinite(v->dy)||
           v->u<0||v->u>1||v->v<0||v->v>1)return 0;
    }
    for(unsigned i=0;i<m->triangles;i++)for(unsigned j=0;j<3;j++)
        if(m->index[i][j]>=m->vertices)return 0;
    return (m->vertices*sizeof(m->vertex[0])+m->triangles*sizeof(m->index[0])+15u)&~(size_t)15u;
}
int fh_glow_mesh_pack(void *storage,size_t capacity,const FhGlowMesh *m,FhGlowMeshView *out){
    size_t bytes=fh_glow_mesh_packed_bytes(m);
    if(!bytes||!storage||!out||((uintptr_t)storage&15u)||capacity<bytes||
       !separate(storage,bytes,m,sizeof(*m))||
       !separate(storage,bytes,out,sizeof(*out))||
       !separate(out,sizeof(*out),m,sizeof(*m)))return 0;
    unsigned char *dst=storage;
    size_t vertices=m->vertices*sizeof(m->vertex[0]);
    FhGlowMeshView view={m->vertices,m->triangles,(const FhGlowMeshVertex*)dst,
                        (const uint16_t (*)[3])(dst+vertices)};
    memset(dst,0,bytes);
    memcpy(dst,m->vertex,vertices);
    memcpy(dst+vertices,m->index,m->triangles*sizeof(m->index[0]));
    *out=view;
    return 1;
}
