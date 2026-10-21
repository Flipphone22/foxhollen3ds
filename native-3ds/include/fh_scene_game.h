#ifndef FH_SCENE_GAME_H
#define FH_SCENE_GAME_H
#include <dolphin/gx.h>
#include "main/model.h"
#include "main/texture.h"
#include "main/rcp_dolphin.h"
#include "fh_native_model_draw.h"
#include "fh_scene_storage.h"
#include "fh_gx_dispatch.h"
static inline unsigned fh_scene_bits(const unsigned char *data,unsigned bit,unsigned count){
    const unsigned char *p=data+(bit>>3);
    return ((unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16)>>(bit&7)&((1u<<count)-1);
}
static inline int fh_scene_material(const FhSceneSection *section,const Shader *shader,unsigned block){
    if(!shader || shader->flags!=section->shader_flags || shader->alpha!=255 ||
       (shader->alphaOverride && shader->alphaOverride!=255) || shader->layerCount!=1 ||
       shader->layers[0].materialId || (block?shader->layers[0].scrollMtx!=255:shader->layers[0].scrollMtx!=0))return 0;
    Texture *tex=block?shader->layers[0].texture:(Texture*)textureIdxToPtr((uintptr_t)shader->layers[0].texture);
    if(!tex || tex->format!=14 || tex->animationFrameCount>256 || tex->imageOffset<0 ||
       (unsigned)tex->width+((unsigned)tex->height<<16)!=section->texture_extent ||
       ((unsigned)tex->wrapS|((unsigned)tex->wrapT<<8))!=section->texture_wrap)return 0;
    unsigned bytes=(unsigned)tex->width*tex->height/2;
    /* loadedSize uses the serialized 96-byte header. TARGET_PC's GXTexObj
     * expands the live Texture header: the payload starts at sizeof(Texture).
     * Keep disk bounds and live pointer arithmetic distinct. */
    if(tex->loadedSize<96 || (unsigned)tex->imageOffset>tex->loadedSize-96 || bytes>tex->loadedSize-96-(unsigned)tex->imageOffset)return 0;
    return fh_native_scene_texture_crc((const unsigned char*)textureGetImageData(tex)+tex->imageOffset,bytes)==section->texture_crc;
}
static inline void fh_scene_surface(const FhSceneGroup *group){
    const FhTexture *page=fh_renderer_scene_page(group->page);GXTexObj tex;
    GXInitTexObj(&tex,(void*)page->pixels,512,512,GX_TF_RGB565,GX_CLAMP,GX_CLAMP,GX_FALSE);
    GXInitTexObjLOD(&tex,GX_LINEAR,GX_LINEAR,0,0,0,GX_FALSE,GX_FALSE,GX_ANISO_1);
    GXLoadTexObj(&tex,GX_TEXMAP0);
    GXSetNumTexGens(1);GXSetTexCoordGen2(GX_TEXCOORD0,GX_TG_MTX2x4,GX_TG_TEX0,GX_IDENTITY,GX_FALSE,GX_PTIDENTITY);
    GXSetNumIndStages(0);GXSetNumTevStages(1);GXSetTevDirect(GX_TEVSTAGE0);
    GXSetTevOrder(GX_TEVSTAGE0,GX_TEXCOORD0,GX_TEXMAP0,GX_COLOR0A0);GXSetTevOp(GX_TEVSTAGE0,GX_MODULATE);
    GXSetTevSwapModeTable(GX_TEV_SWAP0,GX_CH_RED,GX_CH_GREEN,GX_CH_BLUE,GX_CH_ALPHA);
    GXSetTevSwapMode(GX_TEVSTAGE0,GX_TEV_SWAP0,GX_TEV_SWAP0);
    GXSetBlendMode(GX_BM_NONE,GX_BL_ONE,GX_BL_ZERO,GX_LO_NOOP);
    GXSetZMode(GX_TRUE,GX_LEQUAL,GX_TRUE);GXSetZCompLoc(GX_TRUE);
    GXSetAlphaCompare(GX_ALWAYS,0,GX_AOP_AND,GX_ALWAYS,0);
    GXSetCullMode(group->cull?GX_CULL_BACK:GX_CULL_NONE);
}
#endif
