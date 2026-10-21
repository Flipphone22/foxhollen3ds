#include "fh_install_actor_fog.h"

_Static_assert(sizeof(FhInstallActorFogRef)==16,"Fog source key has unexpected padding");
int fh_install_actor_fog_reference(const FhInstallVertexSource *source,
    const FhInstallDrawSource *draw,const FhPicaActorFogRouting *route,
    const FhPicaActorFogMatrixBank *bank,unsigned coordinate,const void *packet,
    unsigned bytes,FhInstallActorFogRef *out){
    FhInstallActorFogRef key;
    if(!source || source->world || !draw || !route || !out ||
       route->packet_prefix_bytes!=draw->layout[0] ||
       !fh_install_vertex_reference(source,draw,packet,bytes,&key.visible) ||
       !fh_pica_actor_fog_matrix_select(&key.fog,bank,route,coordinate,packet,bytes))return 0;
    *out=key;return 1;
}

typedef struct {
    FhPicaActorFogMatrixBank bank;
    FhPicaActorFogRouting route;
    FhInstallActorFogRoute resolve;
    FhInstallActorFogEmit emit;
    void *context;
    unsigned skip,has_route;
    int status;
} Compiler;
static int descriptor(void *context,const FhInstallDrawSource *draw){
    Compiler *c=context;FhPicaActorFogRouting route;
    if(!c->resolve(c->context,draw,&route) || route.texture_loads>1 ||
       route.packet_prefix_bytes!=draw->layout[0] || route.packet_prefix_bytes>9){
        c->status=0;return 0;
    }
    unsigned seen=0;
    for(unsigned i=0;i<8;i++){
        int lane=route.matrix_lane[i];
        if(lane==-1)continue;
        if(lane<1 || (unsigned)lane>=route.packet_prefix_bytes || (seen&(1u<<lane))){c->status=0;return 0;}
        seen|=1u<<lane;
    }
    /* Every direct texture-matrix byte must have exactly one attribute. */
    if(seen!=(((1u<<route.packet_prefix_bytes)-1u) & ~1u)){c->status=0;return 0;}
    c->route=route;c->has_route=1;return 1;
}
static int matrices(void *context,unsigned stream,const uint8_t *joints,unsigned count){
    Compiler *c=context;
    if(stream || !c->has_route ||
       !fh_pica_actor_fog_matrix_load(&c->bank,joints,count,c->route.texture_loads,c->skip)){
        c->status=0;return 0;
    }
    return 1;
}
static int draw(void *context,const FhInstallDrawSource *draw){
    Compiler *c=context;
    if(!c->has_route){c->status=0;return 0;}
    if(!c->emit(c->context,draw,&c->route,&c->bank)){c->status=-1;return 0;}
    return 1;
}
int fh_install_actor_fog(const void *source,uint32_t bytes,
    const FhPicaActorFogMatrixBank *initial,unsigned skip_texture_loads,
    FhInstallActorFogRoute resolve,FhInstallActorFogEmit emit,void *context,unsigned *draws){
    if(draws)*draws=0;
    if(!initial || !resolve || !emit || !draws || skip_texture_loads>1)return 0;
    /* The original vertex-animation owner preloads world position/normal
     * entries before the description stream. Validate the same symbolic keys
     * that matrix_select consumes; these are not source joint zero. Unknown
     * entries must remain zero-initialized, never an implicit identity. */
    for(unsigned i=0;i<20;i++){
        const FhPicaActorFogMatrix*k=initial->slot+i;
        if(k->kind==FH_ACTOR_FOG_UNKNOWN){if(k->joint||k->reserved)return 0;}
        else if(!fh_pica_actor_fog_matrix_valid(k,256,1))return 0;
    }
    Compiler c={.bank=*initial,.resolve=resolve,.emit=emit,.context=context,
        .skip=skip_texture_loads,.status=1};
    const FhInstallDescriptorEvents events={descriptor,matrices};
    int status=fh_install_descriptors_events(source,bytes,0,&events,draw,&c,draws);
    return status==1?1:(c.status==1?status:c.status);
}
