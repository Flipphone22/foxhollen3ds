#include "fh_pica_specular_map.h"
#include <math.h>
void fh_pica_specular_map_prepare(FhPicaSpecularMapMesh *mesh){
    if(!mesh)return;
    for(unsigned i=0;i<17;i++)for(unsigned j=0;j<17;j++){
        float x=2.f*i/15.f-1.f,y=2.f*j/15.f-1.f;
        float square=x*x+y*y;
        FhPicaSpecularMapVertex *v=mesh->vertices+i*17+j;
        /* Source orthographic left=+1,right=-1. Keep its overdraw border
         * (>1), non-normalized corner normals and exact original grid. */
        v->position[0]=-x;v->position[1]=y;
        v->normal[0]=x;v->normal[1]=y;v->normal[2]=square<1.f?sqrtf(1.f-square):0.f;
    }
    unsigned at=0;
    for(unsigned i=0;i<16;i++)for(unsigned j=0;j<16;j++){
        unsigned a=i*17+j,b=a+17;
        mesh->indices[at++]=a;mesh->indices[at++]=b;mesh->indices[at++]=a+1;
        mesh->indices[at++]=a+1;mesh->indices[at++]=b;mesh->indices[at++]=b+1;
    }
}
