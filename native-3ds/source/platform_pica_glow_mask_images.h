/* Included by the existing native image owner, after acquire_image. These
 * references share release_image's lifetime: the caller drains readers BEFORE
 * its final release, just as for ordinary HUD/particle images. No draw-side
 * acquisition, source pointers, separate allocator or eviction protocol. */
#include "fh_pica_glow_mask.h"
enum { GLOW_MASK_IMAGES=16, GLOW_MASK_BYTES=16*1024 };
static unsigned glow_mask_handles[GLOW_MASK_IMAGES],glow_mask_bytes;
static unsigned glow_mask_peak,glow_mask_loads,glow_mask_reuses,glow_mask_declines;

unsigned fh_pica_image_acquire_glow_mask(const FhTexture *source)
{
    if(!source || !source->pixels || source->format!=5 || source->width<8 || source->height<8 ||
       source->width>1024 || source->height>1024 || (source->width&(source->width-1)) ||
       (source->height&(source->height-1))){glow_mask_declines++;return 0;}
    unsigned bytes=source->width*source->height;
    if(bytes>GLOW_MASK_BYTES){glow_mask_declines++;return 0;}
    void *staging=allocate(bytes);
    if(!staging){glow_mask_declines++;return 0;}
    if(!fh_pica_glow_mask_rgb5a3(staging,bytes,source->pixels,
        fh_texture_source_bytes(5,source->width,source->height),source->width,source->height)){
        deallocate(staging,bytes);glow_mask_declines++;return 0;
    }
    unsigned free_mask=GLOW_MASK_IMAGES;
    for(unsigned i=0;i<GLOW_MASK_IMAGES;i++){
        unsigned handle=glow_mask_handles[i];
        if(!handle){if(free_mask==GLOW_MASK_IMAGES)free_mask=i;continue;}
        Image *im=images+handle-1;
        if(!im->refs)fh_platform_panic("PICA glow mask stale image");
        if(im->gpu.width==source->width && im->gpu.height==source->height &&
           im->bytes==bytes && !memcmp(im->gpu.data,staging,bytes)){
            im->refs++;deallocate(staging,bytes);glow_mask_reuses++;return handle;
        }
    }
    unsigned slot=0;while(slot<IMAGES && images[slot].refs)slot++;
    unsigned free_bytes=linearSpaceFree();extern u32 __ctru_linear_heap_size;
    if(slot==IMAGES || free_mask==GLOW_MASK_IMAGES || bytes>GLOW_MASK_BYTES-glow_mask_bytes ||
       gpu_bytes>4u*1024u*1024u || bytes>4u*1024u*1024u-gpu_bytes || bytes>free_bytes ||
       (u64)(free_bytes-bytes)*100<(u64)__ctru_linear_heap_size*15){
        deallocate(staging,bytes);glow_mask_declines++;return 0;
    }
    Image *im=images+slot;
    if(!C3D_TexInit(&im->gpu,source->width,source->height,GPU_A8)){
        memset(im,0,sizeof(*im));deallocate(staging,bytes);glow_mask_declines++;return 0;
    }
    memcpy(im->gpu.data,staging,bytes);deallocate(staging,bytes);
    fh_gpu_cache_flush(im->gpu.data,bytes);
    im->refs=1;im->offset=0;im->bytes=bytes;
    im->source_width=source->width;im->source_height=source->height;
    glow_mask_handles[free_mask]=slot+1;glow_mask_bytes+=bytes;
    if(glow_mask_bytes>glow_mask_peak)glow_mask_peak=glow_mask_bytes;
    gpu_bytes+=bytes;if(gpu_bytes>gpu_peak)gpu_peak=gpu_bytes;
    loads++;glow_mask_loads++;return slot+1;
}

static void glow_mask_image_retire(unsigned handle)
{
    for(unsigned i=0;i<GLOW_MASK_IMAGES;i++)if(glow_mask_handles[i]==handle){
        Image *im=images+handle-1;
        if(im->refs || im->bytes>glow_mask_bytes)fh_platform_panic("PICA glow mask retirement");
        glow_mask_bytes-=im->bytes;glow_mask_handles[i]=0;return;
    }
}
