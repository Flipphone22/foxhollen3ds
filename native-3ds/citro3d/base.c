#include "internal.h"
#include <stdlib.h>
#include <c3d/base.h>
#include <c3d/renderqueue.h>
#include <c3d/effect.h>
#include <c3d/uniforms.h>

C3D_Context __C3D_Context;

static aptHookCookie hookCookie;
void fh_pica_stream_end(void);
unsigned long long fh_program_publication_stats[5];
/* Candidate for the hardware-only GS->HUD hang isolated by particle probe v2.
 * Track device publication, not transient CPU binds. No resource retirement,
 * presentation or explicit CPU completion wait belongs to this boundary. */
unsigned fh_geometry_boundary_enabled = 1;
unsigned fh_geometry_boundary_edges, fh_geometry_boundary_splits;
unsigned fh_geometry_boundary_separated;
static bool fh_geometry_device_enabled;
/* Adapt Melee's retained normal-draw raster state at the common publication
 * point so all Citro3D clients participate. Compare copied values, not pointers.
 * Only pure state is retained. Framebuffer operations, cache flushes, texture
 * binds, uniforms, lighting LUTs and draw commands keep their original paths. */
#include <string.h>
unsigned fh_raster_reuse_enabled=1;
unsigned long long fh_raster_reuse_counts[6][2]; /* considered, retained */
/* Pinned Melee copies sampler descriptors at bind time. Here retain only
 * their pure register publication: texture dirty bits, enable routing and
 * cache clears still run unchanged, including same-address dynamic images. */
unsigned fh_sampler_reuse_enabled=1;
unsigned long long fh_sampler_reuse_counts[2];
static struct {
    unsigned valid;
    u32 viewport[5],scissor[3],texEnvBuf[3];
    C3D_Effect effect;
    C3D_AttrInfo attrInfo;
    C3D_TexEnv texEnv[6];
    C3D_Tex sampler[3];
} fh_raster;
extern void fh_pica_buffers_invalidate(void);
static void fh_raster_invalidate(void){fh_pica_stream_end();fh_raster.valid=0;fh_pica_buffers_invalidate();}
/* All callers supply aligned SDK word blocks. These assertions also guard
 * future SDK layout changes. This shadow is CPU-private; equal-value stores
 * do not publish GPU state or touch command/resource memory. */
_Static_assert(sizeof(C3D_Effect)%4==0 && _Alignof(C3D_Effect)>=4,"raster effect words");
_Static_assert(sizeof(C3D_AttrInfo)%4==0 && _Alignof(C3D_AttrInfo)>=4,"raster attribute words");
_Static_assert(sizeof(C3D_TexEnv)%4==0 && _Alignof(C3D_TexEnv)>=4,"raster TEV words");
typedef u32 FhRasterWord __attribute__((may_alias));
static inline __attribute__((always_inline)) bool fh_raster_needed(unsigned slot,unsigned kind,void *saved,const void *value,size_t size){
    unsigned bit=1u<<slot;
    unsigned changed=!(fh_raster_reuse_enabled && (fh_raster.valid&bit));
    FhRasterWord *dst=saved;const FhRasterWord *src=value;
    ++fh_raster_reuse_counts[kind][0];
    for(unsigned i=0;i<size/4;i++){
        u32 next=src[i];changed|=dst[i]^next;dst[i]=next;
    }
    fh_raster.valid|=bit;
    if(!changed){++fh_raster_reuse_counts[kind][1];return false;}
    return true;
}
static bool fh_sampler_needed(unsigned unit,const C3D_Tex *value){
    unsigned bit=1u<<(11+unit);
    ++fh_sampler_reuse_counts[0];
    /* Cube descriptors contain a pointer to mutable face addresses, rather
     * than the addresses themselves. Leave every non-2D owner untouched. */
    if(C3D_TexGetType(value)!=GPU_TEX_2D){fh_raster.valid&=~bit;return true;}
    if(fh_sampler_reuse_enabled && (fh_raster.valid&bit) &&
       !memcmp(fh_raster.sampler+unit,value,sizeof(*value))){
        ++fh_sampler_reuse_counts[1];return false;
    }
    fh_raster.sampler[unit]=*value;fh_raster.valid|=bit;return true;
}

__attribute__((weak)) void C3Di_LightEnvUpdate(C3D_LightEnv* env)
{
	(void)env;
}

__attribute__((weak)) void C3Di_LightEnvDirty(C3D_LightEnv* env)
{
	(void)env;
}

__attribute__((weak)) void C3Di_ProcTexUpdate(C3D_Context* ctx)
{
	(void)ctx;
}

__attribute__((weak)) void C3Di_ProcTexDirty(C3D_Context* ctx)
{
	(void)ctx;
}

__attribute__((weak)) void C3Di_GasUpdate(C3D_Context* ctx)
{
	(void)ctx;
}

static void C3Di_AptEventHook(APT_HookType hookType, C3D_UNUSED void* param)
{
	C3D_Context* ctx = C3Di_GetContext();

	switch (hookType)
	{
		case APTHOOK_ONSUSPEND:
		{
			C3Di_RenderQueueWaitDone();
			C3Di_RenderQueueDisableVBlank();
			break;
		}
		case APTHOOK_ONRESTORE:
		{
            fh_raster_invalidate();
			C3Di_RenderQueueEnableVBlank();
			ctx->flags |= C3DiF_AttrInfo | C3DiF_BufInfo | C3DiF_Effect | C3DiF_FrameBuf
				| C3DiF_Viewport | C3DiF_Scissor | C3DiF_Program | C3DiF_VshCode | C3DiF_GshCode
				| C3DiF_TexAll | C3DiF_TexEnvBuf | C3DiF_TexEnvAll | C3DiF_LightEnv | C3DiF_Gas;

			C3Di_DirtyUniforms(GPU_VERTEX_SHADER);
			C3Di_DirtyUniforms(GPU_GEOMETRY_SHADER);

			ctx->fixedAttribDirty |= ctx->fixedAttribEverDirty;
			ctx->gasFlags |= C3DiG_BeginAcc | C3DiG_AccStage | C3DiG_RenderStage;

			C3D_LightEnv* env = ctx->lightEnv;
			if (ctx->fogLut)
				ctx->flags |= C3DiF_FogLut;
			if (ctx->gasLut)
				ctx->flags |= C3DiF_GasLut;
			if (env)
				C3Di_LightEnvDirty(env);
			C3Di_ProcTexDirty(ctx);
			break;
		}
		default:
			break;
	}
}

bool C3D_Init(size_t cmdBufSize)
{
	int i;
	C3D_Context* ctx = C3Di_GetContext();

	if (ctx->flags & C3DiF_Active)
		return false;

	cmdBufSize = (cmdBufSize + 0xF) &~ 0xF; // 0x10-byte align
	ctx->cmdBufSize = cmdBufSize/4;
	ctx->cmdBuf = (u32*)linearAlloc(cmdBufSize);
	ctx->cmdBufUsage = 0;
	fh_geometry_device_enabled = false;
	if (!ctx->cmdBuf)
		return false;

	ctx->gxQueue.maxEntries = 32;
	ctx->gxQueue.entries = (gxCmdEntry_s*)malloc(ctx->gxQueue.maxEntries*sizeof(gxCmdEntry_s));
	if (!ctx->gxQueue.entries)
	{
		linearFree(ctx->cmdBuf);
		return false;
	}

	fh_raster_invalidate();
	ctx->flags = C3DiF_Active | C3DiF_TexEnvBuf | C3DiF_TexEnvAll | C3DiF_Effect | C3DiF_TexStatus | C3DiF_TexAll;

	// TODO: replace with direct struct access
	C3D_DepthMap(true, -1.0f, 0.0f);
	C3D_CullFace(GPU_CULL_BACK_CCW);
	C3D_StencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	C3D_StencilOp(GPU_STENCIL_KEEP, GPU_STENCIL_KEEP, GPU_STENCIL_KEEP);
	C3D_BlendingColor(0);
	C3D_EarlyDepthTest(false, GPU_EARLYDEPTH_GREATER, 0);
	C3D_DepthTest(true, GPU_GREATER, GPU_WRITE_ALL);
	C3D_AlphaTest(false, GPU_ALWAYS, 0x00);
	C3D_AlphaBlend(GPU_BLEND_ADD, GPU_BLEND_ADD, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA, GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA);
	C3D_FragOpMode(GPU_FRAGOPMODE_GL);
	C3D_FragOpShadow(0.0, 1.0);

	ctx->texConfig = BIT(12);
	ctx->texShadow = BIT(0);
	ctx->texEnvBuf = 0;
	ctx->texEnvBufClr = 0xFFFFFFFF;
	ctx->fogClr = 0;
	ctx->fogLut = NULL;

	for (i = 0; i < 3; i ++)
		ctx->tex[i] = NULL;

	for (i = 0; i < 6; i ++)
		C3D_TexEnvInit(&ctx->texEnv[i]);

	ctx->fixedAttribDirty = 0;
	ctx->fixedAttribEverDirty = 0;

	C3Di_RenderQueueInit();
	aptHook(&hookCookie, C3Di_AptEventHook, NULL);

	return true;
}

void C3D_SetViewport(u32 x, u32 y, u32 w, u32 h)
{
	C3D_Context* ctx = C3Di_GetContext();
	ctx->flags |= C3DiF_Viewport | C3DiF_Scissor;
	ctx->viewport[0] = f32tof24(w / 2.0f);
	ctx->viewport[1] = f32tof31(2.0f / w) << 1;
	ctx->viewport[2] = f32tof24(h / 2.0f);
	ctx->viewport[3] = f32tof31(2.0f / h) << 1;
	ctx->viewport[4] = (y << 16) | (x & 0xFFFF);
	ctx->scissor[0] = GPU_SCISSOR_DISABLE;
}

void C3D_SetScissor(GPU_SCISSORMODE mode, u32 left, u32 top, u32 right, u32 bottom)
{
	C3D_Context* ctx = C3Di_GetContext();
	ctx->flags |= C3DiF_Scissor;
	ctx->scissor[0] = mode;
	if (mode == GPU_SCISSOR_DISABLE) return;
	ctx->scissor[1] = (top << 16) | (left & 0xFFFF);
	ctx->scissor[2] = ((bottom-1) << 16) | ((right-1) & 0xFFFF);
}


#include "/work/native-3ds/include/fh_pica_draw_owner.h"
static FhPicaStreamState *fh_stream_owner;
static unsigned fh_stream_drawn;
extern void fh_pica_stream_restore_uniforms(void);

int fh_pica_actor_stream_active(void);
void fh_pica_actor_stream_end(void);
void fh_pica_stream_end(void){
    fh_pica_actor_stream_end();
    if(!fh_stream_owner)return;
    FhPicaStreamState *previous=fh_stream_owner;
    fh_stream_owner=NULL;
    fh_pica_stream_state_reset(previous);
    if(!fh_stream_drawn)return;
    fh_stream_drawn=0;
    /* CPU SDK shadows were not replaced by raw commands. They now require
     * republishing even when their values equal the old retained snapshots.
     * Do not touch DrawUsed: target transitions and splits need that evidence. */
    C3D_Context *ctx=C3Di_GetContext();
    ctx->flags|=C3DiF_AttrInfo|C3DiF_BufInfo|C3DiF_Effect|C3DiF_TexAll|
        C3DiF_TexStatus|C3DiF_TexEnvBuf|C3DiF_TexEnv(0)|C3DiF_TexEnv(1)|
        C3DiF_TexEnv(2)|C3DiF_TexEnv(3);
    /* Attributes, raster, material-buffer state, TEV0..3 and samplers0/1.
     * Viewport/scissor, TEV4/5 and sampler2 registers were not changed. */
    fh_raster.valid&=~((1u<<2)|(1u<<3)|(1u<<4)|(15u<<5)|(3u<<11));
    fh_pica_buffers_invalidate();
    fh_pica_stream_restore_uniforms();
}
int fh_pica_stream_begin(FhPicaStreamState *state){
    C3D_Context *ctx=C3Di_GetContext();
    if(!state || fh_stream_owner || fh_pica_actor_stream_active() || !(ctx->flags&C3DiF_Active) ||
       !ctx->program || !ctx->program->vertexShader || ctx->program->geometryShader)return 0;
    /* These are live material rows. A shader which owns them as float24
     * constants cannot share this lane's ABI. Never overwrite its constants. */
    const shaderInstance_s *shader=ctx->program->vertexShader;
    for(unsigned i=0;i<shader->numFloat24Uniforms;i++){
        unsigned row=shader->float24Uniforms[i].id;
        if(row>=7 && row<=9)return 0;
    }
    C3Di_UpdateContext(); /* Includes the established GS->VSH split/restore. */
    fh_pica_stream_state_reset(state);
    fh_stream_drawn=0;fh_stream_owner=state;return 1;
}
unsigned fh_pica_stream_draw(const FhPicaResidentDraw *draw,const FhPicaDrawLive *live,
                            unsigned refresh){
    C3D_Context *ctx=C3Di_GetContext();
    /* Never publish over a pending SDK program/target/raster change. Camera
     * and other uniform writers must follow the explicit batch contract. */
    const u32 pending=C3DiF_AttrInfo|C3DiF_BufInfo|C3DiF_Effect|C3DiF_FrameBuf|
        C3DiF_Viewport|C3DiF_Scissor|C3DiF_Program|C3DiF_VshCode|C3DiF_GshCode|
        C3DiF_TexAll|C3DiF_TexStatus|C3DiF_TexEnvBuf|C3DiF_TexEnvAll|C3DiF_LightEnv;
    if(!fh_stream_owner || !(ctx->flags&C3DiF_Active) || (ctx->flags&pending) ||
       C3D_BoolUnifsDirty[GPU_VERTEX_SHADER])return 0;
    unsigned cursor=gpuCmdBufOffset;
    unsigned words=fh_pica_stream_resident_append(gpuCmdBuf,gpuCmdBufSize,&cursor,
                                                 fh_stream_owner,draw,live,refresh);
    if(words){gpuCmdBufOffset=cursor;ctx->flags|=C3DiF_DrawUsed;fh_stream_drawn=1;}
    return words;
}

#include "/work/native-3ds/include/fh_pica_world_packet_owner.h"
unsigned fh_pica_stream_packet(const FhPicaWorldPacket *packet){
    C3D_Context *ctx=C3Di_GetContext();
    const u32 pending=C3DiF_AttrInfo|C3DiF_BufInfo|C3DiF_Effect|C3DiF_FrameBuf|
        C3DiF_Viewport|C3DiF_Scissor|C3DiF_Program|C3DiF_VshCode|C3DiF_GshCode|
        C3DiF_TexAll|C3DiF_TexStatus|C3DiF_TexEnvBuf|C3DiF_TexEnvAll|C3DiF_LightEnv;
    if(!fh_stream_owner || !(ctx->flags&C3DiF_Active) || (ctx->flags&pending) ||
       C3D_BoolUnifsDirty[GPU_VERTEX_SHADER])return 0;
    unsigned words=fh_pica_packet_append(packet);
    if(words){
        /* Packet publication changed physical private state. A following
         * individual native draw must republish its complete private state;
         * SDK departure still restores CPU shadows through the usual owner. */
        fh_pica_stream_state_reset(fh_stream_owner);
        ctx->flags|=C3DiF_DrawUsed;fh_stream_drawn=1;
    }
    return words;
}

int fh_pica_stream_environment(FhPicaDrawLive *live){
    if(!fh_stream_owner || !live)return 0;
    const C3D_Context *ctx=C3Di_GetContext();
    live->texture_config=ctx->texConfig;live->texture_shadow=ctx->texShadow;
    live->environment_buffer=ctx->texEnvBuf;live->environment_color=ctx->texEnvBufClr;
    live->fog_color=ctx->fogClr;return 1;
}


#include "/work/native-3ds/include/fh_pica_actor_command_owner.h"
static FhPicaActorCommandState *fh_actor_owner;
static unsigned fh_actor_drawn;
static unsigned fh_actor_preparing;
static u32 fh_actor_deferred_flags;
static const shaderProgram_s *fh_actor_program;
/* These fields are replaced completely by the native publisher. Keep effect
 * publication: depth-map/stencil/fragment fields are shared with other owners. */
#define FH_ACTOR_PRIVATE_FLAGS (C3DiF_AttrInfo|C3DiF_BufInfo|C3DiF_TexAll|C3DiF_TexStatus|C3DiF_TexEnvBuf|C3DiF_TexEnv(0)|C3DiF_TexEnv(1)|C3DiF_TexEnv(2)|C3DiF_TexEnv(3))
extern void fh_pica_stream_restore_actor_uniforms(const uint32_t[3]);
extern void fh_pica_stream_update_actor_shared_uniforms(void);
int fh_pica_actor_stream_active(void){return fh_actor_owner!=NULL;}
void fh_pica_actor_stream_end(void){
    if(!fh_actor_owner)return;
    FhPicaActorCommandState *previous=fh_actor_owner;fh_actor_owner=NULL;
    C3Di_GetContext()->flags|=fh_actor_deferred_flags;fh_actor_deferred_flags=0;
    if(fh_actor_drawn){
        C3D_Context *ctx=C3Di_GetContext();
        ctx->flags|=C3DiF_AttrInfo|C3DiF_BufInfo|C3DiF_Effect|C3DiF_TexAll|
            C3DiF_TexStatus|C3DiF_TexEnvBuf|C3DiF_TexEnv(0)|C3DiF_TexEnv(1)|C3DiF_TexEnv(2)|C3DiF_TexEnv(3);
        fh_raster.valid&=~((1u<<2)|(1u<<3)|(1u<<4)|(15u<<5)|(7u<<11));
        fh_pica_buffers_invalidate();fh_pica_stream_restore_actor_uniforms(previous->uniform_dirty);
    }
    fh_actor_drawn=0;fh_actor_program=NULL;fh_pica_actor_command_reset(previous);
}
int fh_pica_actor_stream_begin(FhPicaActorCommandState *state){
    C3D_Context *ctx=C3Di_GetContext();
    if(!state || fh_actor_owner || fh_stream_owner || !(ctx->flags&C3DiF_Active) ||
       !ctx->program || !ctx->program->vertexShader || ctx->program->geometryShader)return 0;
    const shaderInstance_s *shader=ctx->program->vertexShader;
    for(unsigned i=0;i<shader->numFloat24Uniforms;i++){
        unsigned row=shader->float24Uniforms[i].id;if(row>=7 && row<=76)return 0;
    }
    fh_actor_deferred_flags=0;
    fh_actor_preparing=1;C3Di_UpdateContext();fh_actor_preparing=0;
    fh_actor_deferred_flags|=ctx->flags&FH_ACTOR_PRIVATE_FLAGS;
    ctx->flags&=~FH_ACTOR_PRIVATE_FLAGS;
    fh_pica_actor_command_reset(state);fh_actor_drawn=0;fh_actor_program=ctx->program;fh_actor_owner=state;return 1;
}
/* A compatible actor changes shared camera/light state without retiring the
 * native material/fetch cache. Shader/code/target changes use normal departure;
 * they may publish constants or split and therefore cannot retain this owner. */
int fh_pica_actor_stream_sync(FhPicaActorCommandState *state){
    C3D_Context *ctx=C3Di_GetContext();
    if(!state || (fh_actor_owner && fh_actor_owner!=state))return 0;
    if(!fh_actor_owner)return fh_pica_actor_stream_begin(state);
    const u32 boundary=C3DiF_Program|C3DiF_VshCode|C3DiF_GshCode|C3DiF_FrameBuf;
    if(fh_actor_program!=ctx->program || (ctx->flags&boundary)){
        fh_pica_actor_stream_end();return fh_pica_actor_stream_begin(state);
    }
    /* The shared SDK effect publisher can overwrite raw alpha/depth/blend.
     * Force their next native publication, but retain unchanged geometry,
     * samplers, material and palette. Shared depth-map/stencil still publish. */
    if(ctx->flags&C3DiF_Effect)state->raster[0]=~0u;
    fh_actor_preparing=1;
    C3Di_UpdateContext();
    fh_actor_preparing=0;
    fh_actor_deferred_flags|=ctx->flags&FH_ACTOR_PRIVATE_FLAGS;
    ctx->flags&=~FH_ACTOR_PRIVATE_FLAGS;
    return fh_actor_owner==state;
}
int fh_pica_actor_stream_environment(FhPicaActorParameters *p){
    if(!p || !fh_actor_owner)return 0;
    const C3D_Context *ctx=C3Di_GetContext();
    p->texture_config=ctx->texConfig;p->texture_shadow=ctx->texShadow;
    p->environment_buffer=ctx->texEnvBuf;p->environment_color=ctx->texEnvBufClr;p->fog_color=ctx->fogClr;return 1;
}
unsigned fh_pica_actor_stream_draw(const FhPicaActorGeometry *g,const FhPicaActorParameters *p,const FhPicaActorPose *pose,unsigned refresh){
    C3D_Context *ctx=C3Di_GetContext();
    const u32 pending=C3DiF_AttrInfo|C3DiF_BufInfo|C3DiF_Effect|C3DiF_FrameBuf|C3DiF_Viewport|C3DiF_Scissor|
        C3DiF_Program|C3DiF_VshCode|C3DiF_GshCode|C3DiF_TexAll|C3DiF_TexStatus|C3DiF_TexEnvBuf|C3DiF_TexEnvAll|C3DiF_LightEnv;
    if(!fh_actor_owner || !(ctx->flags&C3DiF_Active) || (ctx->flags&pending) || C3D_BoolUnifsDirty[GPU_VERTEX_SHADER])return 0;
    unsigned cursor=gpuCmdBufOffset;
    unsigned words=fh_pica_actor_command_append(gpuCmdBuf,gpuCmdBufSize,&cursor,fh_actor_owner,g,p,pose,refresh);
    if(words){gpuCmdBufOffset=cursor;ctx->flags|=C3DiF_DrawUsed;fh_actor_drawn=1;}
    return words;
}

void C3Di_UpdateContext(void)
{
    if(!fh_actor_preparing)fh_pica_stream_end();
	int i;
	C3D_Context* ctx = C3Di_GetContext();
    /* Before FrameBuf processing: it clears DrawUsed without finalizing the
     * list. BindProgram may have been called several times without a draw. */
    if ((ctx->flags & C3DiF_Program) && fh_geometry_device_enabled &&
        !ctx->program->geometryShader) {
        /* The geometry unit rejoins vertex processing. Its registers were
         * independently written while GS was active; equal CPU VSH values
         * are not proof that all four physical units contain those values.
         * This also invalidates the private retained-upload cache. */
        C3Di_DirtyUniforms(GPU_VERTEX_SHADER);
        ++fh_geometry_boundary_edges;
        if (ctx->flags & C3DiF_DrawUsed) {
            if (fh_geometry_boundary_enabled) {
                C3D_FrameSplit(GX_CMDLIST_FLUSH);
                ++fh_geometry_boundary_splits;
            }
        } else ++fh_geometry_boundary_separated;
    }

	if (ctx->flags & C3DiF_FrameBuf)
	{
		ctx->flags &= ~C3DiF_FrameBuf;
        fh_raster_invalidate();
		if (ctx->flags & C3DiF_DrawUsed)
		{
			ctx->flags &= ~C3DiF_DrawUsed;
			GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 1);
			GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 1);
		}
		C3Di_FrameBufBind(&ctx->fb);
	}

	if (ctx->flags & C3DiF_Viewport)
	{
		ctx->flags &= ~C3DiF_Viewport;
		if(fh_raster_needed(0,0,fh_raster.viewport,ctx->viewport,sizeof(ctx->viewport))){
        GPUCMD_AddIncrementalWrites(GPUREG_VIEWPORT_WIDTH, ctx->viewport, 4);
		GPUCMD_AddWrite(GPUREG_VIEWPORT_XY, ctx->viewport[4]);
        }
	}

	if (ctx->flags & C3DiF_Scissor)
	{
		ctx->flags &= ~C3DiF_Scissor;
		if(fh_raster_needed(1,1,fh_raster.scissor,ctx->scissor,sizeof(ctx->scissor))){
        GPUCMD_AddIncrementalWrites(GPUREG_SCISSORTEST_MODE, ctx->scissor, 3);
        }
	}

	if (ctx->flags & C3DiF_Program)
	{
        fh_raster.valid &= ~(1u<<2);
		/* Count the actual publication, after queue boundaries, without parsing
         * commands or changing any SDK dirty flags. Words include program
         * configuration and code; upload flags are requests, not GPU timing. */
        u32 fh_program_begin=gpuCmdBufOffset;u32 *fh_program_buffer=gpuCmdBuf;
        ++fh_program_publication_stats[0];
        fh_program_publication_stats[2]+=(ctx->flags&C3DiF_VshCode)!=0;
        fh_program_publication_stats[3]+=(ctx->flags&C3DiF_GshCode)!=0;
        shaderProgramConfigure(ctx->program, (ctx->flags & C3DiF_VshCode) != 0, (ctx->flags & C3DiF_GshCode) != 0);
        if(fh_program_buffer==gpuCmdBuf && gpuCmdBufOffset>=fh_program_begin)
            fh_program_publication_stats[1]+=gpuCmdBufOffset-fh_program_begin;
        else ++fh_program_publication_stats[4];
        fh_geometry_device_enabled = ctx->program->geometryShader != NULL;
		ctx->flags &= ~(C3DiF_Program | C3DiF_VshCode | C3DiF_GshCode);
	}

    if(fh_actor_preparing){
        fh_actor_deferred_flags|=ctx->flags&FH_ACTOR_PRIVATE_FLAGS;
        ctx->flags&=~FH_ACTOR_PRIVATE_FLAGS;
    }
	if (ctx->flags & C3DiF_AttrInfo)
	{
		ctx->flags &= ~C3DiF_AttrInfo;
		if(fh_raster_needed(2,2,&fh_raster.attrInfo,&ctx->attrInfo,sizeof(ctx->attrInfo))){
        C3Di_AttrInfoBind(&ctx->attrInfo);
        }
	}

	if (ctx->flags & C3DiF_BufInfo)
	{
		ctx->flags &= ~C3DiF_BufInfo;
		C3Di_BufInfoBind(&ctx->bufInfo);
	}

	if (ctx->flags & C3DiF_Effect)
	{
		ctx->flags &= ~C3DiF_Effect;
		if(fh_raster_needed(3,3,&fh_raster.effect,&ctx->effect,sizeof(ctx->effect))){
        C3Di_EffectBind(&ctx->effect);
        }
	}

	if (ctx->flags & C3DiF_TexAll)
	{
		u32 units = 0;
		for (i = 0; i < 3; i ++)
		{
			if (ctx->tex[i])
			{
				units |= BIT(i);
				if (ctx->flags & C3DiF_Tex(i))
					if(fh_sampler_needed(i,ctx->tex[i])) C3Di_SetTex(i, ctx->tex[i]);
			}
		}

		// Enable texture units and clear texture cache
		ctx->texConfig &= ~7;
		ctx->texConfig |= units | BIT(16);
		ctx->flags &= ~C3DiF_TexAll;
		ctx->flags |= C3DiF_TexStatus;
	}

	if (ctx->flags & C3DiF_TexStatus)
	{
		ctx->flags &= ~C3DiF_TexStatus;
		GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG, 0xB, ctx->texConfig);
		// Clear texture cache if requested *after* configuring texture units
		if (ctx->texConfig & BIT(16))
		{
			ctx->texConfig &= ~BIT(16);
			GPUCMD_AddMaskedWrite(GPUREG_TEXUNIT_CONFIG, 0x4, BIT(16));
		}
		GPUCMD_AddWrite(GPUREG_TEXUNIT0_SHADOW, ctx->texShadow);
	}

	if (ctx->flags & (C3DiF_ProcTex | C3DiF_ProcTexColorLut | C3DiF_ProcTexLutAll))
		C3Di_ProcTexUpdate(ctx);

	if (ctx->flags & C3DiF_TexEnvBuf)
	{
		ctx->flags &= ~C3DiF_TexEnvBuf;
		u32 values[3]={ctx->texEnvBuf,ctx->texEnvBufClr,ctx->fogClr};
        if(fh_raster_needed(4,4,fh_raster.texEnvBuf,values,sizeof(values))){
        GPUCMD_AddMaskedWrite(GPUREG_TEXENV_UPDATE_BUFFER, 0x7, ctx->texEnvBuf);
		GPUCMD_AddWrite(GPUREG_TEXENV_BUFFER_COLOR, ctx->texEnvBufClr);
		GPUCMD_AddWrite(GPUREG_FOG_COLOR, ctx->fogClr);
        }
	}

	if ((ctx->flags & C3DiF_FogLut) && (ctx->texEnvBuf&7) != GPU_NO_FOG)
	{
		ctx->flags &= ~C3DiF_FogLut;
		if (ctx->fogLut)
		{
			GPUCMD_AddWrite(GPUREG_FOG_LUT_INDEX, 0);
			GPUCMD_AddWrites(GPUREG_FOG_LUT_DATA0, ctx->fogLut->data, 128);
		}
	}

	if ((ctx->texEnvBuf&7) == GPU_GAS)
		C3Di_GasUpdate(ctx);

	if (ctx->flags & C3DiF_TexEnvAll)
	{
		for (i = 0; i < 6; i ++)
		{
			if (!(ctx->flags & C3DiF_TexEnv(i))) continue;
			if(fh_raster_needed(5+i,5,&fh_raster.texEnv[i],&ctx->texEnv[i],sizeof(ctx->texEnv[i]))){
        C3Di_TexEnvBind(i, &ctx->texEnv[i]);
        }
		}
		ctx->flags &= ~C3DiF_TexEnvAll;
	}

	C3D_LightEnv* env = ctx->lightEnv;

	if (ctx->flags & C3DiF_LightEnv)
	{
		u32 enable = env != NULL;
		GPUCMD_AddWrite(GPUREG_LIGHTING_ENABLE0, enable);
		GPUCMD_AddWrite(GPUREG_LIGHTING_ENABLE1, !enable);
		ctx->flags &= ~C3DiF_LightEnv;
	}

	if (env)
		C3Di_LightEnvUpdate(env);

	if (ctx->fixedAttribDirty)
	{
		for (i = 0; i < 12; i ++)
		{
			if (!(ctx->fixedAttribDirty & BIT(i))) continue;
			C3D_FVec* v = &ctx->fixedAttribs[i];

			GPUCMD_AddWrite(GPUREG_FIXEDATTRIB_INDEX, i);
			C3D_ImmSendAttrib(v->x, v->y, v->z, v->w);
		}
		ctx->fixedAttribDirty = 0;
	}

    if(fh_actor_preparing)fh_pica_stream_update_actor_shared_uniforms();
    else C3D_UpdateUniforms(GPU_VERTEX_SHADER);
	/* A GS bind followed by another bind may leave dirty GS shadows
     * without ever drawing. Do not publish them after a VSH-only program:
     * that would overwrite the fourth vertex unit after its VSH upload.
     * Keep pending GS state for the next actual geometry draw. */
    if (ctx->program->geometryShader) C3D_UpdateUniforms(GPU_GEOMETRY_SHADER);
}

bool C3Di_SplitFrame(u32** pBuf, u32* pSize)
{
    fh_pica_stream_end();
	C3D_Context* ctx = C3Di_GetContext();

	if (!gpuCmdBufOffset)
		return false; // Nothing was drawn

	if (ctx->flags & C3DiF_DrawUsed)
	{
		ctx->flags &= ~C3DiF_DrawUsed;
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_FLUSH, 1);
		GPUCMD_AddWrite(GPUREG_FRAMEBUFFER_INVALIDATE, 1);
		GPUCMD_AddWrite(GPUREG_EARLYDEPTH_CLEAR, 1);
	}

	GPUCMD_Split(pBuf, pSize);
	u32 totalCmdBufSize = *pBuf + *pSize - ctx->cmdBuf;
	ctx->cmdBufUsage = (float)totalCmdBufSize / ctx->cmdBufSize;
	return true;
}

float C3D_GetCmdBufUsage(void)
{
	return C3Di_GetContext()->cmdBufUsage;
}

void C3D_Fini(void)
{
    fh_pica_stream_end();
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	aptUnhook(&hookCookie);
	C3Di_RenderQueueExit();
	free(ctx->gxQueue.entries);
	linearFree(ctx->cmdBuf);
	ctx->flags = 0;
}

void C3D_BindProgram(shaderProgram_s* program)
{
	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return;

	shaderProgram_s* oldProg = ctx->program;
	shaderInstance_s* newGsh = program->geometryShader;
	if (oldProg != program)
	{
		ctx->program = program;
		ctx->flags |= C3DiF_Program | C3DiF_AttrInfo;

		if (!oldProg)
			ctx->flags |= C3DiF_VshCode | C3DiF_GshCode;
		else
		{
			shaderInstance_s* oldGsh = oldProg->geometryShader;

			DVLP_s* oldProgV = oldProg->vertexShader->dvle->dvlp;
			DVLP_s* oldProgG = oldGsh ? oldGsh->dvle->dvlp : oldProgV;

			DVLP_s* newProgV = program->vertexShader->dvle->dvlp;
			DVLP_s* newProgG = newGsh ? newGsh->dvle->dvlp : newProgV;

			if (oldProgV != newProgV || (!newGsh && oldProgG != newProgG))
				ctx->flags |= C3DiF_VshCode;
			if (oldProgG != newProgG || (newProgG==oldProgV && newProgG->codeSize >= 512))
				ctx->flags |= C3DiF_GshCode;
		}
	}

	C3Di_LoadShaderUniforms(program->vertexShader);
	if (newGsh)
		C3Di_LoadShaderUniforms(newGsh);
	else
		C3Di_ClearShaderUniforms(GPU_GEOMETRY_SHADER);
}

C3D_FVec* C3D_FixedAttribGetWritePtr(int id)
{
	if (id < 0 || id >= 12)
		return NULL;

	C3D_Context* ctx = C3Di_GetContext();

	if (!(ctx->flags & C3DiF_Active))
		return NULL;

	ctx->fixedAttribDirty     |= BIT(id);
	ctx->fixedAttribEverDirty |= BIT(id);
	return &ctx->fixedAttribs[id];
}
