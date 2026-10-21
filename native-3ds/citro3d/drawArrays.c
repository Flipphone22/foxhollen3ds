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


void C3D_DrawArrays(GPU_Primitive_t primitive, int first, int size)
{
	C3Di_UpdateContext();

	u32 *fh_packet=fh_packet_reserve(22), *fh_next=fh_packet;


	// Set primitive type
	FH_WRITE(GPUREG_PRIMITIVE_CONFIG, 2, primitive);
	// Start a new primitive (breaks off a triangle strip/fan)
	FH_WRITE(GPUREG_RESTART_PRIMITIVE, 15, 1);
	// The index buffer is not used, but this command is still required
	FH_WRITE(GPUREG_INDEXBUFFER_CONFIG, 15, 0x80000000);
	// Number of vertices
	FH_WRITE(GPUREG_NUMVERTICES, 15, size);
	// First vertex
	FH_WRITE(GPUREG_VERTEX_OFFSET, 15, first);
	// Enable array drawing mode
	FH_WRITE(GPUREG_GEOSTAGE_CONFIG2, 1, 1);
	// Enable drawing mode
	FH_WRITE(GPUREG_START_DRAW_FUNC0, 1, 0);
	// Trigger array drawing
	FH_WRITE(GPUREG_DRAWARRAYS, 15, 1);
	// Go back to configuration mode
	FH_WRITE(GPUREG_START_DRAW_FUNC0, 1, 1);
	// Disable array drawing mode
	FH_WRITE(GPUREG_GEOSTAGE_CONFIG2, 1, 0);
	// Clear the post-vertex cache
	FH_WRITE(GPUREG_VTX_FUNC, 15, 1);


	gpuCmdBufOffset += (unsigned)(fh_next-fh_packet);
	C3Di_GetContext()->flags |= C3DiF_DrawUsed;
}
