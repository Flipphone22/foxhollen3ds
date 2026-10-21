/* HUD-owned load/retirement helper. Same GPU retirement barrier as the fixed
 * mesh pages; compact views are CPU metadata, not GPU-visible storage. */
#include <stdlib.h>
#include <string.h>
#include "fh_pica_glow_field.h"
#include "fh_pica_glow_packed.h"
#include "fh_pica_glow_quality.h"
enum {GLOW_MESH_BYTES_LIMIT=64*1024};
static void *glow_mesh_pages[GLOW_MASK_PAGES];
static FhGlowMeshView glow_mesh_views[GLOW_MASK_PAGES][4];
static unsigned glow_mesh_page_bytes[GLOW_MASK_PAGES];
static unsigned glow_mesh_bytes,glow_mesh_peak,glow_mesh_loads,glow_mesh_declines;
static int glow_mesh_page_load(unsigned slot,const FhTexture *source){
    if(slot>=GLOW_MASK_PAGES||glow_mesh_pages[slot]||!source||
       source->format!=5||source->wrap_s>2||source->wrap_t>2)return 0;
    extern u32 __ctru_linear_heap_size;
    FhGlowFieldRegion region={{source->pixels,
        fh_texture_source_bytes(source->format,source->width,source->height),0,
        source->width,source->height,source->wrap_s,source->wrap_t,1},{0,0,128,128}};
    unsigned budget=fh_glow_field_quality_budget(&region.field);
    if(!budget){glow_mesh_declines++;return 0;}
    FhGlowMeshVertex *scratch=malloc(sizeof(*scratch)*FH_GLOW_MESH_TRAINING);
    FhGlowMesh *staged=scratch?malloc(4*sizeof(*staged)):NULL;
    if(!scratch||!staged){free(scratch);free(staged);glow_mesh_declines++;return 0;}
    size_t sizes[4],bytes=0;
    for(unsigned flip=0;flip<4;flip++){
        region.uv[0]=flip&1?128:0;region.uv[2]=flip&1?0:128;
        region.uv[1]=flip&2?128:0;region.uv[3]=flip&2?0:128;
        if(!fh_glow_field_region_compile_budget(staged+flip,scratch,FH_GLOW_MESH_TRAINING,&region,budget)||
           !(sizes[flip]=fh_glow_mesh_packed_bytes(staged+flip))){
            free(scratch);free(staged);glow_mesh_declines++;return 0;
        }
        bytes+=sizes[flip];
    }
    free(scratch);
    unsigned available=linearSpaceFree();
    if(bytes>GLOW_MESH_BYTES_LIMIT-glow_mesh_bytes||bytes>available||
       (u64)(available-bytes)*100<(u64)__ctru_linear_heap_size*15){
        free(staged);glow_mesh_declines++;return 0;
    }
    unsigned char *storage=linearMemAlign(bytes,128);
    if(!storage){free(staged);glow_mesh_declines++;return 0;}
    FhGlowMeshView views[4];size_t offset=0;
    for(unsigned flip=0;flip<4;flip++){
        if(!fh_glow_mesh_pack(storage+offset,sizes[flip],staged+flip,views+flip)){
            linearFree(storage);free(staged);glow_mesh_declines++;return 0;
        }
        offset+=sizes[flip];
    }
    free(staged);
    fh_gpu_cache_flush(storage,bytes);
    memcpy(glow_mesh_views[slot],views,sizeof views);
    glow_mesh_page_bytes[slot]=bytes;glow_mesh_pages[slot]=storage;
    glow_mesh_bytes+=bytes;glow_mesh_loads++;
    if(glow_mesh_bytes>glow_mesh_peak)glow_mesh_peak=glow_mesh_bytes;
    return 1;
}
static const FhGlowMeshView *glow_mesh_page_borrow(unsigned slot,const int16_t uv[4]){
    if(slot>=GLOW_MASK_PAGES||!uv||!glow_mesh_pages[slot])return NULL;
    unsigned flip=0;
    for(unsigned axis=0;axis<2;axis++){
        if(uv[axis]==128&&uv[axis+2]==0)flip|=1u<<axis;
        else if(uv[axis]!=0||uv[axis+2]!=128)return NULL;
    }
    return glow_mesh_views[slot]+flip;
}
static void glow_mesh_page_release(unsigned slot){
    if(slot>=GLOW_MASK_PAGES||!glow_mesh_pages[slot])return;
    /* Existing HUD caller must have retired all GPU references first. */
    linearFree(glow_mesh_pages[slot]);glow_mesh_pages[slot]=NULL;
    memset(glow_mesh_views[slot],0,sizeof glow_mesh_views[slot]);
    glow_mesh_bytes-=glow_mesh_page_bytes[slot];glow_mesh_page_bytes[slot]=0;
}
