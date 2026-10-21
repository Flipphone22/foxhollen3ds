#include "fh_pica_diffuse_map.h"
#include "fh_pica_specular.h"
#include "fh_graphics_options.h"
#include "fh_minimal_graphics.h"
#include "fh_platform.h"
#include "fh_pica_scene.h"
#include "fh_pica_scene.h"
#include "fh_minimal_graphics.h"
#include "fh_render_floor.h"
#include "dolphin/math.h"
#include "main/texture.h"
#include "game/objects/object.h"
#include "main/model_light.h"
#include "main/model.h"
#include "main/newclouds.h"
#include "main/rcp_dolphin.h"
#include "main/rcp_dolphin_render.h"
#include "main/camera.h"
#include "main/pi_dolphin.h"
#include "main/screen_transition.h"
#include "main/sky.h"
#include "main/sky_interface.h"
#include "main/mm.h"
#include "main/dll/dll_0000_gameui.h"
#include "dolphin/os/OSCache.h"
#include "dolphin/mtx.h"
#include "dolphin/gx/GXDispList.h"
#include "dolphin/gx/GXFrameBuffer.h"
#include "dolphin/gx/GXGet.h"
#include "dolphin/gx/GXGeometry.h"
#include "dolphin/gx/GXLighting.h"
#include "dolphin/gx/GXManage.h"
#include "dolphin/gx/GXPixel.h"
#include "dolphin/gx/GXTev.h"
#include "dolphin/gx/GXTexture.h"
#include "dolphin/gx/GXTransform.h"
#include "main/pi_dolphin_texture.h"
#include "main/gx_scissor.h"
#include "dolphin/gx/GXCull.h"
#include "track/intersect_depth_state.h"
#include "track/intersect_hud_color.h"
#include "main/shader_init.h"

u8 gRcpDistortSlotIndex;
u8 gRcpDistortGroup;
void* gRcpDistortTexture;
u32 gRcpWarpDistortListSize;
u8 gRcpWarpDistortListBuilt;

GXColor gRcpDistortAmbColor = {0, 0, 0, 0};
GXColor gRcpDistortMatColor = {0xff, 0xff, 0xff, 0xff};
typedef struct RcpDistortSlot {
    u8* texture;       // 0x00
    GameObject* model; // 0x04
    int unk8;          // 0x08
    u8 colR;           // 0x0c
    u8 colG;           // 0x0d
    u8 colB;           // 0x0e
    u8 unkF;           // 0x0f
    f32 params[2];     // 0x10
    u8 scaleR;         // 0x18
    u8 scaleB;         // 0x19
    u8 group;          // 0x1a
    u8 mode;           // 0x1b
} RcpDistortSlot;
STATIC_ASSERT(sizeof(RcpDistortSlot) == 0x1c);
STATIC_ASSERT(offsetof(RcpDistortSlot, model) == 0x04);
STATIC_ASSERT(offsetof(RcpDistortSlot, params) == 0x10);
STATIC_ASSERT(offsetof(RcpDistortSlot, group) == 0x1a);
STATIC_ASSERT(offsetof(RcpDistortSlot, mode) == 0x1b);

extern RcpDistortSlot gRcpDistortSlots[6];
static const f32 gRcpScreenWidth = 640.0f;
static const f32 gRcpScreenHeight = 480.0f;
void* textureAlloc(u16 w, u16 h, int fmt, u8 mip, u8 maxLod, u8 wrapS, u8 wrapT, u8 minFilter, u8 magFilter);
static inline void gxLoadObjectLights(GameObject* model, ModelLightStruct** lights);

#define RCP_DISTORT_TEXTURE_ID 0x5dc

extern u8 gRcpWarpDistortDisplayList[0x6640] __attribute__((aligned(32)));

static void Rcp_SetupDistortionRenderState(void);

static inline void gxLoadObjectLights(GameObject* model, ModelLightStruct** lights) {
    s32 count;
    int n;
    modelLightStruct_selectObjectLights(model, lights, 8, &count, 4);
    modelLightChannels_reset(1);
    modelLightChannel_configure(0, 0, 0);
    for (n = 0; n < count; n++) {
        modelLightStruct_loadChannelLight(0, lights[n], model);
    }
    modelLightChannels_applyGXControls();
}

static int Rcp_SetupDistortionLights(GameObject* model, f32* params);

static void Rcp_DrawWarpDistortionMesh(f32 a, f32 b) /* params unused; callers pass (i*32, 0.0f) */
{
    f32 x0;
    f32 y;
    f32 ySq;
    f32 step;
    f32 half;
    f32 x1;
    f32 span;
    f32 distSq;
    f32 bulge;
    f32 col0;
    f32 col1;
    f32 meshZ;
    u32 i;
    u32 j;

    if (gRcpWarpDistortListBuilt == 0) {
        GXSetMisc(GX_MT_XF_FLUSH, 0);
        DCInvalidateRange(gRcpWarpDistortDisplayList, 0x6640);
        GXBeginDisplayList(gRcpWarpDistortDisplayList, 0x6640);
        i = 0;
        span = 15.0f;
        half = 1.0f;
        step = 2.0f;
        meshZ = -2.0f;
        for (; i < 0x10; i++) {
            GXBegin(GX_TRIANGLESTRIP, GX_VTXFMT4, 0x22);
            j = 0;
            for (; j <= 0x10; j++) {
                col0 = step * (f32)i;
                col1 = step * (f32)(i + 1);
                x0 = col0 / span - half;
                x1 = col1 / span - half;
                y = (step * (f32)j) / span - half;
                ySq = y * y;
                distSq = x0 * x0 + ySq;
                if (distSq < half) {
                    bulge = sqrtf(half - distSq);
                } else {
                    bulge = 0.0f;
                }
                GXPosition3f32(x0, y, meshZ);
                GXNormal3f32(x0, y, bulge);
                distSq = x1 * x1 + ySq;
                if (distSq < half) {
                    bulge = sqrtf(half - distSq);
                } else {
                    bulge = 0.0f;
                }
                GXPosition3f32(x1, y, meshZ);
                GXNormal3f32(x1, y, bulge);
            }
        }
        gRcpWarpDistortListSize = GXEndDisplayList();
        gRcpWarpDistortListBuilt = 1;
        GXSetMisc(GX_MT_XF_FLUSH, 8);
    }
    GXCallDisplayList(gRcpWarpDistortDisplayList, gRcpWarpDistortListSize);
}
static int Rcp_SetupDistortionLights(GameObject* model, f32* params) {
    ModelLightStruct* la;
    ModelLightStruct* lb;
    la = skyGetSunLight();
    lb = skyGetMoonLight();
    if (la == NULL || lb == NULL) {
        return 0;
    }
    modelLightChannels_reset(1);
    modelLightChannel_configure(0, 1, 0);
    modelLightChannel_configure(2, 0, 0);
    modelLightStruct_setSpecularAttenuation(la, params[0], 0.0f);
    modelLightStruct_setSpecularColor(la, 0xff, 0, 0, 0xff);
    modelLightStruct_loadChannelLight(0, la, model);
    modelLightStruct_setSpecularAttenuation(la, params[1], 0.0f);
    modelLightStruct_setSpecularColor(la, 0, 0, 0xff, 0xff);
    modelLightStruct_loadChannelLight(0, la, model);
    modelLightStruct_setAngularAttenuation(la, 1.5f, 0.0f, 0.0f);
    modelLightStruct_loadChannelLight(2, la, model);
    modelLightChannel_configure(1, 1, 0);
    modelLightChannel_configure(3, 0, 0);
    modelLightStruct_setSpecularAttenuation(lb, params[0], 0.0f);
    modelLightStruct_setSpecularColor(lb, 0xff, 0, 0, 0xff);
    modelLightStruct_loadChannelLight(1, lb, model);
    modelLightStruct_setSpecularAttenuation(lb, params[1], 0.0f);
    modelLightStruct_setSpecularColor(lb, 0, 0, 0xff, 0xff);
    modelLightStruct_loadChannelLight(1, lb, model);
    modelLightStruct_setAngularAttenuation(lb, 0.5f, 0.0f, 0.0f);
    modelLightStruct_loadChannelLight(3, lb, model);
    modelLightChannels_applyGXControls();
    modelLightStruct_setAngularAttenuation(la, 1.0f, 0.0f, 0.0f);
    modelLightStruct_setAngularAttenuation(lb, 1.0f, 0.0f, 0.0f);
    return 0;
}
static void Rcp_SetupDistortionRenderState(void) {
    f32 omtx[4][4];
    f32 pmtx[3][4];
    GXSetViewport(0.0f, 0.0f, 32.0f, 32.0f, 0.0f, 1.0f);
    GXSetScissor(0, 0, 32, 32);
    GXSetDispCopySrc(0, 0, 32, 32);
    GXSetDispCopyDst(32, 32);
    GXSetTexCopySrc(0, 0, 32, 32);
    C_MTXOrtho(omtx, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, 15.0f);
    GXSetProjection(omtx, GX_ORTHOGRAPHIC);
    GXSetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_NOOP);
    gxSetZMode_(0, GX_EQUAL, 0);
    GXSetCullMode(GX_CULL_NONE);
    gxSetPeControl_ZCompLoc_(1);
    GXSetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GXClearVtxDesc();
    GXSetVtxDesc(GX_VA_POS, GX_DIRECT);
    GXSetVtxDesc(GX_VA_NRM, GX_DIRECT);
    PSMTXIdentity(pmtx);
    GXLoadPosMtxImm(pmtx, GX_PNMTX0);
    GXLoadNrmMtxImm(pmtx, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

void Rcp_UpdateDistortionTextures(void) {
    union {
        Mtx m;
    } mtxu;
#define mtx mtxu.m
    ModelLightStruct* lights[8];
    GXColor outColor;
    GXColor texColor;
    GXColor matColor;
    RcpDistortSlot* e;
    u8* slots[1];
    int i;
    int clearSlot;
    u8 group;
    int k;
    GameObject* model[1];
    Texture* tex;

    Rcp_SetupDistortionRenderState();
    PSMTXScale(mtx, 0.5f, -0.5f, 0.5f);
    mtx[0][3] = 0.5f;
    mtx[1][3] = 0.5f;
    GXLoadTexMtxImm(mtx, GX_TEXMTX0, GX_MTX2x4);
    GXSetChanAmbColor(GX_COLOR0A0, gRcpDistortAmbColor);
    GXSetChanAmbColor(GX_COLOR1A1, gRcpDistortAmbColor);
    GXSetTexCopyDst(0x20, 0x20, GX_TF_RGBA8, GX_FALSE);
    skyApplyLightSlot(2);
    i = 0;
    slots[0] = (u8*)gRcpDistortSlots;
    for (; i < 6; i++) {
        tex = (Texture*)((RcpDistortSlot*)slots[0])[i].texture;
        if (tex->refCount != 0 && ((RcpDistortSlot*)slots[0])[i].mode == 1 &&
            gRcpDistortGroup == ((RcpDistortSlot*)slots[0])[i].group) {
            matColor.r = (((RcpDistortSlot*)slots[0])[i].colR * ((RcpDistortSlot*)slots[0])[i].scaleR) >> 8;
            matColor.g = 0;
            matColor.b = (((RcpDistortSlot*)slots[0])[i].colB * ((RcpDistortSlot*)slots[0])[i].scaleB) >> 8;
            matColor.a = 0xff;
            GXSetChanMatColor(GX_COLOR0A0, matColor);
            GXSetChanMatColor(GX_COLOR1A1, matColor);
            Rcp_SetupDistortionLights(((RcpDistortSlot*)slots[0])[i].model, ((RcpDistortSlot*)slots[0])[i].params);
            Rcp_ResetTextureStageState();
            addSphereMapLitStages(gRcpDistortTexture, (f32*)mtx, &texColor, 0);
            Rcp_ApplyTextureStageCounts();
            Rcp_DrawWarpDistortionMesh((f32)(i * 0x20), 0.0f);
            GXCopyTex(((RcpDistortSlot*)slots[0])[i].texture + sizeof(Texture), 0);
            tex = (Texture*)((RcpDistortSlot*)slots[0])[i].texture;
            if (tex->preloaded != 0) {
                GXPreLoadEntireTexture(textureGetGXTexObj((Texture*)tex), textureGetGXTexRegion((Texture*)tex));
            }
        }
    }
    Rcp_ResetTextureStageState();
    addVertexColorKAlphaStage(&gRcpDistortMatColor);
    Rcp_ApplyTextureStageCounts();
    GXSetChanMatColor(GX_COLOR0, gRcpDistortMatColor);
    clearSlot = 5;
    k = 5;
    e = &gRcpDistortSlots[5];
    group = gRcpDistortGroup;
    for (; k >= 0; k--) {
        if (((Texture*)e->texture)->refCount != 0 && e->mode == 0 && group == e->group) {
            clearSlot = k;
            break;
        }
        e--;
    }
    i = 0;
    for (; i < 6; i++) {
        if (((Texture*)((RcpDistortSlot*)slots[0])[i].texture)->refCount != 0 &&
            ((RcpDistortSlot*)slots[0])[i].mode == 0 && gRcpDistortGroup == ((RcpDistortSlot*)slots[0])[i].group) {
            model[0] = ((RcpDistortSlot*)slots[0])[i].model;
            skyApplyLightSlot(2 - (i - 3));
            gxLoadObjectLights(model[0], lights);
            lightGetColor(0, &outColor.r, &outColor.g, &outColor.b);
            GXSetChanAmbColor(GX_COLOR0, outColor);
            if (fh_minimal_enabled) {
                FhTexture destination;
                Texture* output=(Texture*)((RcpDistortSlot*)slots[0])[i].texture;
                if (fh_gx_texture_view(textureGetGXTexObj(output),&destination))
                    fh_minimal_sphere_update(i,&destination);
            } else {
            Rcp_DrawWarpDistortionMesh((f32)(i * 0x20), 0.0f);
            GXCopyTex(((RcpDistortSlot*)slots[0])[i].texture + sizeof(Texture), (i == clearSlot) ? GX_TRUE : GX_FALSE);
            }
            tex = (Texture*)((RcpDistortSlot*)slots[0])[i].texture;
            if (tex->preloaded != 0) {
                GXPreLoadEntireTexture(textureGetGXTexObj((Texture*)tex), textureGetGXTexRegion((Texture*)tex));
            }
        }
    }
    GXSetViewport(0.0f, 0.0f, gRcpScreenWidth, gRcpScreenHeight, 0.0f, 1.0f);
    GXSetScissor(0, 0, 0x280, 0x1e0);
    GXSetDispCopySrc(0, 0, 0x280, 0x1e0);
    GXSetDispCopyDst(0x280, 0x1e0);
    GXSetTexCopySrc(0, 0, 0x280, 0x1e0);
    Camera_ApplyFullViewport();
    gRcpDistortGroup = 0;
}

void Rcp_DebugDrawDistortionTexture(void) {
    u32 color = 0xffffffff;
    hudDrawColored((Texture*)gRcpDistortSlots[2].texture, 8, 8, &color, 0x800, 0);
}

void ShaderDef_free(void** def) {
    Texture* s;
    void* p1 = def[0];
    int i;
    void* p2;
    int j;

    if (p1 != NULL) {
        for (i = 0; i < 6; i++) {
            s = (Texture*)gRcpDistortSlots[i].texture;
            if (s->refCount != 0 && s == p1) {
                (((Texture*)gRcpDistortSlots[i].texture)->refCount)--;
                break;
            }
        }
    }
    p2 = def[1];
    if (p2 == NULL) {
        return;
    }
    for (j = 0; j < 6; j++) {
        if (((Texture*)gRcpDistortSlots[j].texture)->refCount != 0 && gRcpDistortSlots[j].texture == p2) {
            (((Texture*)gRcpDistortSlots[j].texture)->refCount)--;
            return;
        }
    }
}

void shaderInit(u8* def, ModelRenderOpTextureRefs* textures, GameObject* obj, int shaderFlags) {
    RcpDistortSlot* slot;
    Texture* s;

    if (((Shader*)def)->reg1Texture != NULL) {
        if (obj != NULL) {
            slot = &gRcpDistortSlots[6 - (obj->lightColorSlot + 1)];
        } else {
            slot = &gRcpDistortSlots[5];
        }
        s = (Texture*)slot->texture;
        (s->refCount)++;
        textures->texture0 = slot->texture;
    }
    if (((Shader*)def)->reg2Texture == NULL) {
        return;
    }
    if (((Shader*)def)->reg2TexSlot >= 6) {
        slot = gRcpDistortSlots;
    } else {
        slot = &gRcpDistortSlots[((Shader*)def)->reg2TexSlot >> 1];
    }
    s = (Texture*)slot->texture;
    (s->refCount)++;
    textures->texture1 = slot->texture;
}

typedef struct RcpDistortConfig {
    f32 radius;
    f32 strength;
} RcpDistortConfig;
extern RcpDistortConfig gRcpDistortConfigs[6];

void Rcp_InitDistortionEffects(void) {
    int i;
    RcpDistortSlot* slots;
    f32* cfg;
    u32 pairIdx;
    RcpDistortSlot* slot;
    f32 strengthScale;
    f32 radiusScale;
    f32 strength;
    f32 falloff;

    i = 0;
    slots = gRcpDistortSlots;
    for (; i < 6; i++) {
        slots[i].texture = (u8*)textureAlloc(0x20, 0x20, 6, 0, 0, 0, 0, 1, 1);
        slots[i].group = 0;
    }
    gRcpDistortSlotIndex = i = 0;
    cfg = &gRcpDistortConfigs[0].radius;
    slots = gRcpDistortSlots;
    radiusScale = 2.146452f;
    strengthScale = 255.0f;
    do {
        strength = cfg[i * 2 + 1];
        (slot = &slots[gRcpDistortSlotIndex])->colR = 0xff;
        slot->colG = 0xff;
        slot->colB = 0xff;
        falloff = radiusScale / powfCoreHighPrecision(cfg[i * 2], 2.520326f);
        slot = &slots[gRcpDistortSlotIndex];
        pairIdx = i & 1;
        slot->params[pairIdx] = falloff;
        *(s8*)(&slot->scaleR + pairIdx) = strengthScale * strength;
        slot->mode = 1;
        if (pairIdx != 0) {
            gRcpDistortSlotIndex = gRcpDistortSlotIndex + 1;
        }
        i++;
    } while (i < 6);
    /* mode = 0 for the three remaining slots */
    gRcpDistortSlots[gRcpDistortSlotIndex++].mode = 0;
    gRcpDistortSlots[gRcpDistortSlotIndex++].mode = 0;
    gRcpDistortSlots[gRcpDistortSlotIndex++].mode = 0;
    gRcpDistortTexture = textureLoadAsset(RCP_DISTORT_TEXTURE_ID);
    if(fh_pica_scene_actors) {
        FhTexture image;
        if(!fh_gx_texture_view(textureGetGXTexObj(gRcpDistortTexture),&image))
            fh_platform_panic("PICA specular source texture view");
        fh_pica_specular_prepare(&image);
        fh_pica_specular_prepare_half();
        fh_pica_diffuse_map_prepare();
    }

}
RcpDistortConfig gRcpDistortConfigs[6] __attribute__((aligned(8))) = {
    {0.5f, 1.0f}, {0.5f, 0.5f}, {0.4f, 1.0f}, {0.3f, 0.8f}, {0.2f, 1.0f}, {0.4f, 0.5f},
};
RcpDistortSlot gRcpDistortSlots[6];
u8 gRcpWarpDistortDisplayList[0x6640] __attribute__((aligned(32)));

_Static_assert(__alignof__(gRcpDistortConfigs) >= 8, "Native alignment: gRcpDistortConfigs");

_Static_assert(__alignof__(gRcpWarpDistortDisplayList) >= 32, "Native alignment: gRcpWarpDistortDisplayList");


#include "main/shader.h"

static void fh_pica_game_specular(void) {
    if(!(fh_graphics_options&FH_GRAPHICS_SPECULAR) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ACTOR_TEX)))return;
    FhPicaSpecularState states[3]={0};unsigned mask=0;
    skyApplyLightSlot(2);
    ModelLightStruct *lights[2]={skyGetSunLight(),skyGetMoonLight()};
    if(!lights[0] || !lights[1])return;
    /* Sky's source sun/moon are directional. Do not treat arbitrary point
     * lights as these maps, nor mutate the lights' original game state. */
    if(lights[0]->lightKind!=4 || lights[1]->lightKind!=4)
        fh_platform_panic("PICA specular non-directional sky light");
    for(unsigned i=0;i<3;i++) {
        RcpDistortSlot *slot=gRcpDistortSlots+i;
        if(!slot->texture || !((Texture*)slot->texture)->refCount ||
           slot->mode!=1 || slot->group!=gRcpDistortGroup)continue;
        FhPicaSpecularState *s=states+i;mask|=1u<<i;
        s->attenuation[0]=slot->params[0]*.5f;s->attenuation[1]=1.f-s->attenuation[0];
        s->attenuation[2]=slot->params[1]*.5f;s->attenuation[3]=1.f-s->attenuation[2];
        s->material[0]=(u8)((slot->colR*slot->scaleR)>>8)/255.f;
        s->material[2]=(u8)((slot->colB*slot->scaleB)>>8)/255.f;
        s->material[3]=1;
        for(unsigned c=0;c<2;c++) {
            ModelLightStruct *l=lights[c];
            s->direction[c][0]=-l->viewDirX;s->direction[c][1]=-l->viewDirY;s->direction[c][2]=-l->viewDirZ;
            Vec half={-l->viewDirX,-l->viewDirY,1.f-l->viewDirZ};
            float q=half.x*half.x+half.y*half.y+half.z*half.z;
            if(q>0){float scale=1.f/sqrtf(q);half.x*=scale;half.y*=scale;half.z*=scale;}
            s->half_direction[c][0]=half.x;s->half_direction[c][1]=half.y;s->half_direction[c][2]=half.z;
            s->diffuse_alpha[c]=(c?.5f:1.5f)*l->diffuseColor[3]/255.f;
        }
    }
    fh_pica_specular_update(mask,states);
}

void fh_pica_game_environment(void) {
    fh_pica_game_specular();
#if FH_PICA_ACTOR_LIGHTING_EXPERIMENT
    /* Match the source mode-0 slots, masks, group and light-color selection.
     * Native PICA receives live light parameters; no source GX state replay. */
    unsigned diffuse_enabled=(fh_graphics_options&FH_GRAPHICS_DIFFUSE_MAPS) &&
        !(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ENVIRONMENT|FH_GRAPHICS_NO_ACTOR_TEX));
    if(fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_ACTORS|
                          FH_GRAPHICS_NO_LIGHTING|FH_GRAPHICS_NO_ENVIRONMENT)) {
        /* Specular takes its independent snapshot before this function body.
         * Preserve the source group boundary even with no environment consumer. */
        gRcpDistortGroup=0;return;
    }
    FhPicaDiffuseMapState maps[3];unsigned diffuse_mask=0;
    for(unsigned i=3;i<6;i++) {
        RcpDistortSlot *slot=gRcpDistortSlots+i;
        if(!slot->texture || !((Texture*)slot->texture)->refCount ||
           slot->mode!=0 || slot->group!=gRcpDistortGroup)continue;
        FhPicaEnvironment state={0};ModelLightStruct *lights[8];s32 count=0;
        skyApplyLightSlot(5-i);
        modelLightStruct_selectObjectLights(slot->model,lights,8,&count,4);
        GXColor ambient={0};lightGetColor(0,&ambient.r,&ambient.g,&ambient.b);
        state.ambient[0]=ambient.r/255.f;state.ambient[1]=ambient.g/255.f;state.ambient[2]=ambient.b/255.f;
        state.count=count;
        for(unsigned j=0;j<(unsigned)count;j++) {
            ModelLightStruct *l=lights[j];
            state.direction[j][0]=-l->viewDirX;state.direction[j][1]=-l->viewDirY;state.direction[j][2]=-l->viewDirZ;
            for(unsigned c=0;c<3;c++)state.color[j][c]=l->diffuseColor[c]/255.f;
        }
        fh_pica_environment_update(5-i,&state);
        if(diffuse_enabled){
            FhPicaDiffuseMapState *map=maps+(5-i);memset(map,0,sizeof(*map));
            map->count=count;memcpy(map->ambient,state.ambient,3*sizeof(float));
            Vec raw={0},transformed={0};
            if(slot->model){
                raw.x=slot->model->anim.localPosX;raw.y=slot->model->anim.localPosY;raw.z=slot->model->anim.localPosZ;
                Vec world={raw.x-playerMapOffsetX,raw.y,raw.z-playerMapOffsetZ};
                PSMTXMultVec((MtxPtr)Camera_GetViewMatrix(),&world,&transformed);
            }
            for(unsigned j=0;j<(unsigned)count;j++){
                ModelLightStruct *l=lights[j];
                if(l->lightKind!=4)fh_platform_panic("PICA diffuse source light kind");
                const Vec *origin=l->transformMode?&raw:&transformed;
                map->position[j][0]=l->viewDirX*-100000.f+origin->x;
                map->position[j][1]=l->viewDirY*-100000.f+origin->y;
                map->position[j][2]=l->viewDirZ*-100000.f+origin->z;
                memcpy(map->color[j],state.color[j],3*sizeof(float));
            }
            diffuse_mask|=1u<<(5-i);
        }
    }
    if(diffuse_mask)fh_pica_diffuse_map_update(diffuse_mask,maps);
    gRcpDistortGroup=0;
#endif
}
