#ifndef FH_GPU_TRANSFER_H
#define FH_GPU_TRANSFER_H
#include <stddef.h>
#include <stdint.h>

enum { FH_GPU_COPY_CONTIGUOUS=8, FH_GPU_COPY_GAPPED=12 };
typedef struct { unsigned source_offset,destination_offset,bytes; } FhGpuCopyRow;
/* Physical runs947aeefb9197 and2f02676c2b22 stall on padded and contiguous
 * raw transfers respectively. Plan rows for GSP DMA into sampled linear
 * storage. Payload and padding ownership stay identical.
 * This planner only admits the zero-input-gap tile-row layouts used here. */
static inline unsigned fh_gpu_copy_rows(unsigned input,unsigned output,unsigned bytes){
    unsigned row=(output&65535u)*16u;
    if(input>>16 || !(output>>16) || !row || !bytes || bytes%row)return 0;
    return bytes/row;
}
static inline FhGpuCopyRow fh_gpu_copy_row(unsigned output,unsigned index){
    unsigned row=(output&65535u)*16u,gap=(output>>16)*16u;
    return (FhGpuCopyRow){index*row,index*(row+gap),row};
}
/* Narrow GPU-owned copy contract. Bit 2 is required when either endpoint has
 * gaps (3dbrew GPU/External_Registers, TextureCopy). Azahar ignores that bit
 * in its texture-copy path, so validate the descriptor before submission.
 * Capacities include the skipped bytes, not just the payload. */
static inline uint64_t fh_gpu_copy_span(unsigned dimensions,unsigned bytes) {
    uint64_t width=(dimensions&65535u)*16u,gap=(dimensions>>16)*16u;
    if(!gap)return bytes;
    if(!width || !bytes)return UINT64_MAX;
    return bytes+((bytes-1)/width)*gap;
}
static inline const char *fh_gpu_texture_copy_layout_error(uintptr_t source,size_t source_bytes,unsigned input,
        uintptr_t destination,size_t destination_bytes,unsigned output,unsigned bytes,unsigned flags) {
    unsigned gapped=(input|output)>>16;
    if(flags!=(gapped?FH_GPU_COPY_GAPPED:FH_GPU_COPY_CONTIGUOUS))
        return "Texture copy flags must include gap mode for strided copies";
    if(bytes<(gapped?192u:16u) || (bytes&15))return "Texture copy size outside supported bounds";
    if((source|destination)&7)return "Texture copy address alignment";
    uint64_t in_span=fh_gpu_copy_span(input,bytes),out_span=fh_gpu_copy_span(output,bytes);
    if(in_span>source_bytes || out_span>destination_bytes)return "Texture copy exceeds allocation or has zero line width";
    if(!source || !destination || in_span>UINTPTR_MAX-source || out_span>UINTPTR_MAX-destination)return "Texture copy address overflow";
    if(source<destination+out_span && destination<source+in_span)return "Texture copy buffers overlap";
    return NULL;
}
static inline const char *fh_gpu_texture_copy_error(uintptr_t source,size_t source_bytes,unsigned input,
        uintptr_t destination,size_t destination_bytes,unsigned output,unsigned bytes,unsigned flags) {
    const char *error=fh_gpu_texture_copy_layout_error(source,source_bytes,input,destination,destination_bytes,output,bytes,flags);
    if(error)return error;
    if(output>>16)return "Regression: padded capture must use registered GPU-sampled linear storage";
    if(source<0x1f000000 || source>=0x1f600000 || destination<0x1f000000 || destination>=0x1f600000 ||
       fh_gpu_copy_span(input,bytes)>0x1f600000-source || fh_gpu_copy_span(output,bytes)>0x1f600000-destination)
        return "Texture copy must remain inside VRAM";
    return NULL;
}
/* Pinned Melee captures tiled rows into a GPU-sampled linear texture. Permit
 * that one registered allocation, never the ARM readback buffer. Hardware
 * tests failed with both VRAM and linear captures; allocation was not proven
 * to cause the hang. This restriction preserves Melee's ownership convention. */
static inline const char *fh_gpu_sampled_copy_error(uintptr_t source,size_t source_bytes,unsigned input,
        uintptr_t destination,size_t destination_bytes,unsigned output,unsigned bytes,unsigned flags,
        uintptr_t gpu_storage,size_t gpu_bytes) {
    const char *error=fh_gpu_texture_copy_layout_error(source,source_bytes,input,destination,destination_bytes,output,bytes,flags);
    if(error)return error;
    if(source<0x1f000000 || source>=0x1f600000 || fh_gpu_copy_span(input,bytes)>0x1f600000-source)
        return "Sampled texture copy source must remain inside VRAM";
    uint64_t span=fh_gpu_copy_span(output,bytes);
    if(!gpu_storage || gpu_bytes>UINTPTR_MAX-gpu_storage || destination<gpu_storage ||
       destination-gpu_storage>gpu_bytes || span>gpu_bytes-(destination-gpu_storage))
        return "Regression: raw GPU readback is outside the supported transfer contract";
    return NULL;
}

/* The port's CPU readback contract is RGBA8 -> RGBA8, tiled -> linear,
 * with no scaling, cropping, or raw-copy mode. This is deliberately narrower
 * than the GPU API. Raw depth readback stalled on the first physical boot;
 * Azahar accepts it, so exclude it from this contract on every platform.
 * Equal-format transfer also preserves the bits of DEPTH24_STENCIL8. */
static inline const char *fh_gpu_readback_error(uintptr_t source,size_t source_bytes,
        uintptr_t destination,size_t destination_bytes,unsigned width,unsigned height,unsigned flags) {
    if(flags)return "Readback requires unscaled RGBA8 tiled-to-linear transfer";
    if(!source || !destination || ((source|destination)&7))return "Readback address must be nonzero and 8-byte aligned";
    if(width<64 || height<16 || width>1024 || height>1024 || (width&7) || (height&7))
        return "Readback dimensions outside supported tiled transfer bounds";
    size_t bytes=(size_t)width*height*4;
    if(bytes>source_bytes || bytes>destination_bytes)return "Readback exceeds buffer allocation";
    if(source>UINTPTR_MAX-bytes || destination>UINTPTR_MAX-bytes)return "Readback address overflow";
    if(source<destination+bytes && destination<source+bytes)return "Readback buffers overlap";
    return NULL;
}
#endif
