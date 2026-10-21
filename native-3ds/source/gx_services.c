#include "gx_internal.h"
#include "fh_minimal_graphics.h"
#include "fh_gx_dispatch.h"
#include "fh_replay_timing.h"
#include <dolphin/vi.h>
#include <string.h>
#include <math.h>
GXRenderModeObj GXNtsc480IntDf={VI_TVMODE_NTSC_INT,640,480,480,40,0,640,480,VI_XFBMODE_DF,0,0,{{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6}},{0,0,21,22,21,0,0}};
GXRenderModeObj GXNtsc480Prog={VI_TVMODE_NTSC_PROG,640,480,480,40,0,640,480,VI_XFBMODE_SF,0,0,{{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6}},{0,0,21,22,21,0,0}};
typedef struct { void *base,*read,*write;u32 bytes,low,high; } Fifo;
static GXFifoObj initial_fifo,*cpu_fifo,*gpu_fifo;
static GXBreakPtCallback breakpoint;
static GXDrawSyncCallback sync_callback;
static int breakpoint_enabled;
static u16 tokens[64],completed_token;
static unsigned token_head,token_tail;
GXFifoObj *GXInit(void *base,u32 bytes) {
    fh_gx_reset();GXInitFifoBase(&initial_fifo,base,bytes);cpu_fifo=gpu_fifo=&initial_fifo;
    fh_renderer_init();return &initial_fifo;
}
void GXInitFifoBase(GXFifoObj *fifo,void *base,u32 bytes) { fh_gx_require(fifo && base && bytes,"Invalid GX FIFO");memset(fifo,0,sizeof(*fifo));*(Fifo*)fifo=(Fifo){base,base,base,bytes,0,bytes}; }
void GXInitFifoLimits(GXFifoObj *fifo,u32 high,u32 low) { fh_gx_require(fifo && low<=high && high<=((Fifo*)fifo)->bytes,"Invalid GX FIFO limits");((Fifo*)fifo)->low=low;((Fifo*)fifo)->high=high; }
void GXSetCPUFifo(GXFifoObj *fifo) { cpu_fifo=fifo; }
void GXSetGPFifo(GXFifoObj *fifo) { gpu_fifo=fifo; }
void GXGetFifoPtrs(GXFifoObj *fifo,void **read,void **write) { fh_gx_require(fifo!=NULL,"Missing GX FIFO");if(read) *read=((Fifo*)fifo)->read;if(write) *write=((Fifo*)fifo)->write; }
void GXGetGPStatus(GXBool *over,GXBool *under,GXBool *read,GXBool *command,GXBool *brk) { if(over) *over=GX_FALSE;if(under) *under=GX_TRUE;if(read) *read=GX_TRUE;if(command) *command=GX_TRUE;if(brk) *brk=breakpoint_enabled; }
GXBreakPtCallback GXSetBreakPtCallback(GXBreakPtCallback callback) { GXBreakPtCallback old=breakpoint;breakpoint=callback;return old; }
GXDrawSyncCallback GXSetDrawSyncCallback(GXDrawSyncCallback callback) { GXDrawSyncCallback old=sync_callback;sync_callback=callback;return old; }
void GXEnableBreakPt(void *address) { (void)address;breakpoint_enabled=1; }
void GXDisableBreakPt(void) { breakpoint_enabled=0; }
void GXSetDrawSync(u16 token) {
    fh_gx_require(token_head-token_tail<64,"GX completion queue overflow");tokens[token_head++%64]=token;
}
u16 GXReadDrawSync(void) { return completed_token; }
void fh_gx_frame_complete(void) {
    fh_gx_dispatch_join_deferred();
    if(token_head==token_tail) return;
    fh_renderer_sync();
    if(breakpoint_enabled && breakpoint) breakpoint();
    completed_token=tokens[token_tail++%64];if(sync_callback) sync_callback(completed_token);
}
void GXFlush(void) { fh_gx_end_primitive(); }
void GXPixModeSync(void) { fh_gx_end_primitive();fh_renderer_sync(); }
void GXResetWriteGatherPipe(void) {}
void __GXAbortWaitPECopyDone(void) { fh_renderer_sync(); }
void GXSetMisc(GXMiscToken token,u32 value) { (void)token;(void)value; }
void GXSetGPMetric(GXPerf0 first,GXPerf1 second) { (void)first;(void)second; }
void GXReadXfRasMetric(u32 *a,u32 *b,u32 *c,u32 *d) { if(a) *a=0;if(b) *b=0;if(c) *c=0;if(d) *d=0; }
void GXSetCopyClear(GXColor color,u32 depth) { fh_gx.clear_color=(u32)color.r<<24|(u32)color.g<<16|(u32)color.b<<8|color.a;fh_gx.clear_depth=depth; }
void GXSetDispCopySrc(u16 x,u16 y,u16 width,u16 height) { u32 values[]={x,y,width,height};memcpy(fh_gx.display_source,values,sizeof(values)); }
void GXSetDispCopyDst(u16 width,u16 height) { fh_gx.display_width=width;fh_gx.display_height=height; }
u32 GXSetDispCopyYScale(f32 scale) { return (u32)(fh_gx.display_source[3]*scale+.5f); }
void GXSetDispCopyGamma(GXGamma gamma) { (void)gamma; }
void GXSetCopyFilter(GXBool aa,u8 samples[12][2],GXBool vertical,u8 filter[7]) { (void)aa;(void)samples;(void)vertical;(void)filter; }
void GXCopyDisp(void *destination,GXBool clear) {
    /* The actual submission precedes VIWaitForRetrace's duplicate present.
     * Nested GPU waits/input/audio keep their exclusive owners; the scope
     * restores the caller when that retrace service returns. */
    FH_REPLAY_SCOPE(PRESENT);
    (void)destination;fh_gx_end_primitive();
    while(token_head!=token_tail)fh_gx_frame_complete();
    fh_renderer_present();if(clear)fh_renderer_clear(fh_gx.clear_color,fh_gx.clear_depth);
    VIWaitForRetrace();
}
void GXSetTexCopySrc(u16 x,u16 y,u16 width,u16 height) { u32 values[]={x,y,width,height};memcpy(fh_gx.copy_source,values,sizeof(values)); }
void GXSetTexCopyDst(u16 width,u16 height,GXTexFmt format,GXBool mipmap) { fh_gx.copy_width=width;fh_gx.copy_height=height;fh_gx.copy_format=format;fh_gx.copy_mipmap=mipmap; }
void GXCopyTex(void *destination,GXBool clear) {
    fh_gx_end_primitive();
    fh_renderer_copy(destination,fh_gx.copy_source,fh_gx.copy_width,fh_gx.copy_height,fh_gx.copy_format,fh_gx.copy_mipmap,clear,fh_gx.clear_color,fh_gx.clear_depth,&fh_gx.draw);
}
extern int fh_renderer_copy_gpu_color(void*,const uint32_t[4],unsigned,unsigned,unsigned,int,int) __attribute__((weak));
void FHCopyReflectionTex(void *destination,GXBool clear) {
    fh_gx_end_primitive();
    if(fh_minimal_enabled)fh_minimal_reflection_update(destination,fh_gx.copy_source,fh_gx.copy_width,fh_gx.copy_height,fh_gx.copy_format,fh_gx.copy_mipmap,clear);
    if(!fh_render_floor && fh_renderer_copy_gpu_color && fh_renderer_copy_gpu_color(destination,fh_gx.copy_source,fh_gx.copy_width,fh_gx.copy_height,fh_gx.copy_format,fh_gx.copy_mipmap,clear))return;
    GXCopyTex(destination,clear);
}
extern int fh_renderer_copy_gpu_depth(void*,const uint32_t[4],unsigned,unsigned,unsigned,int,int) __attribute__((weak));
void FHCopyReflectionDepth(void *destination,GXBool clear) {
    fh_gx_end_primitive();
    if(!fh_render_floor && fh_renderer_copy_gpu_depth && fh_renderer_copy_gpu_depth(destination,fh_gx.copy_source,fh_gx.copy_width,fh_gx.copy_height,fh_gx.copy_format,fh_gx.copy_mipmap,clear))return;
    GXCopyTex(destination,clear);
}
extern int fh_renderer_copy_gpu_shadow(void*,const uint32_t[4],unsigned,unsigned,unsigned,int,int,uint32_t,uint32_t,const FhDrawState*) __attribute__((weak));
void FHCopyShadowR4(void *destination,GXBool clear) {
    fh_gx_end_primitive();
    if(!fh_render_floor && fh_renderer_copy_gpu_shadow && fh_renderer_copy_gpu_shadow(destination,fh_gx.copy_source,fh_gx.copy_width,fh_gx.copy_height,fh_gx.copy_format,fh_gx.copy_mipmap,clear,fh_gx.clear_color,fh_gx.clear_depth,&fh_gx.draw))return;
    GXCopyTex(destination,clear);
}
void GXPeekZ(u16 x,u16 y,u32 *depth) { if(depth) *depth=fh_renderer_depth(x,y); }
void GXDrawTorus(f32 radius,u8 sides,u8 rings) {
    fh_gx_require(radius<1 && radius>0 && sides>=3 && rings>=3,"Invalid torus geometry");
    u32 descriptors[26];FhVtxFormat formats[26];
    memcpy(descriptors,fh_gx.descriptors,sizeof(descriptors));memcpy(formats,fh_gx.formats[3],sizeof(formats));
    GXClearVtxDesc();GXSetVtxDesc(GX_VA_POS,GX_DIRECT);GXSetVtxDesc(GX_VA_NRM,GX_DIRECT);
    GXSetVtxAttrFmt(GX_VTXFMT3,GX_VA_POS,GX_POS_XYZ,GX_F32,0);GXSetVtxAttrFmt(GX_VTXFMT3,GX_VA_NRM,GX_NRM_XYZ,GX_F32,0);
    if(descriptors[GX_VA_TEX0]) { GXSetVtxDesc(GX_VA_TEX0,GX_DIRECT);GXSetVtxAttrFmt(GX_VTXFMT3,GX_VA_TEX0,GX_TEX_ST,GX_F32,0); }
    for(unsigned i=0;i<sides;i++) {
        GXBegin(GX_TRIANGLESTRIP,GX_VTXFMT3,(rings+1)*2);
        for(unsigned j=0;j<=rings;j++) for(int k=1;k>=0;k--) {
            float s=((i+k)%sides)*6.28318530718f/sides,t=(j%rings)*6.28318530718f/rings;
            float cs=cosf(s),ss=sinf(s),ct=cosf(t),st=sinf(t),r=1-radius-radius*cs;
            GXPosition3f32(r*ct,r*st,radius*ss);GXNormal3f32(-ct*cs,-st*cs,ss);
            if(descriptors[GX_VA_TEX0]) GXTexCoord2f32((i+k)/(float)sides,j/(float)rings);
        }
        GXEnd();
    }
    memcpy(fh_gx.descriptors,descriptors,sizeof(descriptors));memcpy(fh_gx.formats[3],formats,sizeof(formats));
}
