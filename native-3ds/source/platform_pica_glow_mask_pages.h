/* Sparse additions to existing HUD/particle pages. The page's source owner and
 * retirement barrier remain authoritative; no game texture pointer is retained.
 * Called only at spawn/load boundaries. Draws borrow immutable descriptors. */
typedef struct {unsigned image;C3D_Tex sampler;} GlowMaskPage;
enum { GLOW_MASK_PAGES=16 };
static GlowMaskPage glow_mask_pages[GLOW_MASK_PAGES];
static uint8_t glow_mask_page_index[PAGES];
#include "platform_pica_glow_shared_pages.h"

int fh_pica_hud_glow_mask_ready(const void *owner)
{
    unsigned page=fh_pica_hud_find(owner);
    return page && glow_mask_page_index[page-1];
}

int fh_pica_hud_glow_mask_load(const void *owner,const FhTexture *source)
{
    if(!(fh_graphics_effects&FH_EFFECT_PARTICLE_REFLECTION) ||
       (fh_effects_unavailable_mask&FH_EFFECT_PARTICLE_REFLECTION) ||
       !(fh_graphics_options&FH_GRAPHICS_PARTICLES) ||
       (fh_graphics_options&(FH_GRAPHICS_NO_RENDER|FH_GRAPHICS_NO_PARTICLE_TEX)))return 0;
    unsigned page=fh_pica_hud_find(owner);
    if(!page)return 0;
    if(glow_mask_page_index[page-1])return 1;
    unsigned slot=0;while(slot<GLOW_MASK_PAGES&&glow_mask_pages[slot].image)slot++;
    if(slot==GLOW_MASK_PAGES)return 0;
    unsigned image=fh_pica_image_acquire_glow_mask(source);
    if(!image)return 0;
    GlowMaskPage *p=glow_mask_pages+slot;p->image=image;
    float scale[2];fh_pica_image_sampler(image,source,&p->sampler,scale);
    /* Compiler admits exact power-of-two extents, never scaled pack entries. */
    if(scale[0]!=1 || scale[1]!=1)fh_platform_panic("PICA glow mask extent");
    glow_mesh_page_load(slot,source);
    glow_mask_page_index[page-1]=slot+1;
    return 1;
}

const C3D_Tex *fh_pica_hud_glow_mask_sampler(unsigned page)
{
    if(!page || page>PAGES || !pages[page-1].owner)return NULL;
    unsigned slot=glow_mask_page_index[page-1];
    return slot?&glow_mask_pages[slot-1].sampler:NULL;
}

static void glow_mask_page_release(unsigned page_index)
{
    unsigned slot=glow_mask_page_index[page_index];
    if(!slot)return;
    GlowMaskPage *p=glow_mask_pages+slot-1;
    glow_mesh_page_release(slot-1);
    fh_pica_image_release(p->image);
    memset(p,0,sizeof(*p));glow_mask_page_index[page_index]=0;
}

const FhGlowMeshView *fh_pica_hud_glow_mesh(unsigned page,const int16_t uv[4])
{
    if(!page||page>PAGES||!pages[page-1].owner)return NULL;
    unsigned slot=glow_mask_page_index[page-1];
    return slot?glow_mesh_page_borrow(slot-1,uv):NULL;
}
