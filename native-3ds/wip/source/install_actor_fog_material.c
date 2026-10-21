#include "fh_install_actor_fog_material.h"
#include <stdlib.h>
#include <stdio.h>
typedef struct {
    const FhPicaActorFogMaterial *material;
    const FhPicaActorFogSelection *selection;
    unsigned count,joints,texture_matrices;
    unsigned char ready[64];
    FhPicaActorFogAlternative alternative[64];
} Materials;
const unsigned fh_install_actor_fog_material_context_bytes=sizeof(Materials);
static const FhPicaActorFogAlternative *material_select(Materials*m,unsigned shader){
    if(shader>=m->count)return NULL;
    if(!m->ready[shader]){
        if(!fh_pica_actor_fog_material_alternative(m->alternative+shader,m->joints,
            m->texture_matrices,m->material+shader,m->selection))return NULL;
        m->ready[shader]=1;
    }
    return m->alternative+shader;
}
static int route(void*context,const FhInstallDrawSource*d,FhPicaActorFogRouting*out){
    const FhPicaActorFogAlternative*a=material_select(context,d->shader);if(!a)return 0;
    *out=a->route;return 1;
}
static int coordinate(void*context,const FhInstallDrawSource*d,unsigned*out){
    const FhPicaActorFogAlternative*a=material_select(context,d->shader);if(!a)return 0;
    *out=a->coordinate;return 1;
}
int fh_install_actor_fog_material_asset(FhInstallActorFogAsset*out,const void*raw,
    unsigned bytes,unsigned id,unsigned variant,const FhPicaActorFogMaterial*material,
    unsigned materials,const FhPicaActorFogSelection*selection){
    if(!out)return 0;
    fh_install_actor_fog_asset_close(out);FhInstallVertexSource source;
    if(!material || !selection || !materials || materials>64 ||
       !fh_install_vertex_source(&source,raw,bytes,0) || materials!=source.shaders_count){
        snprintf(out->error,sizeof(out->error),"Invalid actor fog material profile.");return 0;
    }
    Materials*m=calloc(1,sizeof(*m));
    if(!m){snprintf(out->error,sizeof(out->error),"Cannot allocate actor fog material alternatives.");return 0;}
    m->material=material;m->selection=selection;m->count=materials;
    m->joints=source.joints_count;m->texture_matrices=source.raw[0xfa];
    FhPicaActorFogMatrixBank initial={0};
    if(source.fixed_world)fh_pica_actor_fog_matrix_world(&initial);
    int result=fh_install_actor_fog_asset(out,raw,bytes,id,variant,&initial,0,route,coordinate,m);
    free(m);return result;
}
