/* Bounded GPU image owner for native projected shadows. Not yet wired into the
 * gameplay renderer. Queue ordering follows the pinned Melee/citro3d ownership
 * rules and the existing native auxiliary owner, with no transfer-engine copy. */
#include <3ds.h>
#include <citro3d.h>
#include <stdio.h>
#include "fh_pica_shadow_image.h"
#include "fh_pica_scene.h"
#include "fh_platform.h"
extern void fh_minimal_auxiliary_begin(void);
extern void fh_minimal_auxiliary_end(unsigned,unsigned);
static C3D_Tex images[FH_SHADOW_IMAGE_SLOTS];
static C3D_FrameBuf targets[FH_SHADOW_IMAGE_SLOTS];
static unsigned initialized,active,valid,slot,updates,draw_count,index_count;
#include "platform_shadow_mask_capture.h"
void fh_pica_shadow_image_init(void){
    if(initialized)fh_platform_panic("PICA shadow image duplicate init");
    if(vramSpaceFree()<FH_SHADOW_IMAGE_BYTES+1024*1024)
        fh_platform_panic("PICA shadow image VRAM headroom");
    for(unsigned i=0;i<FH_SHADOW_IMAGE_SLOTS;i++){
        if(!C3D_TexInitVRAM(images+i,FH_SHADOW_IMAGE_SIZE,FH_SHADOW_IMAGE_SIZE,GPU_RGB565))
            fh_platform_panic("PICA shadow image allocation");
        C3D_TexSetFilter(images+i,GPU_LINEAR,GPU_LINEAR);
        C3D_TexSetWrap(images+i,GPU_CLAMP_TO_BORDER,GPU_CLAMP_TO_BORDER);
        images[i].border=0; /* SDK descriptor; published by C3D_TexBind. */
        C3D_FrameBufTex(targets+i,images+i,GPU_TEXFACE_2D,0);
        C3D_FrameBufDepth(targets+i,NULL,GPU_RB_DEPTH24);
    }
    initialized=1;active=valid=0;slot=FH_SHADOW_IMAGE_SLOTS-1;
    updates=draw_count=index_count=0;memset(shadow_capture_meta,0,sizeof(shadow_capture_meta));
}
void fh_pica_shadow_image_rebind(void){
    if(!initialized || !active)fh_platform_panic("PICA shadow image capture lifetime");
    C3D_SetFrameBuf(targets+slot);
    C3D_SetViewport(0,0,FH_SHADOW_IMAGE_SIZE,FH_SHADOW_IMAGE_SIZE);
    C3D_SetScissor(GPU_SCISSOR_DISABLE,0,0,0,0);
}
void fh_pica_shadow_image_begin(void){
    if(!initialized || active)fh_platform_panic("PICA shadow image begin order");
    fh_minimal_auxiliary_begin();
    /* Submit earlier sampling before this slot becomes an attachment again.
     * A ring alone is not a fence: arbitrarily many casters can share a frame.
     * This is ordered GPU work, not a CPU wait or a resource-ownership return. */
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    for(unsigned i=0;i<3;i++)C3D_TexBind(i,NULL);
    slot=(slot+1)%FH_SHADOW_IMAGE_SLOTS;valid=0;active=1;
    fh_pica_shadow_image_rebind();fh_pica_scene_invalidate();
}
void fh_pica_shadow_image_end(unsigned draws,unsigned indices){
    if(!active || !draws || indices<3)fh_platform_panic("PICA shadow image empty capture");
    /* Publish framebuffer writes before restoring the main target and exposing
     * the sampler. The parent restores/invalidate all retained scene state. */
    C3D_FrameSplit(GX_CMDLIST_FLUSH);
    fh_minimal_auxiliary_end(draws,indices);
    active=0;valid=1;updates++;draw_count+=draws;index_count+=indices;
    uint32_t *m=shadow_capture_meta[slot];memset(m,0,8*sizeof(*m));m[0]=updates;m[4]=draws;m[5]=indices;
}
const void *fh_pica_shadow_image_sampler(void){
    return initialized && valid && !active?images+slot:NULL;
}
void fh_pica_shadow_image_shutdown(void){
    if(!initialized)return;
    if(active)fh_platform_panic("PICA shadow image shutdown during capture");
    /* Caller has joined the main queue, including every retained sampler. */
    for(unsigned i=0;i<FH_SHADOW_IMAGE_SLOTS;i++)C3D_TexDelete(images+i);
    char line[192];snprintf(line,sizeof(line),"PICA shadow images: updates=%u draws=%u indices=%u vram_bytes=%u CPU_pixels=0\n",
        updates,draw_count,index_count,FH_SHADOW_IMAGE_BYTES);fh_platform_log(line);
    initialized=valid=0;
}
