/* Included after the renderer's allocation helpers. All page storage counts
 * against its ordinary texture/linear limits, with identical GPU publication. */
#include "fh_scene_storage.h"
#include "fh_native_scene.h"
static Texture scene_pages[4];
void *fh_renderer_scene_page_allocate(unsigned page){
    if(!initialized || page>=4 || scene_pages[page].bytes)return NULL;
    fh_gx_dispatch_barrier();
    const unsigned bytes=512*512*2;
    if(texture_bytes+bytes>TEXTURE_CACHE_LIMIT || linearSpaceFree()<bytes+LINEAR_RESERVE)return NULL;
    Texture *t=scene_pages+page;
    if(!C3D_TexInit(&t->gpu,512,512,GPU_RGB565))return NULL;
    t->bytes=bytes;texture_bytes+=bytes;
    t->source=(FhTexture){.pixels=t->gpu.data,.width=512,.height=512,.format=4,
        .min_filter=1,.mag_filter=1};
    C3D_TexSetFilter(&t->gpu,GPU_LINEAR,GPU_LINEAR);
    C3D_TexSetWrap(&t->gpu,GPU_CLAMP_TO_EDGE,GPU_CLAMP_TO_EDGE);
    return t->gpu.data;
}
void fh_renderer_scene_page_commit(unsigned page){
    if(page>=4 || !scene_pages[page].bytes || scene_pages[page].valid)return;
    C3D_TexFlush(&scene_pages[page].gpu);scene_pages[page].valid=1;
}
void fh_renderer_scene_page_cancel(unsigned page){
    if(page<4 && !scene_pages[page].valid)retire(scene_pages+page);
}
const FhTexture *fh_renderer_scene_page(unsigned page){
    return page<4 && scene_pages[page].valid?&scene_pages[page].source:NULL;
}
static Texture *scene_page_lookup(const FhTexture *source,unsigned mode){
    if(mode)return NULL;
    for(unsigned i=0;i<4;i++)if(scene_pages[i].valid &&
        source->pixels==scene_pages[i].source.pixels && source->width==512 && source->height==512 && source->format==4)return scene_pages+i;
    return NULL;
}
void fh_renderer_scene_storage_join(void){
    if(fh_minimal_enabled){fh_minimal_sync();return;}
    if(!initialized)return;
    fh_gx_dispatch_barrier();
    /* Begin performs the bounded wait for a preceding closed frame. Sync also
     * submits/drains any current frame which may still reference the storage.
     * A vblank-only C3D_FrameSync is NOT a geometry-release fence. */
    begin();fh_renderer_sync();
}
unsigned fh_renderer_scene_upload_begin(unsigned last_used){
    if(fh_minimal_enabled)return fh_minimal_prepare_write(last_used);
    fh_gx_dispatch_barrier();begin();
    if(last_used==frame_number+1)fh_renderer_sync();
    return frame_number+1;
}
unsigned fh_renderer_scene_frame(void){return fh_minimal_enabled?fh_minimal_frames+1:frame_number+1;}
static void scene_storage_shutdown(void){
    fh_native_scene_shutdown();
    for(unsigned i=0;i<4;i++)retire(scene_pages+i);
}
