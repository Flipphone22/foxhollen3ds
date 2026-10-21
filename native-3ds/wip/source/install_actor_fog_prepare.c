#include "fh_install_actor_fog_profile.h"
#include "fh_install_vertex.h"
#include "fh_install_descriptors.h"
#include <string.h>
typedef struct {uint32_t used[2];unsigned count;} Used;
static int collect(void*context,const FhInstallDrawSource*d){
    Used*u=context;if(d->stream||d->shader>=u->count)return 0;
    u->used[d->shader/32]|=1u<<(d->shader%32);return 1;
}
int fh_install_actor_fog_prepare(FhPicaActorFogVariants*out,
    FhPicaActorFogMaterial materials[64],unsigned*count,const void*raw,
    unsigned bytes,unsigned bump,FhActorFogTextureWidth width,void*context){
    FhInstallVertexSource source;
    if(!out||!materials||!count||bump>1||
       !fh_install_vertex_source(&source,raw,bytes,0)||
       !source.shaders_count||source.shaders_count>64)return 0;
    Used used={.count=source.shaders_count};unsigned draws=0;
    const FhInstallDescriptorEvents events={.descriptor=collect};
    if(fh_install_descriptors_events(raw,bytes,0,&events,collect,&used,&draws)!=1)return 0;
    FhPicaActorFogMaterial facts[64]={0};FhPicaActorFogVariants plan;
    for(unsigned i=0;i<used.count;i++)if(used.used[i/32]&(1u<<(i%32)))
        if(!fh_install_actor_fog_profile(facts+i,raw,bytes,i,bump,width,context))return 0;
    if(!fh_pica_actor_fog_variants(&plan,source.joints_count,source.raw[0xfa],facts,used.count,used.used))return 0;
    memcpy(materials,facts,sizeof(facts));*out=plan;*count=used.count;return 1;
}
