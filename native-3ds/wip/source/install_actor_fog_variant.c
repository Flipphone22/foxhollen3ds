#include "fh_install_actor_fog_material.h"
#include <stdio.h>
typedef struct {uint32_t used[2];unsigned count;} Used;
static int used_material(void*context,const FhInstallDrawSource*d){
    Used*u=context;if(d->stream||d->shader>=u->count)return 0;
    u->used[d->shader/32]|=1u<<(d->shader%32);return 1;
}
int fh_install_actor_fog_variants(FhPicaActorFogVariants*out,const void*raw,
    unsigned bytes,const FhPicaActorFogMaterial*material,unsigned count){
    FhInstallVertexSource source;
    if(!out||!material||!count||count>64||
       !fh_install_vertex_source(&source,raw,bytes,0)||count!=source.shaders_count)return 0;
    Used used={.count=count};unsigned draws=0;
    const FhInstallDescriptorEvents events={.descriptor=used_material};
    if(fh_install_descriptors_events(raw,bytes,0,&events,used_material,&used,&draws)!=1)return 0;
    return fh_pica_actor_fog_variants(out,source.joints_count,source.raw[0xfa],material,count,used.used);
}
int fh_install_actor_fog_variant_asset(FhInstallActorFogAsset*out,const void*raw,
    unsigned bytes,unsigned id,const FhPicaActorFogVariants*plan,unsigned variant,
    const FhPicaActorFogMaterial*material,unsigned materials){
    if(!out)return 0;
    FhPicaActorFogSelection selection;
    if(!fh_pica_actor_fog_variant_selection(&selection,plan,variant)){
        fh_install_actor_fog_asset_close(out);
        snprintf(out->error,sizeof(out->error),"Invalid actor fog variant slot.");return 0;
    }
    return fh_install_actor_fog_material_asset(out,raw,bytes,id,variant,material,materials,&selection);
}
