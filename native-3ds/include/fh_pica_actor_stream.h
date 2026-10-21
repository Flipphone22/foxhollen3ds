#ifndef FH_PICA_ACTOR_STREAM_H
#define FH_PICA_ACTOR_STREAM_H
#include <c3d/texture.h>
#include "fh_pica_actor_command_owner.h"
#include "fh_pica_model.h"
typedef struct {
    const FhPicaActorGeometry *geometry;
    unsigned key,environment_slot,environment_on,fog,sphere,second_uv,mask_uv,texture_units;
} FhPicaActorSelection;
int fh_pica_actor_stream_geometry(const FhSceneAsset*,const FhPicaModelDraw*,FhPicaDrawDescription*);
unsigned fh_pica_actor_stream_select(const FhPicaMaterialSet*,unsigned,unsigned,FhPicaActorParameters*,FhPicaActorSelection*);
void fh_pica_actor_stream_common(const FhPicaActorSelection*);
const FhPicaSamplerCommands *fh_pica_specular_commands(unsigned);
const FhPicaSamplerCommands *fh_pica_diffuse_commands(unsigned);
/* Immutable resource descriptor, compiled by its owner immediately after init. */
static inline int fh_pica_actor_sampler_prepare(FhPicaSamplerCommands *out,const C3D_Tex *s){
    FhPicaStreamSampler input={s->border,s->dim,s->param,s->lodParam,osConvertVirtToPhys(s->data),s->fmt};
    return fh_pica_stream_sampler(out,0,&input);
}
#endif
