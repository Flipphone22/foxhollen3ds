#include "fh_pica_world_deferred.h"
extern unsigned fh_hardware_is_new;
#include "fh_pica_height_fog.h"
#include "fh_ground_mask.h"
#include "fh_pica_shadow_project.h"
#include "fh_graphics_options.h"
#include "fh_platform.h"
#include "fh_pica_scene.h"
#include "fh_minimal_graphics.h"
#include "fh_render_floor.h"
#include "fh_surface_detail.h"
#include "fh_native_scene.h"
#include "fh_scene_game.h"
#include "main/map_block.h"
#include "main/texture.h"
#include "track/intersect_depth_state.h"
#include "track/intersect_depth_read.h"
#include "track/intersect_render_setup.h"
#include "main/lightmap.h"
#include "main/shader.h"
#include "main/debug.h"
#include "dolphin/MSL_C/PPCEABI/bare/H/math_float_helpers.h"
#include "dolphin/math.h"
#include "main/frustum.h"
#include "main/asset_load.h"
#include "game/objects/object.h"
#include "main/gameloop.h"
#include "sys/objects.h"
#include "main/mm.h"
#include "main/model_light.h"
#include "main/model.h"
#include "main/model_render_instrs.h"
#include "main/objHitReact.h"
#include "main/objhits.h"
#include "main/objtype.h"
#include "main/object_transform.h"
#include "main/vecmath.h"
#include "dolphin/mtx.h"
#include "dolphin/os/OSFastCast.h"
#include "dolphin/gx/GXCull.h"
#include "dolphin/gx/GXGeometry.h"
#include "dolphin/gx/GXLighting.h"
#include "dolphin/gx/GXBump.h"
#include "dolphin/gx/GXPixel.h"
#include "dolphin/gx/GXTev.h"
#include "dolphin/gx/GXTransform.h"
#include "main/camera.h"
#include "main/sky_state.h"
#include "main/track_dolphin.h"
#include "main/track_dolphin_shadow.h"
#include "main/newshadows_shadow.h"
#include "main/pi_dolphin.h"
#include "dolphin/mtx/vec.h"
#include "main/track_bbox.h"
#include "main/dll/player.h"
#include "main/pause_menu.h"
#include "dolphin/os/OSCache.h"
#include "main/voxmaps.h"
#include "track/intersect.h"
#include "main/rcp_dolphin.h"
#include "main/objmodel.h"
#include "main/newshadows.h"
#include "main/sky.h"
#include "main/newshadows_texture.h"
#include "main/acosf.h"
#include "main/tex_dolphin.h"
#include "dolphin/gx/GXDispList.h"
#include "track/intersect_fog.h"
#include "main/objseq.h"
#include "main/dll/FRONT/n_options.h"
#include "main/lightmap_render_queue.h"
#include "main/lightmap_internal.h"
#include "main/objprint_dolphin_internal.h"

u8 gCloudLayerOverlayColor[4] = {0x20, 0x20, 0x20, 0};
GXColor gTexShaderAmbColor = {0xFF, 0xFF, 0xFF, 0xFF};
GXColor gTexLightmapAmbColor = {0xff, 0xff, 0xff, 0xff};
s8 gTexIndMtxScaleExp = -2;

extern f32 lbl_803DEBCC;
extern const f32 lbl_803DEBFC;
extern const f32 gTexIndMtxScale;
extern f32 lbl_803DEC28;
extern int lbl_803DEBB0;
extern IndTexMtx23 gTexIndMtxTable;
#define FRUSTUM_PLANE_COUNT 5
WarpDestination gRcpPendingWarpDest;
extern GXColor gTexShaderFogColor;
extern GXColor gTexLightmapFogColor;

extern LightSortEntry gLightmapDrawQueue[];

static u8 mapBlockBounds_HasCornerPastDepthThreshold(MapBlockBoundsRec* bounds, float* xform)
{
    Vec v;
    u32 i;
    f32 fbset;
    f32 timing;

    i = 0;
    timing = gTrackPackedCoordScale;
    fbset = lbl_803DEC28;
    while (1)
    {
        {
            switch (i)
            {
            case 0:
                v.x = (f32)bounds->minX;
                v.y = (f32)bounds->minY;
                v.z = (f32)bounds->minZ;
                break;
            case 1:
                v.x = (f32)bounds->maxX;
                v.y = (f32)bounds->minY;
                v.z = (f32)bounds->minZ;
                break;
            case 2:
                v.x = (f32)bounds->minX;
                v.y = (f32)bounds->maxY;
                v.z = (f32)bounds->minZ;
                break;
            case 3:
                v.x = (f32)bounds->maxX;
                v.y = (f32)bounds->maxY;
                v.z = (f32)bounds->minZ;
                break;
            case 4:
                v.x = (f32)bounds->minX;
                v.y = (f32)bounds->minY;
                v.z = (f32)bounds->maxZ;
                break;
            case 5:
                v.x = (f32)bounds->maxX;
                v.y = (f32)bounds->minY;
                v.z = (f32)bounds->maxZ;
                break;
            case 6:
                v.x = (f32)bounds->minX;
                v.y = (f32)bounds->maxY;
                v.z = (f32)bounds->maxZ;
                break;
            case 7:
                v.x = (f32)bounds->maxX;
                v.y = (f32)bounds->maxY;
                v.z = (f32)bounds->maxZ;
                break;
            }
        }
        v.x = v.x * timing;
        v.y = v.y * timing;
        v.z = v.z * timing;
        PSMTXMultVec((MtxPtr)xform, &v, &v);
        if (v.z >= fbset)
        {
            return 1;
        }
        i = i + 1;
        if ((int)i < 8)
        {
            continue;
        }
        return 0;
    }
}

#define SHADER_FLAGS(s) ((s)->flags)

void mapBlockRender_drawLightmapIndirectPasses(struct MapBlockData* blockData, Shader* shader,
                                               ModelRenderInstrsState* state, f32 (*viewMtx)[4])
{
    f32 passMtx[3][4];
    IndTexMtx23 indMtx;
    int noiseFrameCount;
    Texture** noiseTextures;
    MapBlockBoundsRec* bounds[1];
    u8 passCount;
    u8* byteBase;
    u32 bits;
    int bitPos;
    u32 flags;
    int i;

    bitPos = state->bit;
    {
        int off = bitPos >> 3;
        byteBase = state->instrs;
        bits = byteBase[off];
        byteBase += off;
        bits = bits | (u32)(byteBase[1] << 8);
        bits = bits | (u32)(byteBase[2] << 16);
    }
    state->bit = bitPos + 8;
    /* extract this cursor's 8-bit field (LSB-first: shift out the bits already
     * consumed within the byte, then mask the width) -> bounds-record index */
    bounds[0] = &blockData->displayLists[(bits >> (bitPos & 7)) & 0xff];
    flags = SHADER_FLAGS(shader);
    if ((flags & 0x4000) != 0)
    {
        passCount = 4;
    }
    else if ((flags & 0x8000) != 0)
    {
        passCount = 8;
    }
    else if ((flags & 0x10000) != 0)
    {
        passCount = 0x10;
    }
    else
    {
        return;
    }
    i = 0;
    for (; i < passCount; i = i + 1)
    {
        PSMTXTrans(passMtx, 0.0f, 0.4f * (f32)(i + 1), 0.0f);
        PSMTXConcat(viewMtx, passMtx, passMtx);
        GXLoadPosMtxImm(passMtx, GX_PNMTX0);
        indMtx = gTexIndMtxTable;
        getNewShadowNoiseTextureFrames(&noiseTextures, &noiseFrameCount);
        selectTexture(noiseTextures[(u8)i], 1);
        {
            f32 s = (f32)((i & 0xff) + 1) * gTexIndMtxScale;
            indMtx.m[0][0] = s * lbl_803DEBFC;
        }
        indMtx.m[1][1] = indMtx.m[0][0];
        GXSetIndTexMtx(GX_ITM_0, indMtx.m, gTexIndMtxScaleExp);
        GXCallDisplayList(bounds[0]->dlist, bounds[0]->dlistSize);
    }
}

Shader* mapBlockRender_setLightmapShader(struct MapBlockData* blockData, ModelRenderInstrsState* state)
{
    Shader* shader;
    u32 shaderIdx;
    u8* byteBase;
    GXColor fogColor;
    u32 bits;
    u32 bitPos;
    u8 ambColor[3];

    fogColor = gTexLightmapFogColor;
    bitPos = state->bit;
    {
        int off = (int)bitPos >> 3;
        byteBase = state->instrs;
        bits = byteBase[off];
        byteBase += off;
        bits |= (u32)byteBase[1] << 8;
        bits |= (u32)byteBase[2] << 16;
        state->bit = bitPos + 6;
        shaderIdx = (bits >> (bitPos & 7)) & 0x3f;
        shader = &blockData->shaders[shaderIdx];
    }
    GXSetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
    selectTexture(((ShaderLayer*)Shader_getLayer(shader, 0))->texture, 0);
    if ((SHADER_FLAGS(shader) & 4) != 0)
    {
        _gxSetFogParams();
    }
    else
    {
        GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, fogColor);
    }
    if ((SHADER_FLAGS(shader) & 1) != 0 || (SHADER_FLAGS(shader) & 0x40000) != 0 ||
        (SHADER_FLAGS(shader) & 0x800) != 0 || (SHADER_FLAGS(shader) & 0x1000) != 0)
    {
        GXSetChanAmbColor(GX_COLOR0, gTexLightmapAmbColor);
        if ((SHADER_FLAGS(shader) & 0x40000) != 0)
        {
            GXSetChanCtrl(GX_COLOR0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }
        else
        {
            GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }
    }
    else
    {
        objGetSunColor(0, &ambColor[0], &ambColor[1], &ambColor[2]);
        GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanAmbColor(GX_COLOR0, *(GXColor*)&ambColor[0]);
    }
    return shader;
}

void mapBlockRender_drawDimmedAabbLights(MapBlockBoundsRec* bounds, MapBlockData* block, float* viewMtx)
{
    ModelLightStruct** lightPtr;
    f32 posZ;
    f32 posY;
    f32 posX;
    int lightCount;
    u8 colorA;
    u8 colorB;
    u8 colorG;
    u8 colorR;

    {
        f32 fz = *(f32*)&playerMapOffsetZ;
        f32 fldZ = block->transform[2][3];
        f32 fldY = block->transform[1][3];
        f32 fx = *(f32*)&playerMapOffsetX;
        f32 fldX = block->transform[0][3];
        f32 ax0 = (f32)(bounds->minX >> 3) + fldX;
        f32 az0 = (f32)(bounds->minZ >> 3) + fldZ;
        f32 ax1 = (f32)(bounds->maxX >> 3) + fldX;
        f32 az1 = (f32)(bounds->maxZ >> 3) + fldZ;
        modelLightStruct_selectBrightestAabbLights(ax0 + fx, (f32)(bounds->minY >> 3) + fldY, az0 + fz, ax1 + fx,
                                                   (f32)(bounds->maxY >> 3) + fldY, az1 + fz,
                                                   gTexDimmedLightList, 2, &lightCount);
    }
    Rcp_ResetTextureStageState();
    setupCausticBaseTevStages(viewMtx);
    {
        u8* pColorA;
        u8* pColorB;
        u8* pColorG;
        f32* pPosZ;
        f32* pPosY;
        int i;

        i = 0;
        lightPtr = gTexDimmedLightList;
        pColorA = &colorA;
        pColorB = &colorB;
        pColorG = &colorG;
        pPosZ = &posZ;
        pPosY = &posY;
        for (; i < lightCount; lightPtr = lightPtr + 1, i = i + 1)
        {
            modelLightStruct_getDiffuseColor(*lightPtr, &colorR, pColorG, pColorB, pColorA);
            colorR = ((int)colorR >> 1) + ((int)colorR >> 2);
            colorG = ((int)colorG >> 1) + ((int)colorG >> 2);
            colorB = ((int)colorB >> 1) + ((int)colorB >> 2);
            modelLightStruct_getPosition(*lightPtr, &posX, pPosY, pPosZ);
            addPointLightDirectStages(modelLightStruct_getRadius(*lightPtr), (int*)&colorR, &posX);
        }
    }
    Rcp_ApplyTextureStageCounts();
    GXSetNumChans(1);
    GXSetCullMode(GX_CULL_BACK);
    gxSetZMode_(1, GX_LEQUAL, 0);
    gxSetPeControl_ZCompLoc_(1);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    return;
}

u32 frustumTestAabbWithPlaneOffsets(f32 minX, f32 maxX, f32 minY, f32 maxY, f32 minZ, f32 maxZ, f32* planeOffsets)
{
    FrustumPlane* plane;
    int cornerIndex;
    int i;
    float nearX;
    float nearY;
    float nearZ;
    float farX;
    float farY;
    float farZ;

    plane = gViewFrustumPlanes;
    for (i = 0; i < FRUSTUM_PLANE_COUNT; i++)
    {
        cornerIndex = plane[i].aabbCornerIndex;
        if ((cornerIndex & 1) != 0)
        {
            nearX = maxX;
            farX = minX;
        }
        else
        {
            nearX = minX;
            farX = maxX;
        }
        if ((cornerIndex & 2) != 0)
        {
            nearY = maxY;
            farY = minY;
        }
        else
        {
            nearY = minY;
            farY = maxY;
        }
        if ((cornerIndex & 4) != 0)
        {
            nearZ = maxZ;
            farZ = minZ;
        }
        else
        {
            nearZ = minZ;
            farZ = maxZ;
        }
        if ((nearX * plane[i].normalX + nearY * plane[i].normalY + nearZ * plane[i].normalZ + plane[i].distance +
                 planeOffsets[i] <
             0.0f) &&
            (farX * plane[i].normalX + farY * plane[i].normalY + farZ * plane[i].normalZ + plane[i].distance +
                 planeOffsets[i] <
             0.0f))
            return 0;
    }
    return 1;
}

static u8 mapBlockBounds_ComputeAndTestPlanes(MapBlockBoundsRec* bounds, struct MapBlockData* block,
                                              FrustumPlane* planes, int planeCount, f32* minX, f32* minY, f32* minZ,
                                              f32* maxX, f32* maxY, f32* maxZ)
{
    u8 cornerIndex;
    float nearX;
    float nearY;
    float nearZ;
    float farX;
    float farY;
    float farZ;
    int i;
    *maxX = (f32)(bounds->maxX >> 3) + block->transform[0][3];
    *minX = (f32)(bounds->minX >> 3) + block->transform[0][3];
    *maxY = (f32)(bounds->maxY >> 3) + block->transform[1][3];
    *minY = (f32)(bounds->minY >> 3) + block->transform[1][3];
    *maxZ = (f32)(bounds->maxZ >> 3) + block->transform[2][3];
    *minZ = (f32)(bounds->minZ >> 3) + block->transform[2][3];
    for (i = 0; i < planeCount; i = i + 1)
    {
        cornerIndex = planes->aabbCornerIndex;
        if ((cornerIndex & 1) != 0)
        {
            nearX = *maxX;
            farX = *minX;
        }
        else
        {
            nearX = *minX;
            farX = *maxX;
        }
        if ((cornerIndex & 2) != 0)
        {
            nearY = *maxY;
            farY = *minY;
        }
        else
        {
            nearY = *minY;
            farY = *maxY;
        }
        if ((cornerIndex & 4) != 0)
        {
            nearZ = *maxZ;
            farZ = *minZ;
        }
        else
        {
            nearZ = *minZ;
            farZ = *maxZ;
        }
        if ((planes->distance + (nearX * planes->normalX + nearY * planes->normalY + nearZ * planes->normalZ) <
             0.0f) &&
            (planes->distance + (farX * planes->normalX + farY * planes->normalY + farZ * planes->normalZ) <
             0.0f))
        {
            return 0;
        }
        planes++;
    }
    return 1;
}


static const FhTexture* fh_minimal_base(Shader* shader,FhTexture* out) {
    if (!shader || !shader->layerCount) return NULL;
    /* MapBlock_initShaders already resolves layers to real Texture pointers.
     * Model layers use texture handles. ARM pointers are below INT32_MAX;
     * feeding a world pointer through textureIdxToPtr discards every image. */
    Texture* texture=shader->layers[0].texture;
    return texture && fh_gx_texture_view(textureGetGXTexObj(texture),out) ? out : NULL;
}
static const FhSceneAsset* fh_minimal_world_prepare(MapBlockData* block) {
    const FhSceneAsset* asset=fh_native_scene_find(block);
    if (!asset) { fh_minimal_missing++; fh_minimal_coverage_failure(16,0,0); return NULL; }
    unsigned failure=0;
    if (!asset->block) failure=1;
    else if (!fh_native_scene_prepare_world_positions(asset,block->vertices,block->vertexCount)) failure=2;
    else if (!fh_native_scene_source(asset,2,block->vertexColors,block->colorCount*2u)) failure=3;
    else if (!fh_native_scene_source(asset,3,block->vertexTexCoords,block->texCoordCount*4u)) failure=4;
    else if (!fh_native_scene_source(asset,4,block->renderInstrsMain,block->nRenderInstrsMain)) failure=5;
    if (failure) {fh_minimal_rejected++;fh_minimal_coverage_failure(17,asset->id,failure);return NULL;}
    return asset;
}
static Texture* fh_minimal_world_texture(ShaderLayer* layer,float scroll[2]) {
    Texture* texture=layer->texture;
    if (layer->materialId) {
        for (unsigned i=0;i<0x50;i++) {
            MapTextureOverride* entry=gMapTextureOverrides+i;
            if (entry->refCount>0 && entry->texture==texture && entry->type==layer->materialId) {
                texture=textureGetAnimationFrame(texture,entry->frame);break;
            }
        }
    }
    if (layer->scrollMtx!=0xff) {
        scroll[0]=gMapTextureScrolls[layer->scrollMtx].offsetX/1048576.0f;
        scroll[1]=gMapTextureScrolls[layer->scrollMtx].offsetY/1048576.0f;
    }
    return texture;
}
static FhMinimalWorldFalloff fh_minimal_world_falloff(void) {
    FhMinimalWorldFalloff out={0};
    Texture* radial=getNewShadowFalloffTexture();
    Texture* ramp=getNewShadowInverseRampTexture();
    GameObject* player=Obj_GetPlayerObject();
    out.distance=player?Camera_DistanceToCurrentViewPosition(player->anim.worldPosX,
        player->anim.worldPosY,player->anim.worldPosZ):100.f;
    if (!radial || !ramp || !fh_gx_texture_view(textureGetGXTexObj(radial),out.texture) ||
        !fh_gx_texture_view(textureGetGXTexObj(ramp),out.texture+1)) {
        fh_minimal_rejected++;return out;
    }
    out.active=1;return out;
}
/* Static world surface plus live texture animation/scroll and ambient colour.
 * Dynamic lights and deferred falloff remain a separate reconstruction step. */
static int fh_budget_material(Shader* shader,FhMinimalWorldMaterial* material) {
    material->layered=shader->layerCount==2 && (shader->layers[1].typeBits&127)==9;
    memcpy(material->konst,&lbl_803DEBB0,4);
    for(unsigned i=0;i<(material->layered?2u:(shader->layerCount?1u:0u));i++) {
        Texture* texture=fh_minimal_world_texture(shader->layers+i,material->scroll[i]);
        if(texture && !fh_gx_texture_view(textureGetGXTexObj(texture),material->texture+i))return 0;
    }
    if(!(shader->flags&0x40000)) {
        unsigned char ambient[4];
        if(shader->flags&(1|0x800|0x1000))memcpy(ambient,&gTexShaderAmbColor,4);
        else objGetSunColor(0,ambient,ambient+1,ambient+2);
        for(unsigned i=0;i<3;i++)material->raster[i]=ambient[i]/255.f;
    }
    return 1;
}
/* Deliberately incomplete reconstruction tier: callback-free main world.
 * Reuse the existing fenced native storage and PICA world submission. */
static void fh_pica_live_materials(const FhSceneAsset*,MapBlockData*);
static void fh_budget_world(MapBlockData* block) {
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_WORLD))return;
    if(fh_pica_scene_enabled) {
        const FhSceneAsset* resident=fh_native_scene_find(block);
        if(!resident || !fh_pica_world_prepare_positions(resident)) {
            fh_minimal_coverage_failure(27,0,0);return;
        }
        float camera[3][4];unsigned clip[4];
        PSMTXConcat((MtxPtr)Camera_GetViewMatrix(),block->transform,camera);
        GXGetScissor(clip,clip+1,clip+2,clip+3);
        fh_pica_live_materials(resident,block);
        fh_pica_shadow_project_transform(resident,(const float(*)[4])block->transform);
        fh_pica_scene_world(resident,camera,(const float(*)[4])Camera_GetProjectionMatrix(),clip);return;
    }
    const FhSceneAsset* asset=fh_minimal_world_prepare(block);
    if (!asset) return;
    const FhWorldDrawPlan* plan=fh_native_scene_world_plan(asset);
    if (!plan) {fh_minimal_coverage_failure(22,asset->id,0);return;}
    float view[3][4];
    PSMTXConcat((MtxPtr)Camera_GetViewMatrix(),block->transform,view);
    unsigned clip[4];GXGetScissor(clip,clip+1,clip+2,clip+3);
    fh_minimal_movie_active=0;
    fh_minimal_world_batch_begin();
    FhSceneGroup pending={0};
    FhMinimalWorldMaterial material={.raster={1,1,1,1}};
    Shader* pending_shader=NULL;
    unsigned pending_valid=0;
    for (unsigned i=0;i<plan->commands;i++) {
        const FhWorldPlanOp* op=plan->command+i;
        if (op->kind!=2) continue;
        Shader* shader=block->shaders+op->shader;
        if (shader->flags&(0x80000000u|2u)) continue;
        MapBlockBoundsRec* list=block->displayLists+op->value;
        float minX,minY,minZ,maxX,maxY,maxZ;
        if (!mapBlockBounds_ComputeAndTestPlanes(list,block,gViewFrustumPlanes,
            FRUSTUM_PLANE_COUNT,&minX,&minY,&minZ,&maxX,&maxY,&maxZ)) continue;
        /* Preserve the original main-pass (visArg=0) visibility decision.
         * Deferred shadow/water passes are intentionally absent in this tier. */
        if (shader->flags&0x2000u) {
            if (shader->flags&0x40000000u) continue;
            if (mapBlockBounds_ComputeAndTestPlanes(list,block,gPlayerRelativeFrustumPlanes,
                FRUSTUM_PLANE_COUNT,&minX,&minY,&minZ,&maxX,&maxY,&maxZ)) continue;
        }
        if (!fh_native_scene_source(asset,5+op->value,list->dlist,list->dlistSize)) {
            fh_minimal_coverage_failure(23,asset->id,op->value);continue;
        }
        FhSceneGroup geometry=plan->draw[op->ordinal];
        if(!fh_render_budget_textures)geometry.two_uv=0; /* Same vertex stride. */
        /* Flat stage has no lights/texture/falloff. Only retain flags read by
         * native raster setup. Join adjacent slices of the same resident group;
         * no reordering, hidden ranges, geometry copies or ownership changes. */
        unsigned flags=shader->flags&(0x60000000u|0x480u);
        if (pending_valid && (!fh_render_budget_textures || pending_shader==shader) &&
            material.flags==flags && pending.vertex==geometry.vertex &&
            pending.index+pending.indices==geometry.index && pending.cull==geometry.cull &&
            pending.format==geometry.format && pending.vertices==geometry.vertices) {
            pending.indices+=geometry.indices;pending.draws+=geometry.draws;
        } else {
            if(pending_valid)fh_minimal_world_group(asset,&pending,view,&material,clip);
            pending_valid=0;
            material=(FhMinimalWorldMaterial){.raster={1,1,1,1},.flags=flags};
            if(fh_render_budget_textures && (!fh_budget_material(shader,&material) ||
                material.layered!=geometry.two_uv)) {
                fh_minimal_coverage_failure(24,asset->id,op->shader);continue;
            }
            pending=geometry;pending_shader=shader;pending_valid=1;
        }
    }
    if(pending_valid)fh_minimal_world_group(asset,&pending,view,&material,clip);
    fh_minimal_world_batch_end();
}
static void fh_minimal_world_draw(const FhSceneAsset* asset,MapBlockData* block,
    unsigned ordinal,unsigned list_index,Shader* shader,const float view[3][4],float near_z,unsigned water,unsigned foreground,const FhMinimalWorldLights* lights,const FhMinimalWorldFalloff* falloff,const FhSceneGroup* planned) {
    if (!asset) return;
    FhSceneGroup part;
    unsigned section_list=list_index,section_shader=shader-block->shaders;
    if (planned) part=*planned;
    else {
        unsigned gi;
        if (!fh_native_scene_draw(asset,ordinal,list_index,section_shader,&gi)) {fh_minimal_rejected++;fh_minimal_coverage_failure(18,asset->id,ordinal);return;}
        if (gi==65535) {fh_minimal_missing++;fh_minimal_coverage_failure(19,asset->id,ordinal);return;}
        FhSceneGroup group;FhSceneSection section;
        fh_native_scene_group(asset,gi,&group);
        fh_native_scene_section(asset,group.first_section+ordinal-group.first,&section);
        section_list=section.list;section_shader=section.shader;
        part=group;part.index+=section.first;part.indices=section.count;part.draws=1;
    }
    if (section_list>=block->displayListCount || section_shader>=block->shaderCount) {fh_minimal_rejected++;return;}
    MapBlockBoundsRec* list=block->displayLists+section_list;
    if (!fh_native_scene_source(asset,5+section_list,list->dlist,list->dlistSize)) {fh_minimal_rejected++;fh_minimal_coverage_failure(20,asset->id,section_list);return;}
            unsigned scissor[4];GXGetScissor(scissor,scissor+1,scissor+2,scissor+3);
            if (water) { fh_minimal_water_group(asset,&part,view,scissor);return; }
            if (fh_minimal_movie_active) {
                const float pose[16]={1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
                fh_minimal_group(asset,&part,pose,1,view,1,NULL,NULL,0,shader->flags,0,NULL,scissor,NULL,NULL);
                return;
            }
            FhMinimalWorldMaterial material={.flags=shader->flags,.raster={1,1,1,1},.near_z=near_z};
            material.lighting=*lights;material.falloff=*falloff;
            if (foreground && (shader->flags&0x2000)) material.flags|=0x40000000u;
            material.layered=shader->layerCount==2 && (shader->layers[1].typeBits&127)==9;
            if (material.layered!=part.two_uv) { fh_minimal_rejected++;fh_minimal_coverage_failure(21,asset->id,section_shader);return; }
            memcpy(material.konst,&lbl_803DEBB0,4);
            for (unsigned i=0;i<(material.layered?2u:(shader->layerCount?1u:0u));i++) {
                Texture* texture=fh_minimal_world_texture(shader->layers+i,material.scroll[i]);
                if (texture && !fh_gx_texture_view(textureGetGXTexObj(texture),material.texture+i)) {
                    fh_minimal_rejected++;return;
                }
            }
            if (!(shader->flags&0x40000)) {
                unsigned char ambient[4];
                if (shader->flags&(1|0x800|0x1000)) memcpy(ambient,&gTexShaderAmbColor,4);
                else objGetSunColor(0,ambient,ambient+1,ambient+2);
                for (unsigned i=0;i<3;i++) material.raster[i]=ambient[i]/255.f;
            }
            fh_minimal_world_group(asset,&part,view,&material,scissor);
}

static void fh_mapBlockRender_callList(u8 passSelect, u32 visArg, MapBlockData* block, Shader* shader,
                             ModelRenderInstrsState* state, float* mtx,
                             const FhSceneAsset* native_asset, unsigned native_ordinal,const FhWorldPlanOp* planned_op,const FhSceneGroup* planned_group)
{
    FhMinimalWorldLights native_lights={0};
    FhMinimalWorldFalloff native_falloff={0};
    int lightPos[3];
    int count;
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
    u8 lightColor[4];
    GXColor chanColor;
    int i;
    u32 visible;
    u32 flags;
    u32 bits;
    int bitPos;
    u8* byteBase;

    {
        LightSortEntry* texGlobals;
        MapBlockBoundsRec* bounds[1];

        texGlobals = gLightmapDrawQueue;
        bitPos = state->bit;
        if (planned_op) {
            state->bit=bitPos+8;
            bounds[0]=block->displayLists+planned_op->value;
        } else {
        {
            int off = bitPos >> 3;
            byteBase = state->instrs;
            bits = byteBase[off];
            byteBase += off;
            bits = bits | (u32)(byteBase[1] << 8);
            bits = bits | (u32)(byteBase[2] << 16);
        }
        state->bit = bitPos + 8;
        bounds[0] = &block->displayLists[(bits >> (bitPos & 7)) & 0xff];
        if (fh_render_budget_strip) return;
        }
        if ((shader != NULL) && ((SHADER_FLAGS(shader) & 2) != 0))
        {
            return;
        }
        if (mapBlockBounds_ComputeAndTestPlanes(bounds[0], block, gViewFrustumPlanes,
                                                FRUSTUM_PLANE_COUNT, &minX, &minY, &minZ, &maxX, &maxY, &maxZ) == 0)
        {
            return;
        }
        if (passSelect == 0)
        {
            flags = SHADER_FLAGS(shader);
            if ((flags & 0x80000000) != 0)
            {
                int shadowType;

                lightmapQueueShadowRow(bounds[0], block, bounds[0]->selector);
                shadowType = 5;
                texGlobals[gLightmapDrawQueueCount].d = shadowType;
                gLightmapDrawQueueCount = gLightmapDrawQueueCount + 1;
            }
            else if (((flags & 0x40000000) != 0) || ((flags & 0x2000) != 0))
            {
                int shadowType;

                lightmapQueueShadowRow(bounds[0], block, bounds[0]->selector);
                shadowType = 4;
                texGlobals[gLightmapDrawQueueCount].d = shadowType;
                gLightmapDrawQueueCount = gLightmapDrawQueueCount + 1;
            }
        }
        else
        {
            if (shader != NULL)
            {
                flags = SHADER_FLAGS(shader);
                if (((flags & 0x80000000) == 0) && ((flags & 0x20000) == 0))
                {
                    if ((shader != NULL) && ((flags & 0x80000) != 0))
                    {
                        count = 0;
                    }
                    else
                    {
                        modelLightStruct_selectBrightestAabbLights(
                            minX + playerMapOffsetX, minY, minZ + playerMapOffsetZ, maxX + playerMapOffsetX, maxY,
                            maxZ + playerMapOffsetZ, gTexBlockLightList, 2, &count);
                    }
                    if (fh_minimal_enabled) {
                        native_lights.count=count;
                        native_lights.mode=(flags&0x800)?1:(flags&0x1000)?2:0;
                        if (native_lights.mode) ObjSeq_copyDefaultColor(&chanColor);
                        for (i=0;i<count;i++) {
                            FhMinimalWorldLight* light=native_lights.light+i;
                            modelLightStruct_getDiffuseColor(gTexBlockLightList[i],light->color,
                                light->color+1,light->color+2,light->color+3);
                            modelLightStruct_getPosition(gTexBlockLightList[i],light->position,
                                light->position+1,light->position+2);
                            light->radius=modelLightStruct_getRadius(gTexBlockLightList[i]);
                        }
                    } else {
                    if ((shader != NULL) &&
                        (((SHADER_FLAGS(shader) & 0x800) != 0 || ((SHADER_FLAGS(shader) & 0x1000) != 0))))
                    {
                        ObjSeq_copyDefaultColor(&chanColor);
                        chanColor.a = 0;
                        chanColor.b = 0;
                        chanColor.g = 0;
                        chanColor.r = 0;
                        if (count == 0)
                        {
                            if ((shader != NULL) && ((SHADER_FLAGS(shader) & 0x800) != 0))
                            {
                                addLightColorModulateStage((int*)&chanColor);
                            }
                            else
                            {
                                addVertexAlphaDimStage((u8*)&chanColor);
                            }
                        }
                        else
                        {
                            modelLightStruct_getDiffuseColor(gTexBlockLightList[0], &lightColor[0], &lightColor[1],
                                                             &lightColor[2], &lightColor[3]);
                            modelLightStruct_getPosition(gTexBlockLightList[0], (f32*)&lightPos[0],
                                                         (f32*)&lightPos[1], (f32*)&lightPos[2]);
                            addFirstPointLightStages(modelLightStruct_getRadius(gTexBlockLightList[0]),
                                        (int*)lightColor, (f32*)&lightPos[0], (u8*)&chanColor);
                            for (i = 1; i < count; i = i + 1)
                            {
                                modelLightStruct_getDiffuseColor(gTexBlockLightList[i], &lightColor[0],
                                                                 &lightColor[1], &lightColor[2], &lightColor[3]);
                                modelLightStruct_getPosition(gTexBlockLightList[i], (f32*)&lightPos[0],
                                                             (f32*)&lightPos[1], (f32*)&lightPos[2]);
                                addPointLightAccumStages(modelLightStruct_getRadius(gTexBlockLightList[i]),
                                            (int*)lightColor, (f32*)&lightPos[0]);
                            }
                            if ((shader != NULL) && ((SHADER_FLAGS(shader) & 0x800) != 0))
                            {
                                addAccumulatedLightModulateStage();
                            }
                            else
                            {
                                addAccumulatedLightBlendStages();
                            }
                        }
                    }
                    else
                    {
                        for (i = 0; i < count; i = i + 1)
                        {
                            modelLightStruct_getDiffuseColor(gTexBlockLightList[i], &lightColor[0],
                                                             &lightColor[1], &lightColor[2], &lightColor[3]);
                            modelLightStruct_getPosition(gTexBlockLightList[i], (f32*)&lightPos[0],
                                                         (f32*)&lightPos[1], (f32*)&lightPos[2]);
                            addPointLightDirectStages(modelLightStruct_getRadius(gTexBlockLightList[i]),
                                        (int*)lightColor, (f32*)&lightPos[0]);
                        }
                    }
                    }
                    if ((shader != NULL) && ((SHADER_FLAGS(shader) & 0x2000) != 0))
                    {
                        if ((shader != NULL) && ((SHADER_FLAGS(shader) & 0x40000000) != 0))
                        {
                            visible = visArg;
                        }
                        else
                        {
                            u8 mirrorVisible = mapBlockBounds_ComputeAndTestPlanes(
                                bounds[0], block, gPlayerRelativeFrustumPlanes, FRUSTUM_PLANE_COUNT, &minX, &minY,
                                &minZ, &maxX, &maxY, &maxZ);
                            if ((mirrorVisible != 0 && (u8)visArg != 0) || (mirrorVisible == 0 && (u8)visArg == 0))
                            {
                                visible = 1;
                            }
                            else
                            {
                                visible = 0;
                            }
                            if ((u8)visArg != 0)
                            {
                                GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
                                gxSetZMode_(1, GX_LEQUAL, 0);
                                gxSetPeControl_ZCompLoc_(1);
                                GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
                            }
                        }
                        if ((u8)visible == 0)
                        {
                            return;
                        }
                        if (fh_minimal_enabled) native_falloff=fh_minimal_world_falloff();
                        else addShadowFalloffTevStages();
                    }
                    Rcp_ApplyTextureStageCounts();
                }
            }
        if (fh_minimal_enabled) {
            unsigned stream=state->instrs==block->renderInstrsTransp?1:
                state->instrs==block->renderInstrsWater?2:0;
            const FhSceneAsset* asset=native_asset;
            unsigned ordinal,list=bounds[0]-block->displayLists;
            /* Main and queued draws share the original visibility boundary.
             * The queued draw selects by its exact original stream bit,
             * not by list identity: repeated lists can use different layouts. */
            if (asset && stream==0) {
                fh_minimal_world_draw(asset,block,native_ordinal,list,shader,(const float(*)[4])mtx,0,0,visArg,&native_lights,&native_falloff,planned_group);
            } else {
                asset=fh_minimal_world_prepare(block);
                if (asset && (stream==0 || fh_native_scene_source(asset,5+block->displayListCount+(stream==2),
                    stream==2?block->renderInstrsWater:block->renderInstrsTransp,
                    stream==2?block->nRenderInstrsWater:block->nRenderInstrsTransp)) &&
                fh_native_scene_stream_draw(asset,stream,bitPos,list,shader-block->shaders,&ordinal))
                    fh_minimal_world_draw(asset,block,ordinal,list,shader,(const float(*)[4])mtx,stream==1?-.01f:0,stream==2,visArg,&native_lights,&native_falloff,NULL);
                else fh_minimal_missing++;
            }
        }
            GXCallDisplayList(bounds[0]->dlist, bounds[0]->dlistSize);
            flags = SHADER_FLAGS(shader);
            if ((((flags & 0x4000) != 0) || ((flags & 0x8000) != 0) || ((flags & 0x10000) != 0)) &&
                (mapBlockBounds_HasCornerPastDepthThreshold(bounds[0], mtx) != 0))
            {
                int shadowType;

                lightmapQueueShadowRow(bounds[0], block, 0x17);
                shadowType = 6;
                texGlobals[gLightmapDrawQueueCount].d = shadowType;
                gLightmapDrawQueueCount = gLightmapDrawQueueCount + 1;
            }
        }
    }
}
void mapBlockRender_callList(u8 passSelect, u32 visArg, MapBlockData* block, Shader* shader,
                             ModelRenderInstrsState* state, float* mtx) {
    fh_mapBlockRender_callList(passSelect,visArg,block,shader,state,mtx,NULL,0,NULL,NULL);
}


static void mapBlockRender_setupShaderTextures(Shader* shader, int mode)
{
    /* Only an ordinary opaque multiplicative detail layer is removed. The
     * original base layer still resolves animation/scrolls and its vertex
     * lighting. Masks, alpha blends, projected textures, caustics and decals
     * use the original constructor; geometry, visibility and draw order stay. */
    if (fh_surface_detail && shader->layerCount == 2 &&
        (shader->layers[1].typeBits & 0x7f) == 6 &&
        !(shader->flags & (0xe0000000u | 0x80 | 0x100 | 0x400 | 0x20000 | 0x100000))) {
        Shader base = *shader;base.layerCount = 1;
        mapBlockRender_setupShaderTextures(&base, mode);
        fh_surface_detail_world++;return;
    }

    int layerIdx;
    ShaderLayer* layer;
    Texture* texture;
    f32 (*texMtx)[4];
    int overrideIdx;
    int remain;
    MapTextureOverride* overrideEntry;
    u8 layerByte;
    u32 kColor;
    f32 tx;
    f32 texMatrix[3][4];

    kColor = lbl_803DEBB0;
    if ((shader->layerCount == 2) &&
        (texture = (Texture*)Shader_getLayer(shader, 1),
         (((ShaderLayer*)texture)->typeBits & 0x7f) == 9u))
    {
        layer = Shader_getLayer(shader, 0);
        {
            u8 overrideType;
            if ((overrideType = layer->materialId) != '\0')
            {
                Texture* layerTextureId = layer->texture;
                MapTextureOverride* overrides;
                overrideIdx = 0;
                overrides = gMapTextureOverrides;
                overrideEntry = overrides;
                for (remain = 0x50; remain != 0 || (texture = layerTextureId, 0); remain--)
                {
                    if (((overrideEntry->refCount > 0) &&
                         (overrideEntry->texture == layerTextureId)) &&
                        ((int)overrideType == overrideEntry->type))
                    {
                        texture = textureGetAnimationFrame(layerTextureId,
                                                           overrides[overrideIdx].frame);
                        break;
                    }
                    overrideEntry = overrideEntry + 1;
                    overrideIdx = overrideIdx + 1;
                }
            }
            else
            {
                texture = layer->texture;
            }
        }
        if (layer->scrollMtx != 0xff)
        {
            tx = gMapTextureScrolls[layer->scrollMtx].offsetX / 1048576.0f;
            PSMTXTrans(texMatrix, tx,
                       gMapTextureScrolls[layer->scrollMtx].offsetY /
                           1048576.0f,
                       0.0f);
            texMtx = texMatrix;
        }
        else
        {
            texMtx = NULL;
        }
        addTexLayerStageKColor(texture, texMtx, 0, (GXColor*)&kColor);
        if ((SHADER_FLAGS(shader) & 0x100) != 0)
        {
            addSmallReflectionTevStage();
        }
        layer = Shader_getLayer(shader, 1);
        {
            u8 overrideType;
            if ((overrideType = layer->materialId) != '\0')
            {
                Texture* layerTextureId = layer->texture;
                MapTextureOverride* overrides;
                overrideIdx = 0;
                overrides = gMapTextureOverrides;
                overrideEntry = overrides;
                for (remain = 0x50; remain != 0 || (texture = layerTextureId, 0); remain--)
                {
                    if (((overrideEntry->refCount > 0) &&
                         (overrideEntry->texture == layerTextureId)) &&
                        ((int)overrideType == overrideEntry->type))
                    {
                        texture = textureGetAnimationFrame(layerTextureId,
                                                           overrides[overrideIdx].frame);
                        break;
                    }
                    overrideEntry = overrideEntry + 1;
                    overrideIdx = overrideIdx + 1;
                }
            }
            else
            {
                texture = layer->texture;
            }
        }
        if (layer->scrollMtx != 0xff)
        {
            tx = gMapTextureScrolls[layer->scrollMtx].offsetX / 1048576.0f;
            PSMTXTrans(texMatrix, tx,
                       gMapTextureScrolls[layer->scrollMtx].offsetY /
                           1048576.0f,
                       0.0f);
            texMtx = texMatrix;
        }
        else
        {
            texMtx = NULL;
        }
        addTexLayerStage(texture, texMtx, 9);
        addVertexColorKAlphaStage((GXColor*)&kColor);
    }
    else
    {
        for (layerIdx = 0; layerIdx < (int)(u32)shader->layerCount; layerIdx = layerIdx + 1)
        {
            Texture* layerTextureId;
            layer = Shader_getLayer(shader, layerIdx);
            layerTextureId = layer->texture;
            if (layerTextureId != NULL)
            {
                u8 overrideType;
                {
                    if ((overrideType = layer->materialId) != '\0')
                    {
                        MapTextureOverride* overrides;
                        overrideIdx = 0;
                        overrides = gMapTextureOverrides;
                        overrideEntry = overrides;
                        for (remain = 0x50; remain != 0 || (texture = layerTextureId, 0); remain--)
                        {
                            if (((overrideEntry->refCount > 0) &&
                                 (overrideEntry->texture == layerTextureId)) &&
                                ((int)overrideType == overrideEntry->type))
                            {
                                texture = textureGetAnimationFrame(
                                    layerTextureId, overrides[overrideIdx].frame);
                                break;
                            }
                            overrideEntry = overrideEntry + 1;
                            overrideIdx = overrideIdx + 1;
                        }
                    }
                    else
                    {
                        texture = layerTextureId;
                    }
                    if (layer->scrollMtx != 0xff)
                    {
                        int scrollOffset = (u32)layer->scrollMtx * 0x10;
                        tx = ((MapTextureScroll*)((u8*)gMapTextureScrolls + scrollOffset))->offsetX / 1048576.0f;
                        PSMTXTrans(texMatrix, tx,
                                   ((MapTextureScroll*)((u8*)gMapTextureScrolls + scrollOffset))->offsetY / 1048576.0f,
                                   0.0f);
                        texMtx = texMatrix;
                    }
                    else
                    {
                        texMtx = NULL;
                    }
                    layerByte = layer->typeBits & 0x7f;
                    if ((SHADER_FLAGS(shader) & 0x40000) != 0)
                    {
                        addTexLayerStagesLit((void*)texture, texMtx);
                    }
                    else
                    {
                        addTexLayerStage(texture, texMtx, layerByte);
                    }
                }
            }
            else
            {
                addVertexColorStage();
            }
        }
        if ((SHADER_FLAGS(shader) & 0x100) != 0)
        {
            addSmallReflectionTevStage();
        }
    }
    return;
}

static Shader* fh_mapBlockRender_setShader(u8 doSetup, MapBlockData* blockData, ModelRenderInstrsState* state,int planned_shader) {
    Shader* shader;
    u32 shaderIdx;
    GXColor fogColor;
    u8* byteBase;
    u32 flags;
    int* cloudTex;
    u8 ambColor[3];
    u8 fogRgba[4];
    u32 bits;
    u32 bitPos;

    if (fh_minimal_enabled) {fh_minimal_movie_active=0;fh_minimal_callback.active=0;}
    fogColor = gTexShaderFogColor;
    bitPos = state->bit;
    if (planned_shader>=0) {
        state->bit=bitPos+6;shader=blockData->shaders+planned_shader;
    } else
    {
        int off = (int)bitPos >> 3;
        byteBase = state->instrs;
        bits = byteBase[off];
        byteBase += off;
        bits |= (u32)byteBase[1] << 8;
        bits |= (u32)byteBase[2] << 16;
        state->bit = bitPos + 6;
        shaderIdx = (bits >> (bitPos & 7)) & 0x3f;
        shader = &blockData->shaders[shaderIdx];
    }

    if (doSetup == 0) {
        return shader;
    }

    if ((SHADER_FLAGS(shader) & 4) != 0) {
        _gxSetFogParams();
    } else {
        GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, fogColor);
    }
    if ((shader != 0) && ((SHADER_FLAGS(shader) & 0x80000000) != 0)) {
        return shader;
    }
    if ((shader != 0) && ((SHADER_FLAGS(shader) & 0x20000) != 0)) {
        u32 res;
        fh_minimal_texture_scope_end();
        res = AttractMovie_DrawTextureCallback(0, 0, 0);
        if ((res & 0xff) != 0) {
            return shader;
        }
    }
    if (fh_render_floor) return shader;
    Rcp_ResetTextureStageState();
    if ((SHADER_FLAGS(shader) & 0x80) != 0) {
        setupHeatShimmerTevStages((char*)shader);
    } else {
        mapBlockRender_setupShaderTextures(shader, 0x80);
    }
    flags = SHADER_FLAGS(shader);
    if ((flags & 0x20) != 0 && (cloudTex = gCloudLayerTexture) != 0) {
        addSignedOverlayTexStage((u8*)cloudTex, &gCloudLayerTexMatrix, gCloudLayerOverlayColor);
    } else if ((flags & 0x40) != 0) {
        addWarpedRingTevStages();
    } else if (isHeavyFogEnabled()) {
        getFogColorRgb(fogRgba);
        renderHeavyFog(fogRgba);
    }
    if (((SHADER_FLAGS(shader) & 0x40000000) != 0) || ((SHADER_FLAGS(shader) & 0x20000000) != 0)) {
        GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_NOOP);
        gxSetZMode_(1, GX_LEQUAL, 0);
        gxSetPeControl_ZCompLoc_(1);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    } else if ((SHADER_FLAGS(shader) & 0x400) != 0 && (SHADER_FLAGS(shader) & 0x80) == 0) {
        GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
        gxSetZMode_(1, GX_LEQUAL, 1);
        gxSetPeControl_ZCompLoc_(0);
        GXSetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_GREATER, 0);
    } else {
        GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
        gxSetZMode_(1, GX_LEQUAL, 1);
        gxSetPeControl_ZCompLoc_(1);
        GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    }
    if ((SHADER_FLAGS(shader) & 1) != 0 || (SHADER_FLAGS(shader) & 0x40000) != 0 ||
        (SHADER_FLAGS(shader) & 0x800) != 0 || (SHADER_FLAGS(shader) & 0x1000) != 0) {
        GXSetChanAmbColor(GX_COLOR0, gTexShaderAmbColor);
        if ((SHADER_FLAGS(shader) & 0x40000) != 0) {
            GXSetChanCtrl(GX_COLOR0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        } else {
            GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        }
    } else {
        objGetSunColor(0, &ambColor[0], &ambColor[1], &ambColor[2]);
        GXSetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
        GXSetChanAmbColor(GX_COLOR0, *(GXColor*)&ambColor[0]);
    }
    if ((SHADER_FLAGS(shader) & 0x8) != 0) {
        GXSetCullMode(GX_CULL_BACK);
    } else {
        GXSetCullMode(GX_CULL_NONE);
    }
    return shader;
}

Shader* mapBlockRender_setShader(u8 doSetup,MapBlockData* blockData,ModelRenderInstrsState* state) {
    return fh_mapBlockRender_setShader(doSetup,blockData,state,-1);
}

typedef struct TrackP6Entry
{
    f32 relX0;
    f32 relY0;
    f32 relZ0;
    f32 relX1;
    f32 relY1;
    f32 relZ1;
    f32 relX2;
    f32 relY2;
    f32 relZ2;
} TrackP6Entry;


/* TrackTriangle -- the 0x4c-byte collision triangle record packed into
 * gTrackTriangleBuffer.  Plane and edge-plane normals are prebaked f32;
 * vertex coordinates are stored as s16 triplets grouped by axis
 * (x0 x1 x2 / y0 y1 y2 / z0 z1 z2), which the hit-detect code reads both
 * by field and as an s16 index off the record base. */
typedef struct TrackTriangle
{
    f32 planeD;     /* 0x00 plane equation constant */
    f32 planeN[3];  /* 0x04 plane normal xyz */
    s16 vx[3];      /* 0x10 vertex x coords */
    s16 vy[3];      /* 0x16 vertex y coords */
    s16 vz[3];      /* 0x1c vertex z coords */
    u8 pad22[2];    /* 0x22 */
    f32 edgeN0[3];  /* 0x24 edge 0 outward normal */
    f32 edgeN1[3];  /* 0x30 edge 1 outward normal */
    f32 edgeN2[3];  /* 0x3c edge 2 outward normal */
    u8 surfaceType; /* 0x48 copied into intersect-line records */
    s8 flags;       /* 0x49 0x10 = disabled, 0x4 = force */
    u8 minMaxY;     /* 0x4a lo/hi nibble: s16 index (base 0xb) of min/max height */
    u8 edgeOutBits; /* 0x4b per-edge outside bits from last query */
} TrackTriangle;

extern int sSynthFadeUnit;
extern int renderFlags;
extern f32 lbl_803DEBDC;
extern f32 lbl_803DEC40;

void* trackGetBlockDescriptors(u32* outVal);

void mapBlockRender_setVtxDcrs(u8 doSetup, MapBlockData* block, Shader* shader,
                               ModelRenderInstrsState* state)
{
    u32 val;
    int pos;
    int off;
    u8* p;
    int bit;
    u32 val2;
    int pos2;
    int off2;
    u8* q;
    int bit2;
    u32 val3;
    int pos3;
    int off3;
    u8* r;
    int bit3;
    int i;

    if (doSetup != 0)
    {
        GXClearVtxDesc();
    }
    pos = state->bit;
    off = pos >> 3;
    val = state->instrs[off];
    p = state->instrs + off;
    val |= p[1] << 8;
    val |= p[2] << 16;
    state->bit = pos + 1;
    bit = (val >> (pos & 7)) & 1;
    if (doSetup != 0)
    {
        GXSetVtxDesc(GX_VA_POS, bit ? GX_INDEX16 : GX_INDEX8);
    }
    pos2 = state->bit;
    off2 = pos2 >> 3;
    val2 = state->instrs[off2];
    q = state->instrs + off2;
    val2 |= q[1] << 8;
    val2 |= q[2] << 16;
    state->bit = pos2 + 1;
    bit2 = (val2 >> (pos2 & 7)) & 1;
    if (doSetup != 0)
    {
        GXSetVtxDesc(GX_VA_CLR0, bit2 ? GX_INDEX16 : GX_INDEX8);
    }
    pos3 = state->bit;
    off3 = pos3 >> 3;
    val3 = state->instrs[off3];
    r = state->instrs + off3;
    val3 |= r[1] << 8;
    val3 |= r[2] << 16;
    state->bit = pos3 + 1;
    bit3 = (val3 >> (pos3 & 7)) & 1;
    if (doSetup != 0)
    {
        if (shader != NULL && (shader->flags & 0x80000000) == 0)
        {
            for (i = 0; i < shader->layerCount; i++)
            {
                GXSetVtxDesc(i + GX_VA_TEX0, bit3 ? GX_INDEX16 : GX_INDEX8);
            }
        }
        else
        {
            GXSetVtxDesc(GX_VA_TEX0, bit3 ? GX_INDEX16 : GX_INDEX8);
        }
    }
}


void setupToRenderMapBlock(MapBlockData* block, void* posMtx)
{
    Mtx out;
    Mtx tmp;
    f32 fc;

    GXLoadPosMtxImm((const f32 (*)[4])posMtx, GX_PNMTX0);
    PSMTXCopy((MtxPtr)posMtx, tmp);
    fc = 0.0f;
    tmp[0][3] = fc;
    tmp[1][3] = fc;
    tmp[2][3] = fc;
    GXLoadNrmMtxImm(tmp, GX_PNMTX0);
    PSMTXConcat((MtxPtr)gCameraLightPerspectiveMatrix, (MtxPtr)posMtx, out);
    GXLoadTexMtxImm(out, GX_TEXMTX2, GX_MTX3x4);
    GXSetArray(GX_VA_POS, block->vertices, block->vertexCount * 6, 6, true);
    GXSetArray(GX_VA_CLR0, block->vertexColors, block->colorCount * 2, 2, true);
    GXSetArray(GX_VA_TEX0, block->vertexTexCoords, block->texCoordCount * 4, 4, true);
    GXSetArray(GX_VA_TEX1, block->vertexTexCoords, block->texCoordCount * 4, 4, true);
}


static const FhSceneAsset* fh_scene_world_prepare(MapBlockData* block, unsigned char admitted[256], unsigned char visible[256]) {
    if (fh_render_floor || !fh_native_scene_enabled) return NULL;
    fh_native_scene_admission[6]++;
    if (isHeavyFogEnabled()) return NULL;
    fh_gx_dispatch_barrier();
    const FhSceneAsset* asset = fh_native_scene_find(block);
    if (asset) fh_native_scene_admission[7]++;
    if (!asset || !asset->block || !fh_native_scene_source(asset, 0, block->vertices, block->vertexCount * 6u) ||
        !fh_native_scene_source(asset, 2, block->vertexColors, block->colorCount * 2u) ||
        !fh_native_scene_source(asset, 3, block->vertexTexCoords, block->texCoordCount * 4u) ||
        !fh_native_scene_source(asset, 4, block->renderInstrsMain, block->nRenderInstrsMain)) return NULL;
    for (unsigned i = 0; i < block->displayListCount; i++) {
        MapBlockBoundsRec* list = block->displayLists + i;
        if (!fh_native_scene_source(asset, 5 + i, list->dlist, list->dlistSize)) return NULL;
    }
    signed char material[64]; memset(material, -1, sizeof(material));
    memset(admitted, 0, 256); memset(visible, 0, 256);
    fh_native_scene_admission[8]++;
    for (unsigned i = 0; i < asset->groups; i++) {
        FhSceneGroup group; fh_native_scene_group(asset, i, &group);
        unsigned ok = fh_renderer_scene_page(group.page) != NULL, lighting = 0xffffffff;
        for (unsigned j = 0; ok && j < group.draws; j++) {
            FhSceneSection section; fh_native_scene_section(asset, group.first_section + j, &section);
            if (section.shader >= block->shaderCount || section.list >= block->displayListCount) { ok = 0; break; }
            Shader* shader = block->shaders + section.shader;
            /* Cloud/warp, radial lights and deferred lightmap passes retain
             * their original path until the native equivalent is implemented. */
            if (shader->flags & (2 | 0x20 | 0x40 | 0x800 | 0x1000 | 0x4000 | 0x8000 | 0x10000)) { ok = 0; break; }
            unsigned mode = shader->flags & (1 | 4 | 0x40000);
            if (lighting != 0xffffffff && lighting != mode) { ok = 0; break; }
            lighting = mode;
            if (material[section.shader] < 0) material[section.shader] = fh_scene_material(&section, shader, 1);
            if (!material[section.shader]) { ok = 0; break; }
            float loX, loY, loZ, hiX, hiY, hiZ;
            unsigned seen = mapBlockBounds_ComputeAndTestPlanes(block->displayLists + section.list, block,
                gViewFrustumPlanes, FRUSTUM_PLANE_COUNT, &loX, &loY, &loZ, &hiX, &hiY, &hiZ);
            visible[group.first + j] = seen != 0;
            if (seen && !(shader->flags & 0x80000)) {
                int count = 0;
                modelLightStruct_selectBrightestAabbLights(loX + playerMapOffsetX, loY, loZ + playerMapOffsetZ,
                    hiX + playerMapOffsetX, hiY, hiZ + playerMapOffsetZ, gTexBlockLightList, 2, &count);
                if (count) { ok = 0; break; }
            }
        }
        admitted[i] = ok; fh_native_scene_admission[ok ? 9 : 10]++;
    }
    return asset;
}
static int fh_scene_world_draw(const FhSceneAsset* asset, const FhSceneGroup* group,
    MapBlockData* block, const unsigned char visible[256], const float view[3][4]) {
    FhSceneSection first; fh_native_scene_section(asset, group->first_section, &first);
    Shader* shader = block->shaders + first.shader;
    fh_scene_surface(group); GXSetNumChans(1);
    if (shader->flags & 4) _gxSetFogParams();
    else GXSetFog(GX_FOG_NONE, 0, 0, 0, 0, gTexShaderFogColor);
    GXColor ambient = gTexShaderAmbColor;
    if (!(shader->flags & (1 | 0x40000))) objGetSunColor(0, &ambient.r, &ambient.g, &ambient.b);
    GXSetChanAmbColor(GX_COLOR0, ambient);
    GXSetChanCtrl(GX_COLOR0, (shader->flags & 0x40000) ? GX_DISABLE : GX_ENABLE,
        GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    GXSetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT_NULL, GX_DF_NONE, GX_AF_NONE);
    const float pose[16] = {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    FhModelSource source = {.position_count=block->vertexCount, .color_count=block->colorCount, .uv_count=block->texCoordCount};
    for (unsigned j = 0; j < group->draws;) {
        if (!visible[group->first + j]) { j++; continue; }
        FhSceneSection section; fh_native_scene_section(asset, group->first_section + j, &section);
        FhSceneGroup part = *group; part.index += section.first; part.indices = 0; part.draws = 0;
        do {
            fh_native_scene_section(asset, group->first_section + j, &section);
            part.indices += section.count; part.draws++; j++;
        } while (j < group->draws && visible[group->first + j]);
        if (!fh_gx_native_scene_draw(asset, &part, &source, pose, 1, view, 1)) return 0;
    }
    return 1;
}

/* No live shader, texture or matrix state is cached. Exclude callbacks before
 * executing anything; unsupported/over-budget plans use the whole interpreter. */
static int fh_execute_world_plan(const FhSceneAsset* asset,MapBlockData* block,
    ModelRenderInstrsState* state,float* matrix) {
    const FhWorldDrawPlan* plan=fh_native_scene_world_plan(asset);
    if (!plan) return 0;
    for (unsigned i=0;i<plan->commands;i++) {
        const FhWorldPlanOp* op=plan->command+i;
        if (op->kind==1 && (op->value>=block->shaderCount ||
            (block->shaders[op->value].flags&0x20000))) return 0;
        if (op->kind==2 && (op->shader>=block->shaderCount || op->value>=block->displayListCount)) return 0;
    }
    Shader* shader=NULL;
    fh_world_plan_runs++;
    for (unsigned i=0;i<plan->commands;i++) {
        const FhWorldPlanOp* op=plan->command+i;
        state->bit=op->bit;
        if (op->kind==1) shader=fh_mapBlockRender_setShader(TRUE,block,state,op->value);
        else if (op->kind==3) {
            /* Retain descriptor side effects for subsequent generic effects. */
            GXClearVtxDesc();
            GXSetVtxDesc(GX_VA_POS,(op->value&1)?GX_INDEX16:GX_INDEX8);
            GXSetVtxDesc(GX_VA_CLR0,(op->value&2)?GX_INDEX16:GX_INDEX8);
            unsigned count=shader && !(shader->flags&0x80000000)?shader->layerCount:1;
            for (unsigned j=0;j<count;j++) GXSetVtxDesc(GX_VA_TEX0+j,(op->value&4)?GX_INDEX16:GX_INDEX8);
        } else {
            fh_world_plan_draws++;
            fh_mapBlockRender_callList(TRUE,0,block,shader,state,matrix,asset,op->ordinal,op,plan->draw+op->ordinal);
        }
    }
    return 1;
}

static unsigned fh_deferred_queued,fh_deferred_drawn,fh_deferred_unimplemented;
static unsigned fh_deferred_skip_flags[48],fh_deferred_skip_family[48],fh_deferred_skip_count[48],fh_deferred_skip_kinds;
void fh_platform_log(const char*);
int snprintf(char*,unsigned int,const char*,...);
void sceneDrawTransparentPolys(void);
extern unsigned fh_pica_world_water_active;
void fh_pica_game_deferred_counts(unsigned out[3]){
    out[0]=fh_deferred_queued;out[1]=fh_deferred_drawn;out[2]=fh_deferred_unimplemented;
    for(unsigned k=0;k<fh_deferred_skip_kinds;k++){char line[96];int n=snprintf(line,sizeof line,"PICA deferred skipped: family=%u flags=%08x count=%u",fh_deferred_skip_family[k],fh_deferred_skip_flags[k],fh_deferred_skip_count[k]);if(n>0&&n<94){line[n]=10;line[n+1]=0;}fh_platform_log(line);}
}
static void fh_pica_queue_world_deferred(MapBlockData *block,unsigned stream){
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_WORLD))return;
    const FhSceneAsset *asset=fh_native_scene_find(block);
    const FhPicaWorldDeferredPlan *plan=asset?fh_native_scene_deferred_plan(asset):NULL;
    if(!plan || stream<1 || stream>2)return;
    unsigned first=stream==2?plan->count[0]:0,count=plan->count[stream-1];
    const float *depth=((const float*)Camera_GetViewMatrix())+8;
    float translation[3]={block->transform[0][3],block->transform[1][3],block->transform[2][3]};
    for(unsigned i=first;i<first+count;i++){
        const FhPicaWorldDeferredEntry *e=plan->entries+i;
        unsigned flags=block->shaders[e->draw.material].flags;
        /* First native family: ordinary alpha surfaces. Projected falloff,
         * caustics and additional effects need distinct native consumers. */
        if((e->draw.family==5 && !fh_hardware_is_new) || (e->draw.family!=5 && (e->draw.family!=4 || !fh_pica_deferred_surface_flags(flags)))){
            {unsigned k=0;for(;k<fh_deferred_skip_kinds;k++)if(fh_deferred_skip_flags[k]==flags&&fh_deferred_skip_family[k]==e->draw.family)break;
            if(k==fh_deferred_skip_kinds&&k<48){fh_deferred_skip_flags[k]=flags;fh_deferred_skip_family[k]=e->draw.family;fh_deferred_skip_kinds++;}
            if(k<48)fh_deferred_skip_count[k]++;}
            fh_deferred_unimplemented++;continue;
        }
        float loX,loY,loZ,hiX,hiY,hiZ;
        if(!mapBlockBounds_ComputeAndTestPlanes(block->displayLists+e->draw.list,block,
            gViewFrustumPlanes,FRUSTUM_PLANE_COUNT,&loX,&loY,&loZ,&hiX,&hiY,&hiZ))continue;
        uint32_t key;
        if(!fh_pica_world_sort_key(&e->bounds,gTrackPackedCoordScale,translation,depth,&key))continue;
        if(gLightmapDrawQueueCount==1000){sceneDrawTransparentPolys();gLightmapDrawQueueCount=0;}
        gLightmapDrawQueue[gLightmapDrawQueueCount].a=(uintptr_t)e;
        gLightmapDrawQueue[gLightmapDrawQueueCount].b=(uintptr_t)block;
        gLightmapDrawQueue[gLightmapDrawQueueCount].key=key;
        gLightmapDrawQueue[gLightmapDrawQueueCount++].d=4;fh_deferred_queued++;
    }
}

static int fh_deferred_fade_visible(const FhPicaWorldDeferredEntry *e,MapBlockData *block){
    GameObject *player=Obj_GetPlayerObject();
    if(!player)return 1;
    float dist=Camera_DistanceToCurrentViewPosition(player->anim.worldPosX,player->anim.worldPosY,player->anim.worldPosZ);
    const float *row=((const float*)Camera_GetViewMatrix())+8;
    float farthest=-1e30f;
    for(unsigned c=0;c<8;c++){
        float p[3];
        for(unsigned k=0;k<3;k++)p[k]=(((c>>k)&1)?e->bounds.hi[k]:e->bounds.lo[k])*gTrackPackedCoordScale+block->transform[k][3];
        float d=-(((row[0]*p[0]+row[1]*p[1])+row[2]*p[2])+row[3]);
        if(d>farthest)farthest=d;
    }
    return farthest>=dist-10.f;
}
void fh_pica_game_deferred_draw(const FhPicaWorldDeferredEntry *e,MapBlockData *block){
    const FhSceneAsset *asset=fh_native_scene_find(block);
    if(!asset || !fh_pica_world_prepare_positions(asset))return;
    if(e->draw.family==4 && (e->geometry.flags&0x2000u) && !fh_deferred_fade_visible(e,block))return;
    float camera[3][4];unsigned clip[4];
    PSMTXConcat((MtxPtr)Camera_GetViewMatrix(),block->transform,camera);
    GXGetScissor(clip,clip+1,clip+2,clip+3);
    /* Live state already prepared for this block's main pass. */
    fh_pica_world_water_active=e->draw.family==5;
    fh_pica_scene_world_slice(asset,&e->geometry,camera,
        (const float(*)[4])Camera_GetProjectionMatrix(),clip);fh_deferred_drawn++;
    fh_pica_world_water_active=0;
}

void renderMapBlock(MapBlockData* block, u8 type)
{
    if(type==0)fh_budget_world(block);
    else fh_pica_queue_world_deferred(block,type);
}

void renderGlows(void)
{
    f32 px, py, pz;
    s32 sx, sy, sz;
    u8 amb[3];
    GXColor fogCol;
    Mtx sunMtx;
    Vec dir;
    Vec cam;
    MtxPtr viewMtx;
    u8 alpha;
    u8 sunAlpha;
    f32 sunDot;
    int i;
    ModelLightStruct* e;

    fogCol = *(GXColor*)&sSynthFadeUnit;
    GXSetCullMode(GX_CULL_NONE);
    Camera_RebuildProjectionMatrix();
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    gxTevResetStages();
    gxTevColor1TexAlphaStage();
    gxTevCommitStages();
    GXSetFog(GX_FOG_NONE, 0.0f, 0.0f, 0.0f, 0.0f, fogCol);
    gxSetAdditiveBlendNoZTest();
    alpha = 0xff;
    gSunFlareScissorWidth = 0;
    gSunFlareScissorHeight = 0;
    sunAlpha = skyGetSunRenderAlpha(2);
    if (sunAlpha != 0 && (renderFlags & 0x40))
    {
        viewMtx = (MtxPtr)Camera_GetViewMatrix();
        skyGetSunLightDirection(0, &dir.x, &dir.y, &dir.z);
        cam.x = viewMtx[2][0];
        cam.y = viewMtx[2][1];
        cam.z = viewMtx[2][2];
        sunDot = PSVECDotProduct(&dir, &cam);
        if (sunDot > 0.0f)
        {
            int occ;
            f32 fade;
            skyBuildSunModelMatrix(sunMtx);
            Camera_ProjectWorldPointWithOffset(sunMtx[0][3], sunMtx[1][3], sunMtx[2][3], 100.0f, &px, &py, &pz);
            Camera_ClipToScreen(px, py, pz, &sx, &sy, &sz);
            gSunFlareScissorX = sx - 0x10;
            gSunFlareScissorWidth = 0x20;
            gSunFlareScissorY = sy - 0x10;
            gSunFlareScissorHeight = 0x20;
            if ((int)gSunFlareScissorX < 0)
                gSunFlareScissorX = 0;
            else if ((int)gSunFlareScissorX > 0x280)
                gSunFlareScissorX = 0x280;
            if ((int)gSunFlareScissorY < 0)
                gSunFlareScissorY = 0;
            else if ((int)gSunFlareScissorY > 0x1e0)
                gSunFlareScissorY = 0x1e0;
            if ((int)gSunFlareScissorX + 0x20 > 0x280)
                gSunFlareScissorWidth = 0x280 - gSunFlareScissorX;
            if ((int)gSunFlareScissorY + 0x20 > 0x1e0)
                gSunFlareScissorHeight = 0x1e0 - gSunFlareScissorY;
            occ = 0;
            for (i = 0; i < 5; i++)
            {
                int d = depthReadRequestPoll(sx + gSunOcclusionSampleOffsets[i].x,
                                             sy + gSunOcclusionSampleOffsets[i].y, (void*)(uintptr_t)i);
                if (sz <= d && pauseMenuGetState() == 0)
                    occ++;
            }
            fade = (f32)(u32)occ / 5.0f - gSunFlareFade;
            if (fade > 0.0125f)
                fade = 0.0125f;
            else if (fade < -0.0125f)
                fade = -0.0125f;
            gSunFlareFade = gSunFlareFade + fade;
            sunDot = sunDot * gSunFlareFade;
            if (sunDot > 0.0f)
            {
                PSMTXConcat(viewMtx, sunMtx, sunMtx);
                GXLoadPosMtxImm((const f32 (*)[4])sunMtx, GX_PNMTX0);
                GXSetCurrentMtx(GX_PNMTX0);
                selectTexture(skyGetSkyTexture(), 0);
                skyGetSunColor(0, &amb[0], &amb[1], &amb[2]);
                sunDot = (f32)(u32)sunAlpha * sunDot;
                _gxSetTevColor2(amb[0], amb[1], amb[2], (int)(0.5f * sunDot));
                alpha = 255.0f - 0.9f * sunDot;
                fade = 20000.0f * sunDot;
                sunDot = fade * lbl_803DEC40;
                GXBegin(GX_QUADS, GX_VTXFMT2, 4);
                GXPosition3f32(-sunDot, -sunDot, lbl_803DEBCC);
                GXTexCoord2f32(lbl_803DEBCC, lbl_803DEBCC);
                GXPosition3f32(sunDot, -sunDot, lbl_803DEBCC);
                GXTexCoord2f32(lbl_803DEBDC, lbl_803DEBCC);
                GXPosition3f32(sunDot, sunDot, lbl_803DEBCC);
                GXTexCoord2f32(lbl_803DEBDC, lbl_803DEBDC);
                GXPosition3f32(-sunDot, sunDot, lbl_803DEBCC);
                GXTexCoord2f32(lbl_803DEBCC, lbl_803DEBDC);
            }
        }
    }
    colorScale = alpha;
    if (gGlowLightCount != 0)
    {
        for (i = 0; i < gGlowLightCount; i++)
        {
            int d;
            e = gGlowLightList[i];
            Camera_ProjectWorldPointWithOffset(e->worldX - playerMapOffsetX, e->worldY, e->worldZ - playerMapOffsetZ,
                                               e->glowProjectionRadius, &px, &py, &pz);
            Camera_ClipToScreen(px, py, pz, &sx, &sy, &sz);
            d = depthReadRequestPoll(sx, sy, e);
            if (sz <= d && pauseMenuGetState() == 0)
                e->glowAlphaStep = 0x10;
            else
                e->glowAlphaStep = -0x10;
        }
        fhLoadIdentityPosMtx();
        GXSetCurrentMtx(GX_PNMTX_IDENTITY);
        gxTevColor1TexAlphaStage();
        gxSetAdditiveBlendNoZTest();
        for (i = 0; i < gGlowLightCount; i++)
        {
            e = gGlowLightList[i];
            if (e->glowAlpha != 0)
            {
                selectTexture((Texture*)e->glowTexture, 0);
                _gxSetTevColor2((int)((f32)(u32)e->glowColor[0] * e->activeIntensity),
                                (int)((f32)(u32)e->glowColor[1] * e->activeIntensity),
                                (int)((f32)(u32)e->glowColor[2] * e->activeIntensity),
                                (u8)((int)(e->glowColor[3] * e->glowAlpha) >> 8));
                GXBegin(GX_QUADS, GX_VTXFMT2, 4);
                GXPosition3f32(e->viewX - e->glowScale, e->viewY - e->glowScale, e->viewZ);
                GXTexCoord2f32(lbl_803DEBCC, lbl_803DEBCC);
                GXPosition3f32(e->viewX + e->glowScale, e->viewY - e->glowScale, e->viewZ);
                GXTexCoord2f32(lbl_803DEBDC, lbl_803DEBCC);
                GXPosition3f32(e->viewX + e->glowScale, e->viewY + e->glowScale, e->viewZ);
                GXTexCoord2f32(lbl_803DEBDC, lbl_803DEBDC);
                GXPosition3f32(e->viewX - e->glowScale, e->viewY + e->glowScale, e->viewZ);
                GXTexCoord2f32(lbl_803DEBCC, lbl_803DEBDC);
            }
        }
        GXSetCurrentMtx(GX_PNMTX0);
    }
}

void getSunFlareScissorRect(int* outX, int* outY, int* outWidth, int* outHeight)
{
    *outX = gSunFlareScissorX;
    *outY = gSunFlareScissorY;
    *outWidth = gSunFlareScissorWidth;
    *outHeight = gSunFlareScissorHeight;
}

static inline int isGlowInFrustum(ModelLightStruct* light)
{
    FrustumPlane* plane;
    u8 i;
    f32 offsetX;
    f32 offsetZ;
    f32 bias;

    i = 0;
    offsetZ = playerMapOffsetZ;
    offsetX = playerMapOffsetX;
    bias = lbl_803DEBCC;
    for (; i < 5; i++)
    {
        f32 dot;
        plane = &gViewFrustumPlanes[i];
        dot = light->worldY * plane->normalY + plane->normalX * (light->worldX - offsetX) +
                  plane->normalZ * (light->worldZ - offsetZ) + plane->distance + bias;
        if (dot < bias)
        {
            return 0;
        }
    }
    return 1;
}

void queueGlowRender(ModelLightStruct* light)
{
    int visible;
    u8 idx;

    if (gGlowLightCount >= 100)
        return;

    visible = isGlowInFrustum(light);
    {
        u8 vis = visible;
        if (vis == 0 && light->glowAlpha == 0)
            return;
        if (vis == 0)
        {
            light->glowAlphaStep = -0x10;
        }
    }
    idx = gGlowLightCount++;
    gGlowLightList[idx] = light;
}

void trackPackVector(short* out, float* vec)
{
    int yScaled;
    int zScaled;

    yScaled = (int)(8.0f * vec[1]);
    zScaled = (int)(8.0f * vec[2]);
    *out = (short)(int)(8.0f * *vec);
    out[1] = yScaled;
    out[2] = zScaled;
}

void trackUnpackVector(s16* in, f32* out)
{
    out[0] = (f32)(s32)in[0] * gTrackPackedCoordScale;
    out[1] = (f32)(s32)in[1] * gTrackPackedCoordScale;
    out[2] = (f32)(s32)in[2] * gTrackPackedCoordScale;
}

/* trackBuildModelTriangles -- gather model triangles overlapping a swept bbox into the
 * hit-detect triangle buffer at cur (0x4c-byte records); returns advanced
 * cursor. */

u32 trackGetPackedSurfaceType(int* obj)
{
    u32 v = obj[4];
    v &= 0x00FF0000;
    return v >> 16;
}

int mapBlockGetPolygonGroupType(void* obj)
{
    return (((MapTriGroup*)obj)->flags & 0xff000000) >> 24;
}

int mapBlockCountTrianglesByType(MapBlockData* block, int type)
{
    MapTriGroup* entry;
    int offset;
    int total;
    int i;
    int count;
    total = 0;
    offset = 0;
    count = block->polyGroupCount;
    for (i = 0; i < count; i++)
    {
        entry = (MapTriGroup*)((u8*)block->polygonGroups + offset);
        if (type == (int)((entry->flags & 0xff000000) >> 24))
        {
            total += entry[1].firstTri - entry->firstTri;
        }
        offset += 0x14;
    }
    return total;
}

void* mapBlockGetPolygon(MapBlockData* obj, int idx)
{
    return (char*)obj->gcPolygons + idx * 8;
}

MapTriGroup* mapBlockGetPolygonGroup(MapBlockData* obj, int idx)
{
    return (MapTriGroup*)obj->polygonGroups + idx;
}

MapBlockBoundsRec* mapBlockGetDisplayListBounds(MapBlockData* obj, int idx)
{
    return &obj->displayLists[idx];
}

Shader* mapBlockGetShader(MapBlockData* obj, int idx)
{
    return obj->shaders + idx;
}


static void fh_pica_load_materials(MapBlockData* block) {
    if(!fh_pica_scene_enabled)return;
    const FhSceneAsset* asset=fh_native_scene_find(block);
    if(!asset || !fh_pica_world_attach_positions(asset,block->vertices,block->vertexCount)) {
        fh_minimal_coverage_failure(34,asset?asset->id:0,0);return;
    }
    if(!fh_pica_scene_textured)return;
    const FhPicaWorldPlan* plan=fh_native_scene_pica_plan(asset);
    const FhPicaWorldDeferredPlan *deferred=fh_native_scene_deferred_plan(asset);
    unsigned deferred_readers=0;
    if(deferred)for(unsigned i=0;i<deferred->count[0]+deferred->count[1];i++)
        deferred_readers+=(deferred->entries[i].draw.family==5 && fh_hardware_is_new) || (deferred->entries[i].draw.family==4 &&
            fh_pica_deferred_surface_flags(deferred->entries[i].geometry.flags));
    if(plan && !plan->count && !deferred_readers)return;
    if(!plan || block->shaderCount>64 || !fh_pica_materials_begin(asset,block->shaderCount)) {
        fh_minimal_coverage_failure(29,asset?asset->id:0,0);return;
    }
    unsigned used[64]={0};
    for(unsigned i=0;i<plan->count;i++) {
        if(plan->draw[i].material>=block->shaderCount){fh_minimal_coverage_failure(29,asset->id,1);return;}
        used[plan->draw[i].material]=1;
    }
    if(deferred)for(unsigned i=0;i<deferred->count[0]+deferred->count[1];i++){
        const FhPicaWorldDeferredEntry *e=deferred->entries+i;
        if((e->draw.family==5 && fh_hardware_is_new) || (e->draw.family==4 && fh_pica_deferred_surface_flags(e->geometry.flags)))used[e->draw.material]=1;
    }
    for(unsigned i=0;i<block->shaderCount;i++)if(used[i]) {
        Shader* sh=block->shaders+i;
        unsigned layers=sh->layerCount,counts[2]={0,0};
        if(layers>2 || (layers==2 && (sh->layers[1].typeBits&127)!=9)) {
            fh_minimal_coverage_failure(30,asset->id,i);continue;
        }
        if(layers==1 && !sh->layers[0].texture)layers=0;
        for(unsigned j=0;j<layers;j++) {
            Texture* t=sh->layers[j].texture;
            counts[j]=t?(t->animationFrameCount+255u)/256u:0;
            if(!counts[j] && t)counts[j]=1;
        }
        if(!fh_pica_material_begin(asset,i,layers,counts[0],counts[1],sh->flags)) {
            fh_minimal_coverage_failure(31,asset->id,i);continue;
        }
        for(unsigned j=0;j<layers;j++)for(unsigned k=0;k<counts[j];k++) {
            Texture* t=textureGetAnimationFrame(sh->layers[j].texture,k<<8);
            FhTexture image;
            if(!t || !fh_gx_texture_view(textureGetGXTexObj(t),&image) ||
                !fh_pica_material_frame(asset,i,j,k,&image))fh_minimal_coverage_failure(32,asset->id,i);
        }
    }
    if(!fh_pica_materials_compile(asset)) {
        char line[128];snprintf(line,sizeof(line),"PICA world materials declined: asset=%u\\n",asset->id);fh_platform_log(line);
        fh_pica_materials_release(fh_native_scene_pica_materials(asset));
    }
}
static void fh_pica_live_materials(const FhSceneAsset* asset,MapBlockData* block) {
    if(!fh_pica_scene_textured)return;
    if(fh_graphics_options&FH_GRAPHICS_HEIGHT_FOG){
    /* Original generated image, camera matrix and fog colour; no GX callbacks
     * or draw-time texture acquisition. PNMTX0 is this block's camera matrix. */
    extern float gHeavyFogDepthScale,gHeavyFogDepthOffset,gHeavyFogTop,gHeavyFogBottom;
    extern unsigned char gHeavyFogMode,gNewShadowHeavyFogIntensity;
    Texture *height_image=NULL;unsigned char fog_rgba[4]={0,0,0,255};
    if(isHeavyFogEnabled())getNewShadowHeavyFogTexture(&height_image);
    if(height_image)getFogColorRgb(fog_rgba);
    unsigned fog_color=(unsigned)fog_rgba[0]|((unsigned)fog_rgba[1]<<8)|((unsigned)fog_rgba[2]<<16)|((unsigned)fog_rgba[3]<<24);
    fh_pica_height_fog_world_set(height_image?(const unsigned char*)height_image+sizeof(Texture):NULL,
        gNewShadowHeavyFogIntensity,(const float(*)[4])Camera_GetInverseViewMatrix(),
        gHeavyFogDepthScale,gHeavyFogDepthOffset,gHeavyFogTop,gHeavyFogBottom,fog_color,gHeavyFogMode);
    }else fh_pica_height_fog_world_disable();

    /* One small parameter record per material, irrespective of draw count.
     * Animation producers and texture scrolling remain the game's own code. */
    for(unsigned i=0;i<block->shaderCount;i++) {
        Shader* sh=block->shaders+i;float tint[4]={1,1,1,1},scroll[2][2]={{0}};
        unsigned frame[2]={0,0},konst;memcpy(&konst,&lbl_803DEBB0,4);
        if(!(sh->flags&0x40000)) {
            unsigned char rgb[4];
            if(sh->flags&(1|0x800|0x1000))memcpy(rgb,&gTexShaderAmbColor,4);
            else objGetSunColor(0,rgb,rgb+1,rgb+2);
            for(unsigned j=0;j<3;j++)tint[j]=rgb[j]/255.f;
        }
        if(!(fh_graphics_options&FH_GRAPHICS_NO_WORLD_TEX))for(unsigned j=0;j<sh->layerCount && j<2;j++) {
            ShaderLayer* layer=sh->layers+j;
            if(layer->materialId)for(unsigned k=0;k<0x50;k++) {
                MapTextureOverride* entry=gMapTextureOverrides+k;
                if(entry->refCount>0 && entry->texture==layer->texture && entry->type==layer->materialId) {
                    frame[j]=entry->frame<0?0:(unsigned)entry->frame;break;
                }
            }
            if(layer->scrollMtx!=0xff) {
                scroll[j][0]=gMapTextureScrolls[layer->scrollMtx].offsetX/1048576.f;
                scroll[j][1]=gMapTextureScrolls[layer->scrollMtx].offsetY/1048576.f;
            }
        }
        fh_pica_material_live(asset,i,tint,scroll,frame,konst,sh->flags);
    }
}

void MapBlock_initShaders(MapBlockData* block)
{
    int i;
    int j;
    int ref;
    Shader* sh;
    for (i = 0; i < block->shaderCount; i++)
    {
        sh = &block->shaders[i];
        for (j = 0; j < sh->layerCount; j++)
        {
            ref = sh->layers[j].textureIndex;
            if (ref != -1)
            {
                sh->layers[j].texture = block->textures[ref].texture;
                ref = sh->layers[j].materialId;
                if ((u32)ref != 0u)
                {
                    mapTextureOverrideAcquire(sh->layers[j].texture, 0, ref);
                }
            }
            else
            {
                sh->layers[j].texture = NULL;
            }
            sh->layers[j].scrollMtx = 0xff;
        }
        ref = sh->auxTextureIndex;
        if (ref != -1)
        {
            sh->auxTexture = block->textures[ref].texture;
        }
        else
        {
            sh->auxTexture = NULL;
        }
    }
    fh_pica_load_materials(block);
}

static inline void* mapBlockRelocatePointer(MapBlockData* block, void* offset)
{
    return (u8*)block + (uintptr_t)offset;
}

void MapBlock_init(MapBlockData* block)
{
    int i;

    if (block->textures != NULL)
        block->textures = mapBlockRelocatePointer(block, block->textures);
    if (block->gcPolygons != NULL)
        block->gcPolygons = mapBlockRelocatePointer(block, block->gcPolygons);
    if (block->polygonGroups != NULL)
        block->polygonGroups = mapBlockRelocatePointer(block, block->polygonGroups);
    block->vertices = mapBlockRelocatePointer(block, block->vertices);
    block->vertexColors = mapBlockRelocatePointer(block, block->vertexColors);
    block->vertexTexCoords = mapBlockRelocatePointer(block, block->vertexTexCoords);
    if (block->renderInstrsMain != NULL)
        block->renderInstrsMain = mapBlockRelocatePointer(block, block->renderInstrsMain);
    if (block->renderInstrsTransp != NULL)
        block->renderInstrsTransp = mapBlockRelocatePointer(block, block->renderInstrsTransp);
    if (block->renderInstrsWater != NULL)
        block->renderInstrsWater = mapBlockRelocatePointer(block, block->renderInstrsWater);
    block->displayLists = mapBlockRelocatePointer(block, block->displayLists);
    if (block->shaders != NULL)
        block->shaders = mapBlockRelocatePointer(block, block->shaders);

    for (i = 0; i < block->displayListCount; i++)
    {
        block->displayLists[i].dlist = mapBlockRelocatePointer(block, block->displayLists[i].dlist);
    }
    fh_native_ground_mask_load(block,mapBlockGetPolygon(block,0),block->nPolygons);
}

void MapBlock_initHits(MapBlockData* block, int index)
{
    int i;
    int* table = (int*)gHitsTab;
    int fileOff = table[index];
    int size = table[index + 1] - fileOff;
    MapHitLine* entry;
    s16 value;

    if (size > 0)
    {
        block->hits = mmAlloc(size, 5, 0);
        fileLoadToBufferOffset(MLDF_FILEID_HITS_BIN, block->hits, fileOff, size);
    }
    block->hitCount = (u32)size / sizeof(MapHitLine);
    i = 0;
    while (i < block->hitCount)
    {
        entry = &block->hits[i];
        entry->x[0] = (s16)fhSwap16((u16)entry->x[0]);
        entry->x[1] = (s16)fhSwap16((u16)entry->x[1]);
        entry->y[0] = (s16)fhSwap16((u16)entry->y[0]);
        entry->y[1] = (s16)fhSwap16((u16)entry->y[1]);
        entry->z[0] = (s16)fhSwap16((u16)entry->z[0]);
        entry->z[1] = (s16)fhSwap16((u16)entry->z[1]);
        entry->param = (s16)fhSwap16((u16)entry->param);
        if (entry->x[0] < 0 || (value = entry->x[1]) < 0 || entry->x[0] > 0x280 || value > 0x280)
        {
            entry->kind = 0x40;
        }
        entry = &block->hits[i];
        if (entry->z[0] < 0 || (value = entry->z[1]) < 0 || entry->z[0] > 0x280 || value > 0x280)
        {
            entry->kind = 0x40;
        }
        i++;
    }
    block->auxData = NULL;
    block->unk9E = 0;
    block->flags4 &= ~0x40;
}

static u32 mapBlockReadU32(u8* raw, u32 offset)
{
    return fhSwap32(*(u32*)(raw + offset));
}

static u16 mapBlockReadU16(u8* raw, u32 offset)
{
    return fhReadBE16(raw + offset);
}

static s16 mapBlockReadS16(u8* raw, u32 offset)
{
    return fhReadBES16(raw + offset);
}

static f32 mapBlockReadF32(u8* raw, u32 offset)
{
    u32 value = mapBlockReadU32(raw, offset);
    f32 result;
    memcpy(&result, &value, sizeof(result));
    return result;
}

static uintptr_t mapBlockDataOffset(u8* raw, u32 offset, u32 rawOffset)
{
    u32 value = mapBlockReadU32(raw, offset);
    return value == 0 ? 0 : value + rawOffset;
}

static void mapBlockUnpackShaders(Shader* shaders, u8* raw, u32 offset, u32 count)
{
    u32 i;
    int j;
    for (i = 0; i < count; i++)
    {
        u8* src = raw + offset + i * 0x44;
        Shader* shader = &shaders[i];
        memcpy(shader->pad00, src, sizeof(shader->pad00));
        shader->reg1Texture = (void*)(uintptr_t)mapBlockReadU32(src, 0x08);
        shader->alpha = src[0x0c];
        memcpy(shader->pad0D, src + 0x0d, sizeof(shader->pad0D));
        shader->reg2Texture = (void*)(uintptr_t)mapBlockReadU32(src, 0x14);
        shader->textureId = (s32)mapBlockReadU32(src, 0x18);
        shader->unk1C = mapBlockReadU32(src, 0x1c);
        shader->reg2TexSlot = src[0x20];
        shader->pad21 = src[0x21];
        shader->reg2Alpha = src[0x22];
        shader->pad23 = src[0x23];
        for (j = 0; j < 2; j++)
        {
            u8* layer = src + 0x24 + j * 8;
            shader->layers[j].texture = NULL;
            shader->layers[j].textureIndex = (s32)mapBlockReadU32(layer, 0);
            shader->layers[j].typeBits = layer[4];
            shader->layers[j].materialId = layer[5];
            shader->layers[j].scrollMtx = layer[6];
            shader->layers[j].unk7 = layer[7];
        }
        shader->auxTexture = NULL;
        shader->auxTextureIndex = mapBlockReadU32(src, 0x34);
        shader->indTexture = NULL;
        shader->indTextureId = (s32)mapBlockReadU32(src, 0x38);
        shader->flags = mapBlockReadU32(src, 0x3c);
        shader->vtxAttrFlags = src[0x40];
        shader->layerCount = src[0x41];
        shader->envMapParams = src[0x42];
        shader->alphaOverride = src[0x43];
    }
}

static void mapBlockUnpackDisplayLists(MapBlockBoundsRec* lists, u8* raw, u32 offset, u32 count, u32 rawOffset)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        u8* src = raw + offset + i * 0x1c;
        MapBlockBoundsRec* list = &lists[i];
        list->dlist = (void*)(uintptr_t)(mapBlockReadU32(src, 0) + rawOffset);
        list->dlistSize = mapBlockReadU16(src, 4);
        list->minX = mapBlockReadS16(src, 6);
        list->minY = mapBlockReadS16(src, 8);
        list->minZ = mapBlockReadS16(src, 0x0a);
        list->maxX = mapBlockReadS16(src, 0x0c);
        list->maxY = mapBlockReadS16(src, 0x0e);
        list->maxZ = mapBlockReadS16(src, 0x10);
        list->flags = src[0x12];
        list->shaderIndex = src[0x13];
        list->renderBitOffset = mapBlockReadU16(src, 0x14);
        memcpy(list->pad16, src + 0x16, sizeof(list->pad16));
        list->selector = src[0x18];
        memcpy(list->pad19, src + 0x19, sizeof(list->pad19));
    }
}

static void mapBlockUnpackPolygons(MapBlockData* block)
{
    MapTriIndex* polygons = (MapTriIndex*)((u8*)block + (uintptr_t)block->gcPolygons);
    MapTriGroup* groups = (MapTriGroup*)((u8*)block + (uintptr_t)block->polygonGroups);
    u32 i;
    int j;
    for (i = 0; i < block->nPolygons; i++)
    {
        for (j = 0; j < 3; j++)
            polygons[i].vert[j] = fhSwap16(polygons[i].vert[j]);
        polygons[i].cellMask = fhSwap16(polygons[i].cellMask);
    }
    for (i = 0; i <= block->polyGroupCount; i++)
    {
        groups[i].firstTri = fhSwap16(groups[i].firstTri);
        groups[i].minX = (s16)fhSwap16((u16)groups[i].minX);
        groups[i].maxX = (s16)fhSwap16((u16)groups[i].maxX);
        groups[i].minY = (s16)fhSwap16((u16)groups[i].minY);
        groups[i].maxY = (s16)fhSwap16((u16)groups[i].maxY);
        groups[i].minZ = (s16)fhSwap16((u16)groups[i].minZ);
        groups[i].maxZ = (s16)fhSwap16((u16)groups[i].maxZ);
        groups[i].flags = fhSwap32(groups[i].flags);
    }
}

static void mapBlockUnpackFile(MapBlockData* block, u8* raw, u32 rawOffset, u32 allocationSize,
                               u32 textureOffset, u32 displayListOffset, u32 shaderOffset)
{
    u32 i;
    u32 textureDataOffset = mapBlockReadU32(raw, 0x54);
    u32 displayListDataOffset = mapBlockReadU32(raw, 0x68);
    u32 shaderDataOffset = mapBlockReadU32(raw, 0x64);
    MapTextureRef* textures = (MapTextureRef*)((u8*)block + textureOffset);
    MapBlockBoundsRec* displayLists = (MapBlockBoundsRec*)((u8*)block + displayListOffset);
    Shader* shaders = (Shader*)((u8*)block + shaderOffset);

    memset(block, 0, rawOffset);
    block->flags4 = mapBlockReadU16(raw, 4);
    block->unk6 = mapBlockReadU16(raw, 6);
    block->size = allocationSize;
    for (i = 0; i < 12; i++)
        ((f32*)block->transform)[i] = mapBlockReadF32(raw, 0x0c + i * 4);
    memcpy(block->pad3C, raw + 0x3c, sizeof(block->pad3C));
    block->gcPolygons = (void*)mapBlockDataOffset(raw, 0x4c, rawOffset);
    block->polygonGroups = (void*)mapBlockDataOffset(raw, 0x50, rawOffset);
    block->textures = textureDataOffset == 0 ? NULL : (MapTextureRef*)(uintptr_t)textureOffset;
    block->vertices = (u8*)mapBlockDataOffset(raw, 0x58, rawOffset);
    block->vertexColors = (void*)mapBlockDataOffset(raw, 0x5c, rawOffset);
    block->vertexTexCoords = (void*)mapBlockDataOffset(raw, 0x60, rawOffset);
    block->shaders = shaderDataOffset == 0 ? NULL : (Shader*)(uintptr_t)shaderOffset;
    block->displayLists = (MapBlockBoundsRec*)(uintptr_t)displayListOffset;
    block->renderInstrsMain = (void*)mapBlockDataOffset(raw, 0x78, rawOffset);
    block->renderInstrsTransp = (void*)mapBlockDataOffset(raw, 0x7c, rawOffset);
    block->renderInstrsWater = (void*)mapBlockDataOffset(raw, 0x80, rawOffset);
    block->nRenderInstrsMain = mapBlockReadU16(raw, 0x84);
    block->nRenderInstrsTransp = mapBlockReadU16(raw, 0x86);
    block->nRenderInstrsWater = mapBlockReadU16(raw, 0x88);
    block->minY = mapBlockReadS16(raw, 0x8a);
    block->maxY = mapBlockReadS16(raw, 0x8c);
    block->collisionYOffset = mapBlockReadS16(raw, 0x8e);
    block->vertexCount = mapBlockReadU16(raw, 0x90);
    block->unk92 = mapBlockReadU16(raw, 0x92);
    block->colorCount = mapBlockReadU16(raw, 0x94);
    block->texCoordCount = mapBlockReadU16(raw, 0x96);
    block->nPolygons = mapBlockReadU16(raw, 0x98);
    block->polyGroupCount = mapBlockReadU16(raw, 0x9a);
    block->hitCount = mapBlockReadU16(raw, 0x9c);
    block->unk9E = mapBlockReadU16(raw, 0x9e);
    block->textureCount = raw[0xa0];
    block->displayListCount = raw[0xa1];
    block->shaderCount = raw[0xa2];
    block->padA3 = raw[0xa3];

    for (i = 0; i < block->textureCount; i++)
        textures[i].fileId = (s32)mapBlockReadU32(raw, textureDataOffset + i * 4);
    mapBlockUnpackDisplayLists(displayLists, raw, displayListDataOffset, block->displayListCount, rawOffset);
    mapBlockUnpackShaders(shaders, raw, shaderDataOffset, block->shaderCount);
    mapBlockUnpackPolygons(block);
    fhSwapU16Array((u8*)block + (uintptr_t)block->vertices, (u32)block->vertexCount * 3);
    fhSwapU16Array((u8*)block + (uintptr_t)block->vertexColors, block->colorCount);
    fhSwapU16Array((u8*)block + (uintptr_t)block->vertexTexCoords, (u32)block->texCoordCount * 2);
}

MapBlockData* MapBlock_loadFromFile(int blockId)
{
    int compressedLen;
    int decompressedSize;
    u8* raw;
    MapBlockData* block;
    u32 textureOffset;
    u32 displayListOffset;
    u32 shaderOffset;
    u32 rawOffset;
    u32 allocationSize;
    int blockOff = 0;
    int* table;
    int tableEntry;
    if (blockId <= gMapBlockIndexCount)
    {
        table = gMapBlockIndexList;
        if (table != 0)
        {
            tableEntry = table[blockId];
            if (tableEntry != -1)
            {
                if (tableEntry != 0 || table[blockId + 1] != 0)
                {
                    blockOff = tableEntry;
                    checkLoadBlock(tableEntry, &compressedLen, &decompressedSize);
                }
                else
                {
                    return 0;
                }
            }
        }
    }
    else
    {
        return 0;
    }
    if (compressedLen <= 0)
    {
        return 0;
    }
    if (decompressedSize > 0x32000)
    {
        return 0;
    }
    raw = mmAlloc(decompressedSize, 5, 0);
    if (raw == 0)
    {
        return 0;
    }
    loadAndDecompressDataFile(MLDF_FILEID_BLOCKS_BIN_A, raw, blockOff, compressedLen, 0, 0, 0);
    extern void fh_model_lod_prepare(void*, unsigned, unsigned);
    fh_model_lod_prepare(raw, decompressedSize, 0x80000000u | (unsigned)blockId);
    if (decompressedSize < 0xa4)
    {
        mm_free(raw);
        return 0;
    }
    textureOffset = (sizeof(MapBlockData) + 15) & ~15;
    displayListOffset = (textureOffset + raw[0xa0] * sizeof(MapTextureRef) + 15) & ~15;
    shaderOffset = (displayListOffset + raw[0xa1] * sizeof(MapBlockBoundsRec) + 15) & ~15;
    rawOffset = (shaderOffset + raw[0xa2] * sizeof(Shader) + 15) & ~15;
    allocationSize = rawOffset + decompressedSize;
    block = mmAlloc(allocationSize, 5, 0);
    if (block == NULL)
    {
        mm_free(raw);
        return 0;
    }
    fh_native_scene_register(block, raw, decompressedSize, 0x80000000u | (unsigned)blockId);
    memcpy((u8*)block + rawOffset, raw, decompressedSize);
    mapBlockUnpackFile(block, (u8*)block + rawOffset, rawOffset, allocationSize,
                       textureOffset, displayListOffset, shaderOffset);
    mm_free(raw);
    return block;
}

void mapBlockGpuRecoveryHook(void)
{
    int n;
    int i;

    i = 0;
    n = gMapBlockCount;
    for (; i < n; i++)
    {
    }
}

void* mapBlockGetUnused00Value(MapBlockData* block)
{
    return NULL;
}

void mapGetBlocks(void** outLayerTables, uintptr_t* outBlocks)
{
    *outLayerTables = gMapBlockLayerTables;
    *outBlocks = (uintptr_t)gMapBlocks;
}

void mapClearBlockEdgeFlags(void)
{
    int i;
    int j;
    MapBlockData* block;

    for (i = 0; i < gMapBlockCount; i++)
    {
        block = gMapBlocks[i];
        if (block != NULL)
        {
            for (j = 0; j < block->displayListCount; j++)
            {
                block->displayLists[j].flags = 0;
            }
        }
    }
}

int collectShadowTrackTriangles(GameObject* obj, uintptr_t triBuf, void* planesOut, uintptr_t vertsOut, int unusedTriangleCount,
                                f32 offX, f32 offZ, int unusedRenderMode, int kindSelector)
{
    int j;
    f32 lm[12];
    TrackBlockDescriptor* desc = (TrackBlockDescriptor*)trackGetBlockDescriptors((u32*)&j);
    TrackBlockDescriptor* end = desc + j;
    int total;
    int grp;
    int outOff;
    int triangleFlag;

    outOff = 0;
    j = grp = 0;
    total = 0;
    triangleFlag = kindSelector ? 4 : 8;
    for (; desc < end; desc++)
    {
        void* id = desc->object;
        if (id == NULL || id == obj->anim.parent)
        {
            f32 fx = obj->anim.localPosX;
            f32 fz = obj->anim.localPosZ;
            TrackShadowTriangle* outA;

            if (id == NULL)
            {
                fx -= offX;
                fz -= offZ;
            }
            j = (s16)desc->firstTriangle;
            outA = (TrackShadowTriangle*)((char*)planesOut + outOff);
            while (j < (s16)desc[1].firstTriangle && grp < 0x4b0 && total < 0xe10)
            {
                if (triangleFlag & ((TrackTriangle*)triBuf + j)->flags)
                {
                    ((TrackP6Entry*)vertsOut)->relX0 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vx[0]) - fx;
                    ((TrackP6Entry*)vertsOut)->relY0 =
                        __OSs16tof32(&((TrackTriangle*)triBuf + j)->vy[0]) - obj->anim.localPosY;
                    ((TrackP6Entry*)vertsOut)->relZ0 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vz[0]) - fz;
                    ((TrackP6Entry*)vertsOut)->relX1 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vx[1]) - fx;
                    ((TrackP6Entry*)vertsOut)->relY1 =
                        __OSs16tof32(&((TrackTriangle*)triBuf + j)->vy[1]) - obj->anim.localPosY;
                    ((TrackP6Entry*)vertsOut)->relZ1 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vz[1]) - fz;
                    ((TrackP6Entry*)vertsOut)->relX2 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vx[2]) - fx;
                    ((TrackP6Entry*)vertsOut)->relY2 =
                        __OSs16tof32(&((TrackTriangle*)triBuf + j)->vy[2]) - obj->anim.localPosY;
                    ((TrackP6Entry*)vertsOut)->relZ2 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vz[2]) - fz;
                    outA->normal.x = ((TrackTriangle*)triBuf + j)->planeN[0];
                    outA->normal.y = ((TrackTriangle*)triBuf + j)->planeN[1];
                    outA->normal.z = ((TrackTriangle*)triBuf + j)->planeN[2];
                    outA->flags = ((TrackTriangle*)triBuf + j)->flags;
                    vertsOut += 0x24;
                    total += 3;
                    outA++;
                    grp += 1;
                    outOff += 0x14;
                }
                j++;
            }
        }
        else
        {
            f32* m = (f32*)desc->currentMatrix;
            f32* p6start;
            int totalStart;
            TrackShadowTriangle* outA;

            lm[0] = m[0];
            lm[1] = m[4];
            lm[2] = m[8];
            lm[3] = m[12] - obj->anim.localPosX;
            lm[4] = m[1];
            lm[5] = m[5];
            lm[6] = m[9];
            lm[7] = m[13] - obj->anim.localPosY;
            lm[8] = m[2];
            lm[9] = m[6];
            lm[10] = m[10];
            lm[11] = m[14] - obj->anim.localPosZ;
            p6start = (f32*)vertsOut;
            totalStart = total;
            j = (s16)desc->firstTriangle;
            outA = (TrackShadowTriangle*)((char*)planesOut + outOff);
            while (j < (s16)desc[1].firstTriangle && grp < 0x4b0 && total < 0xe10)
            {
                if (triangleFlag & ((TrackTriangle*)triBuf + j)->flags)
                {
                    ((TrackP6Entry*)vertsOut)->relX0 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vx[0]);
                    ((TrackP6Entry*)vertsOut)->relY0 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vy[0]);
                    ((TrackP6Entry*)vertsOut)->relZ0 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vz[0]);
                    ((TrackP6Entry*)vertsOut)->relX1 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vx[1]);
                    ((TrackP6Entry*)vertsOut)->relY1 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vy[1]);
                    ((TrackP6Entry*)vertsOut)->relZ1 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vz[1]);
                    ((TrackP6Entry*)vertsOut)->relX2 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vx[2]);
                    ((TrackP6Entry*)vertsOut)->relY2 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vy[2]);
                    ((TrackP6Entry*)vertsOut)->relZ2 = __OSs16tof32(&((TrackTriangle*)triBuf + j)->vz[2]);
                    outA->normal.x = ((TrackTriangle*)triBuf + j)->planeN[0];
                    outA->normal.y = ((TrackTriangle*)triBuf + j)->planeN[1];
                    outA->normal.z = ((TrackTriangle*)triBuf + j)->planeN[2];
                    outA->flags = ((TrackTriangle*)triBuf + j)->flags;
                    vertsOut += 0x24;
                    total += 3;
                    outA++;
                    grp += 1;
                    outOff += 0x14;
                }
                j++;
            }
            if (totalStart < total)
            {
                PSMTXMultVecArray((MtxPtr)lm, (Vec*)p6start, (Vec*)p6start, total - totalStart);
            }
        }
    }
    return grp;
}

FrustumPlane gViewFrustumPlanes[FRUSTUM_PLANE_COUNT];
FrustumPlane gPlayerRelativeFrustumPlanes[FRUSTUM_PLANE_COUNT];
