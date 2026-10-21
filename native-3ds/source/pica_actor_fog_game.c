#include "fh_pica_actor_fog_game.h"
#include "game/objects/object.h"
#include "main/model.h"
#include "main/model_light.h"
#include "main/texture.h"
#include "main/rcp_dolphin.h"

int fh_pica_actor_fog_game_material(FhPicaActorFogMaterial*out,
    const ObjModel*model,unsigned index,unsigned bump_enabled){
    if(!out || !model || !model->file || model->renderCallback ||
       !model->file->renderOps || !model->textureRefs ||
       index>=model->file->renderOpCount || bump_enabled>1)return 0;
    const Shader*op=model->file->renderOps+index;
    const ModelRenderOpTextureRefs*refs=model->textureRefs+index;
    if(op->layerCount>2 || ((op->flags&SHADER_FLAG_DECAL_LAYER) && op->layerCount<2))return 0;
    FhPicaActorFogMaterial m={0};
    m.environment=(refs->texture0!=NULL) | ((refs->texture1!=NULL)<<1);
    m.auxiliary=op->auxTexture!=NULL;
    m.bump_enabled=bump_enabled;
    m.bump_parameters=op->envMapParams;
    m.layers=op->layerCount;
    m.decal=(op->flags&SHADER_FLAG_DECAL_LAYER)!=0;
    for(unsigned i=0;i<m.layers;i++)if(op->layers[i].texture)m.textured_layers|=1u<<i;
    /* addEnvMapBumpStages consumes layer zero even if layerCount is zero.
     * Such an inconsistent source header needs explicit admission, not a guessed
     * coordinate. Ordinary relocated models have no orphan layer-zero texture. */
    if(!m.layers && op->layers[0].texture)return 0;
    if(m.environment && m.auxiliary && bump_enabled && (m.textured_layers&1)){
        const Texture*aux=textureIdxToPtr((uintptr_t)op->auxTexture);
        const Texture*base=textureIdxToPtr((uintptr_t)op->layers[0].texture);
        if(!aux || !base || !aux->width || !base->width)return 0;
        m.auxiliary_width=aux->width;
        m.base_width=base->width;
    }
    *out=m;return 1;
}

int fh_pica_actor_fog_game_selection(FhPicaActorFogSelection*out,
    const ObjModel*model,GameObject*obj,unsigned near_shadow){
    if(!out || !model || !model->file || model->renderCallback || !obj ||
       !obj->anim.modelInstance || near_shadow>1 ||
       model->file->texMtxCount>8)return 0;
    FhPicaActorFogSelection s={0};
    s.shadow_near=near_shadow;
    s.render_flags=obj->anim.modelInstance->renderFlags;
    /* Original setup only reads shadowCastSlot under flag 4, after near-shadow
     * precedence. Static objects commonly have no modelState at all. Retain
     * rejection if that source branch actually needs an absent owner. */
    if(!near_shadow && (s.render_flags&4) && !obj->anim.modelState)return 0;
    s.cast_shadow=obj->anim.modelState && obj->anim.modelState->shadowCastSlot!=NULL;
    /* The native render entry precedes objSetupLightChannels. Its globals are
     * consequently stale. Ask the existing game light owner for type 8 only;
     * do not restore diffuse selection, GX light loads or channel construction. */
    if(!(model->file->shaderFlags&2)){
        ModelLightStruct*lights[8]={0};
        s32 count=0;
        unsigned maximum=model->file->texMtxCount;
        if(maximum)modelLightStruct_selectObjectLights(obj,lights,maximum,&count,8);
        if(count<0 || (unsigned)count>maximum)return 0;
        s.selected_lights=(unsigned)count;
        if((s.render_flags&4) || near_shadow)s.selected_lights=2;
        else if(s.render_flags&0x11)s.selected_lights=1;
        if(!near_shadow && !(s.render_flags&(4|16))){
            /* Source flag 1 can force one slot even with no fresh light. The
             * original then reads a retained global pointer. Do not fabricate
             * its texture or dereference stale state; that case needs an owner. */
            if(s.selected_lights>(unsigned)count)return 0;
            for(unsigned i=0;i<s.selected_lights;i++){
                if(!lights[i])return 0;
                if(modelLightStruct_getProjectionTexture(lights[i]))s.projection_images|=1u<<i;
            }
        }
    }
    *out=s;return 1;
}
