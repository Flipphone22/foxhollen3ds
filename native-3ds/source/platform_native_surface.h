/* Native framebuffer and small GPU-owned reflection. Included after the
 * renderer's declarations; also compiled unchanged by the device fixture.
 * Pinned Melee's EFB copy ownership, with the port's audited direct texture
 * view instead of its hardware-problematic raw capture into linear memory. */
#include "fh_gpu_transfer.h"
#include "../vendor/citro3d/internal.h"
static C3D_Tex native_surface_color,native_surface_reflection;
static C3D_RenderTarget *native_surface_display,*native_surface_reflection_target;
typedef struct {float position[4],color[4],uv[2];} NativeSurfaceVertex;
static NativeSurfaceVertex *native_surface_quad;
static unsigned native_surface_copies;
static void native_surface_split(void){
    /* The private SDK reserves transfer space and joins completion callbacks
     * before clearing a full queue. Its shared diagnostic wait retains all
     * resources; a surface-specific timeout must not terminate GPU readers. */
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
}
static C3D_RenderTarget *native_surface_create(void){
    if(!C3D_TexInitVRAM(&native_surface_color,256,512,GPU_RGBA8))
        fh_platform_panic("Native framebuffer texture allocation");
    C3D_TexSetFilter(&native_surface_color,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&native_surface_color,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    C3D_RenderTarget *out=C3D_RenderTargetCreateFromTex(&native_surface_color,
        GPU_TEXFACE_2D,0,GPU_RB_DEPTH24_STENCIL8);
    native_surface_display=C3D_RenderTargetCreate(240,400,GPU_RB_RGBA8,(C3D_DEPTHTYPE){.__i=-1});
    if(!out || !native_surface_display)fh_platform_panic("Native framebuffer target allocation");
    C3D_RenderTargetSetOutput(native_surface_display,GFX_TOP,GFX_LEFT,0);
    return out;
}
static void native_surface_draw_on(C3D_RenderTarget *out){
    C3D_FrameDrawOn(out);C3D_SetViewport(0,0,240,400);
}
static void native_surface_present(void){
    /* Input gaps are supported by the existing console presentation path.
     * No output gaps, linear destination, CPU readback, or depth copies. */
    unsigned input=GX_BUFFER_DIM(240*8*4/16,16*8*4/16);
    u32 *source=(u32*)((u8*)native_surface_color.data+112*256*4);
    const char *error=fh_gpu_texture_copy_error((uintptr_t)source,
        native_surface_color.size-112*256*4,input,(uintptr_t)native_surface_display->frameBuf.colorBuf,
        240*400*4,0,240*400*4,FH_GPU_COPY_GAPPED);
    if(error)fh_platform_panic(error);
    native_surface_split();
    C3D_SyncTextureCopy(source,input,native_surface_display->frameBuf.colorBuf,
        0,240*400*4,FH_GPU_COPY_GAPPED);
    native_surface_display->used=true;
}
static int native_surface_prepare_snapshot(C3D_Tex *image,C3D_RenderTarget **image_target){
    if((*image_target))return 1;
    /* Until publication no commands can reference these allocations. Failure
     * therefore rolls back immediately, without draining unrelated GPU work. */
    if(!native_surface_quad){
    native_surface_quad=linearMemAlign(6*sizeof(*native_surface_quad),128);
    if(!native_surface_quad)return 0;
    const unsigned corner[6]={0,1,2,0,2,3};
    const float xy[4][2]={{0,0},{0,1},{1,1},{1,0}};
    for(unsigned i=0;i<6;i++){
        float x=xy[corner[i]][0],y=xy[corner[i]][1];
        /* Undo LCD rotation. Clamp to texel centres of the active viewport,
         * not to the unused right/bottom padding of the power-of-two view. */
        native_surface_quad[i]=(NativeSurfaceVertex){
            .position={2*x-1,1-2*y,-.5f,1},.color={1,1,1,1},
            .uv={(239.5f-239*y)/256.f,(112.5f+399*x)/512.f}};
    }
    fh_gpu_cache_flush(native_surface_quad,6*sizeof(*native_surface_quad));
    }
    if(!C3D_TexInitVRAM(image,128,128,GPU_RGB565))return 0;
    C3D_RenderTarget *prepared=C3D_RenderTargetCreateFromTex(image,
        GPU_TEXFACE_2D,0,(C3D_DEPTHTYPE){.__i=-1});
    if(!prepared)goto unavailable;
    C3D_TexSetFilter(image,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(image,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    (*image_target)=prepared;return 1;
unavailable:
    C3D_TexDelete(image);
    memset(image,0,sizeof(*image));
    return 0;
}
static C3D_Tex *native_surface_capture_into(C3D_RenderTarget *out,shaderProgram_s *copy_program,C3D_Tex *image,C3D_RenderTarget *image_target){
    /* Ordered framebuffer flush before sampling, then flush the snapshot
     * before returning to the source framebuffer. No feedback attachment. */
    native_surface_split();
    C3D_FrameDrawOn(image_target);
    C3D_BindProgram(copy_program);
    C3D_AttrInfo *a=C3D_GetAttrInfo();AttrInfo_Init(a);
    AttrInfo_AddLoader(a,0,GPU_FLOAT,4);AttrInfo_AddLoader(a,1,GPU_FLOAT,4);AttrInfo_AddLoader(a,2,GPU_FLOAT,2);
    C3D_BufInfo *b=C3D_GetBufInfo();BufInfo_Init(b);
    if(BufInfo_Add(b,native_surface_quad,sizeof(*native_surface_quad),3,0x210)<0)
        fh_platform_panic("Native reflection vertex binding");
    C3D_FVUnifSet(GPU_VERTEX_SHADER,0,1,1,0,0);C3D_FVUnifSet(GPU_VERTEX_SHADER,1,1,1,0,0);
    fh_native_fog_apply(0);
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);C3D_CullFace(GPU_CULL_NONE);
    C3D_DepthMap(true,-1,0);C3D_DepthTest(false,GPU_ALWAYS,GPU_WRITE_COLOR);
    C3D_AlphaTest(false,GPU_ALWAYS,0);C3D_StencilTest(false,GPU_ALWAYS,0,255,255);
    C3D_AlphaBlend(GPU_BLEND_ADD,GPU_BLEND_ADD,GPU_ONE,GPU_ZERO,GPU_ONE,GPU_ZERO);
    C3D_TexEnvBufUpdate(C3D_Both,0);
    for(unsigned i=0;i<6;i++)C3D_TexEnvInit(C3D_GetTexEnv(i));
    C3D_TexEnvSrc(C3D_GetTexEnv(0),C3D_Both,GPU_TEXTURE0,0,0);
    C3D_TexEnvFunc(C3D_GetTexEnv(0),C3D_Both,GPU_REPLACE);
    C3D_TexBind(0,&native_surface_color);C3D_TexBind(1,NULL);C3D_TexBind(2,NULL);
    C3D_DrawArrays(GPU_TRIANGLES,0,6);
    native_surface_split();native_surface_draw_on(out);
    native_surface_copies++;return image;
}
static C3D_Tex *native_surface_capture(C3D_RenderTarget *out,shaderProgram_s *program){
    if(!native_surface_prepare_snapshot(&native_surface_reflection,&native_surface_reflection_target))return NULL;
    return native_surface_capture_into(out,program,&native_surface_reflection,native_surface_reflection_target);
}
static void native_surface_destroy(void){
    /* Caller has retired all queued GPU reads/writes before releasing. */
    if(native_surface_reflection_target)C3D_RenderTargetDelete(native_surface_reflection_target);
    if(native_surface_reflection.data)C3D_TexDelete(&native_surface_reflection);
    linearFree(native_surface_quad);native_surface_quad=NULL;
    C3D_RenderTargetDelete(native_surface_display);C3D_TexDelete(&native_surface_color);
    native_surface_reflection_target=native_surface_display=NULL;
}
