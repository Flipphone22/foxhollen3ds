#ifndef FH_PICA_DEPTH_VIEW_H
#define FH_PICA_DEPTH_VIEW_H
#include <stdint.h>
#include <citro3d.h>
/* Borrow the padded native framebuffer's D24S8 bytes as an RGBA8 sampler.
 * R=stencil, G=depth-high, B=depth-middle, A=depth-low. The main render target
 * remains the only allocation owner. Do not TexDelete/Flush/Load this view.
 * Caller must publish prior depth writes, use a separate destination WITHOUT
 * this depth attachment, and retire readers before reusing/freeing its owner.
 * Currently exercised by an isolated emulator fixture, NOT enabled in-game. */
static inline int fh_pica_depth_view(C3D_Tex*out,const C3D_FrameBuf*fb,C3D_Tex*color){
    if(!out || !fb || !color || fb->depthFmt!=GPU_RB_DEPTH24_STENCIL8 || fb->block32 ||
       color->fmt!=GPU_RGBA8 || color->maxLevel || color->minLevel ||
       C3D_TexGetType(color)!=GPU_TEX_2D || fb->colorBuf!=color->data ||
       fb->width!=color->width || fb->height!=color->height ||
       color->width!=256 || color->height!=512 || color->size!=256*512*4)return 0;
    uintptr_t depth=(uintptr_t)fb->depthBuf,base=(uintptr_t)color->data;
    size_t bytes=color->size;
    if((depth&127) || depth<0x1f000000u || depth>=0x1f600000u || bytes>0x1f600000u-depth ||
       base<0x1f000000u || base>=0x1f600000u || bytes>0x1f600000u-base ||
       (depth<base+bytes && base<depth+bytes))return 0;
    C3D_Tex view=*color;view.data=fb->depthBuf;
    /* Reconstruct channels before any filtering of quantized depth. */
    C3D_TexSetFilter(&view,GPU_NEAREST,GPU_NEAREST);
    C3D_TexSetWrap(&view,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    *out=view;return 1;
}
#endif
