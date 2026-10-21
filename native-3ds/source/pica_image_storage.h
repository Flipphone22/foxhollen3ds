/* Native immutable image placement. Consulted pinned Melee texture_vram.h
 * c2ebe7c769b3: its queued promotion/eviction path is disabled on hardware.
 * Here placement is chosen ONCE, before exposing any sampler. The original
 * linear image remains owned until all queued copies/draws retire; no hot
 * promotion, demotion, reverse copy, pointer rebinding or CPU VRAM reads.
 * L4/A4 remain linear regardless of sampler unit (known hardware hazard).
 */
enum { PICA_IMAGE_VRAM_LIMIT=3*1024*1024, PICA_IMAGE_VRAM_RESERVE=1024*1024 };
static unsigned image_vram_bytes,image_vram_peak,image_vram_copies;
static unsigned image_vram_pressure,image_vram_unsafe;
static const void *image_cpu_data(const Image *im){
    return im->linear_shadow?im->linear_shadow:im->gpu.data;
}
static void image_place(Image *im){
    if(!fh_minimal_texture_vram_enabled())return;
    if(im->gpu.fmt==GPU_L4 || im->gpu.fmt==GPU_A4){image_vram_unsafe++;return;}
    if(!im->bytes || (im->bytes&15) || im->bytes>PICA_IMAGE_VRAM_LIMIT-image_vram_bytes ||
       vramSpaceFree()<PICA_IMAGE_VRAM_RESERVE+im->bytes){image_vram_pressure++;return;}
    void *destination=vramAlloc(im->bytes);
    if(!destination){image_vram_pressure++;return;}
    /* Upload is ordered before the first sampler can be published. Its source
     * is already cache-clean and remains immutable for the entire lifetime. */
    fh_minimal_texture_upload(im->gpu.data,destination,im->bytes);
    im->linear_shadow=im->gpu.data;im->gpu.data=destination;
    image_vram_bytes+=im->bytes;image_vram_copies++;
    if(image_vram_bytes>image_vram_peak)image_vram_peak=image_vram_bytes;
}
static void image_storage_retire(Image *im){
    /* The caller already joined copies AND draws, as for linear-only images. */
    if(im->linear_shadow){
        if(image_vram_bytes<im->bytes)fh_platform_panic("PICA image VRAM accounting");
        image_vram_bytes-=im->bytes;linearFree(im->linear_shadow);im->linear_shadow=NULL;
    }
    C3D_TexDelete(&im->gpu);
}
