/* HUD load/retirement owner for one canonical field mesh across four UV flips.
 * Requires the shared shader's geometry-parameter mapping. */
#include <stdlib.h>
#include <string.h>
#include "fh_pica_glow_field.h"
#include "fh_pica_glow_packed.h"
#include "fh_pica_glow_quality.h"

enum {GLOW_MESH_BYTES_LIMIT=64*1024};
static void *glow_mesh_pages[GLOW_MASK_PAGES];
static FhGlowMeshView glow_mesh_views[GLOW_MASK_PAGES];
static unsigned glow_mesh_page_bytes[GLOW_MASK_PAGES];
static unsigned glow_mesh_bytes,glow_mesh_peak,glow_mesh_loads,glow_mesh_declines;
static int glow_mesh_page_load(unsigned slot,const FhTexture *source){
    if(slot>=GLOW_MASK_PAGES||glow_mesh_pages[slot]||!source||
       source->format!=5||source->wrap_s>2||source->wrap_t>2)return 0;
    extern u32 __ctru_linear_heap_size;
    FhGlowField field={source->pixels,
        fh_texture_source_bytes(source->format,source->width,source->height),0,
        source->width,source->height,source->wrap_s,source->wrap_t,1};
    unsigned budget=fh_glow_field_quality_budget(&field);
    if(!budget){glow_mesh_declines++;return 0;}
    FhGlowMeshVertex *scratch=malloc(sizeof(*scratch)*FH_GLOW_MESH_TRAINING);
    FhGlowMesh *staged=scratch?malloc(sizeof(*staged)):NULL;
    if(!scratch||!staged){free(scratch);free(staged);glow_mesh_declines++;return 0;}
    size_t bytes;
    if(!fh_glow_field_shared_compile(staged,scratch,FH_GLOW_MESH_TRAINING,&field,budget)||
       !(bytes=fh_glow_mesh_packed_bytes(staged))){
        free(scratch);free(staged);glow_mesh_declines++;return 0;
    }
    free(scratch);
    unsigned available=linearSpaceFree();
    if(bytes>GLOW_MESH_BYTES_LIMIT-glow_mesh_bytes||bytes>available||
       (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15){
        free(staged);glow_mesh_declines++;return 0;
    }
    void *storage=linearMemAlign(bytes,128);
    if(!storage){free(staged);glow_mesh_declines++;return 0;}
    FhGlowMeshView view;
    if(!fh_glow_mesh_pack(storage,bytes,staged,&view)){
        linearFree(storage);free(staged);glow_mesh_declines++;return 0;
    }
    free(staged);
    fh_gpu_cache_flush(storage,bytes);
    glow_mesh_views[slot]=view;
    glow_mesh_page_bytes[slot]=bytes;glow_mesh_pages[slot]=storage;
    glow_mesh_bytes+=bytes;glow_mesh_loads++;
    if(glow_mesh_bytes>glow_mesh_peak)glow_mesh_peak=glow_mesh_bytes;
    return 1;
}
static const FhGlowMeshView *glow_mesh_page_borrow(unsigned slot,const int16_t uv[4]){
    if(slot>=GLOW_MASK_PAGES||!uv||!glow_mesh_pages[slot])return NULL;
    for(unsigned axis=0;axis<2;axis++)
        if(!((uv[axis]==128&&uv[axis+2]==0)||(uv[axis]==0&&uv[axis+2]==128)))return NULL;
    return glow_mesh_views+slot;
}
static void glow_mesh_page_release(unsigned slot){
    if(slot>=GLOW_MASK_PAGES||!glow_mesh_pages[slot])return;
    /* Existing HUD caller must have retired all GPU references first. */
    linearFree(glow_mesh_pages[slot]);glow_mesh_pages[slot]=NULL;
    memset(glow_mesh_views+slot,0,sizeof(*glow_mesh_views));
    glow_mesh_bytes-=glow_mesh_page_bytes[slot];glow_mesh_page_bytes[slot]=0;
}
