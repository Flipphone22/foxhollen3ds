#include "internal.h"

/* Foxhollow adaptation of pinned Melee checked command packets.
 * No command suppression, GPU ownership or cache-publication changes.
 * Write once into the current command buffer. Reserve AFTER context updates:
 * they may append commands or replace the buffer. Publication/retirement still
 * belongs to the existing render queue, not this CPU-only append operation. */
extern void fh_platform_panic(const char*) __attribute__((noreturn));
#define FH_WORD(v) do { *fh_next++ = (u32)(v); } while (0)
#define FH_WRITE(reg,mask,value) do { \
    FH_WORD(value); FH_WORD(GPUCMD_HEADER(0,mask,reg)); \
} while (0)
#define FH_INCREMENT(reg,ptr,count) do { \
    const u32 *fh_values = (ptr); \
    FH_WORD(fh_values[0]); \
    FH_WORD(GPUCMD_HEADER(1u,15,reg) | (((count)-1) << 20)); \
    for(unsigned fh_i=1; fh_i<(count); ++fh_i) FH_WORD(fh_values[fh_i]); \
    if(!((count)&1)) FH_WORD(0); \
} while (0)
static inline u32 *fh_packet_reserve(unsigned words) {
    if(!gpuCmdBuf || gpuCmdBufOffset > gpuCmdBufSize || words > gpuCmdBufSize-gpuCmdBufOffset)
        fh_platform_panic("PICA command packet capacity exceeded");
    return gpuCmdBuf+gpuCmdBufOffset;
}


static inline C3D_Effect* getEffect()
{
	C3D_Context* ctx = C3Di_GetContext();
	ctx->flags |= C3DiF_Effect;
	return &ctx->effect;
}

void C3D_DepthMap(bool bIsZBuffer, float zScale, float zOffset)
{
	C3D_Effect* e = getEffect();
	e->zBuffer = bIsZBuffer;
	e->zScale  = f32tof24(zScale);
	e->zOffset = f32tof24(zOffset);
}

void C3D_CullFace(GPU_CULLMODE mode)
{
	C3D_Effect* e = getEffect();
	e->cullMode = mode;
}

void C3D_StencilTest(bool enable, GPU_TESTFUNC function, int ref, int inputMask, int writeMask)
{
	C3D_Effect* e = getEffect();
	e->stencilMode = (!!enable) | ((function & 7) << 4) | (writeMask << 8) | (ref << 16) | (inputMask << 24);
}

void C3D_StencilOp(GPU_STENCILOP sfail, GPU_STENCILOP dfail, GPU_STENCILOP pass)
{
	C3D_Effect* e = getEffect();
	e->stencilOp = sfail | (dfail << 4) | (pass << 8);
}

void C3D_BlendingColor(u32 color)
{
	C3D_Effect* e = getEffect();
	e->blendClr = color;
}

void C3D_EarlyDepthTest(bool enable, GPU_EARLYDEPTHFUNC function, u32 ref)
{
	C3D_Effect* e = getEffect();
	e->earlyDepth = enable;
	e->earlyDepthFunc = function;
	e->earlyDepthRef = ref;
}

void C3D_DepthTest(bool enable, GPU_TESTFUNC function, GPU_WRITEMASK writemask)
{
	C3D_Effect* e = getEffect();
	e->depthTest = (!!enable) | ((function & 7) << 4) | (writemask << 8);
}

void C3D_AlphaTest(bool enable, GPU_TESTFUNC function, int ref)
{
	C3D_Effect* e = getEffect();
	e->alphaTest = (!!enable) | ((function & 7) << 4) | (ref << 8);
}

void C3D_AlphaBlend(GPU_BLENDEQUATION colorEq, GPU_BLENDEQUATION alphaEq, GPU_BLENDFACTOR srcClr, GPU_BLENDFACTOR dstClr, GPU_BLENDFACTOR srcAlpha, GPU_BLENDFACTOR dstAlpha)
{
	C3D_Effect* e = getEffect();
	e->alphaBlend = colorEq | (alphaEq << 8) | (srcClr << 16) | (dstClr << 20) | (srcAlpha << 24) | (dstAlpha << 28);
	e->fragOpMode &= ~0xFF00;
	e->fragOpMode |= 0x0100;
}

void C3D_ColorLogicOp(GPU_LOGICOP op)
{
	C3D_Effect* e = getEffect();
	e->fragOpMode &= ~0xFF00;
	e->clrLogicOp = op;
}

void C3D_FragOpMode(GPU_FRAGOPMODE mode)
{
	C3D_Effect* e = getEffect();
	e->fragOpMode &= ~0xFF00FF;
	e->fragOpMode |= 0xE40000 | mode;
}

void C3D_FragOpShadow(float scale, float bias)
{
	C3D_Effect* e = getEffect();
	e->fragOpShadow = f32tof16(scale+bias) | (f32tof16(-scale)<<16);
}

void C3Di_EffectBind(C3D_Effect* e)
{
	u32 *fh_packet=fh_packet_reserve(34), *fh_next=fh_packet;

	FH_WRITE(GPUREG_DEPTHMAP_ENABLE, 15, e->zBuffer ? 1 : 0);
	FH_WRITE(GPUREG_FACECULLING_CONFIG, 15, e->cullMode & 0x3);
	FH_INCREMENT(GPUREG_DEPTHMAP_SCALE, (u32*)&e->zScale, 2);
	FH_INCREMENT(GPUREG_FRAGOP_ALPHA_TEST, (u32*)&e->alphaTest, 4);
	FH_WRITE(GPUREG_GAS_DELTAZ_DEPTH, 0x8, (u32)GPU_MAKEGASDEPTHFUNC((e->depthTest>>4)&7) << 24);
	FH_WRITE(GPUREG_BLEND_COLOR, 15, e->blendClr);
	FH_WRITE(GPUREG_BLEND_FUNC, 15, e->alphaBlend);
	FH_WRITE(GPUREG_LOGIC_OP, 15, e->clrLogicOp);
	FH_WRITE(GPUREG_COLOR_OPERATION, 7, e->fragOpMode);
	FH_WRITE(GPUREG_FRAGOP_SHADOW, 15, e->fragOpShadow);
	FH_WRITE(GPUREG_EARLYDEPTH_TEST1, 1, e->earlyDepth ? 1 : 0);
	FH_WRITE(GPUREG_EARLYDEPTH_TEST2, 15, e->earlyDepth ? 1 : 0);
	FH_WRITE(GPUREG_EARLYDEPTH_FUNC, 1, e->earlyDepthFunc);
	FH_WRITE(GPUREG_EARLYDEPTH_DATA, 0x7, e->earlyDepthRef);

	gpuCmdBufOffset += (unsigned)(fh_next-fh_packet);
}
