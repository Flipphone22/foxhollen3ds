/* Included by HUD page owner. Called at texture load/retirement boundaries,
 * never by the draw loop. Existing page retirement must drain GPU readers
 * before release. Pinned Melee explicit publication/lifetime rules apply. */
#include <stdlib.h>
#include "fh_pica_glow_field.h"
enum { GLOW_MESH_BYTES_LIMIT=64*1024 };
static FhGlowMesh *glow_mesh_pages[GLOW_MASK_PAGES];
static unsigned glow_mesh_bytes,glow_mesh_peak,glow_mesh_loads,glow_mesh_declines;

static int glow_mesh_page_load(unsigned slot,const FhTexture *source)
{
    if(slot>=GLOW_MASK_PAGES||glow_mesh_pages[slot]||!source||
       source->format!=5||source->wrap_s>2||source->wrap_t>2)return 0;
    const unsigned bytes=4*sizeof(FhGlowMesh);
    extern u32 __ctru_linear_heap_size;
    unsigned free_bytes=linearSpaceFree();
    if(bytes>GLOW_MESH_BYTES_LIMIT-glow_mesh_bytes||bytes>free_bytes||
       (u64)(free_bytes-bytes)*100<(u64)__ctru_linear_heap_size*15){
        glow_mesh_declines++;return 0;
    }
    FhGlowFieldRegion region={{source->pixels,
        fh_texture_source_bytes(source->format,source->width,source->height),0,
        source->width,source->height,source->wrap_s,source->wrap_t,1},{0,0,128,128}};
    if(!fh_glow_field_bytes(&region.field)){glow_mesh_declines++;return 0;}
    FhGlowMeshVertex *scratch=malloc(sizeof(*scratch)*FH_GLOW_MESH_TRAINING);
    FhGlowMesh *meshes=scratch?linearMemAlign(bytes,128):NULL;
    if(!scratch||!meshes){free(scratch);glow_mesh_declines++;return 0;}
    for(unsigned flip=0;flip<4;flip++){
        region.uv[0]=(flip&1)?128:0;region.uv[2]=(flip&1)?0:128;
        region.uv[1]=(flip&2)?128:0;region.uv[3]=(flip&2)?0:128;
        if(!fh_glow_field_region_compile(meshes+flip,scratch,FH_GLOW_MESH_TRAINING,&region)){
            free(scratch);linearFree(meshes);glow_mesh_declines++;return 0;
        }
    }
    free(scratch);
    /* Publish only after all four variants have compiled and become coherent. */
    fh_gpu_cache_flush(meshes,bytes);
    glow_mesh_pages[slot]=meshes;glow_mesh_bytes+=bytes;glow_mesh_loads++;
    if(glow_mesh_bytes>glow_mesh_peak)glow_mesh_peak=glow_mesh_bytes;
    return 1;
}
static const FhGlowMesh *glow_mesh_page_borrow(unsigned slot,const int16_t uv[4])
{
    if(slot>=GLOW_MASK_PAGES||!uv||!glow_mesh_pages[slot])return NULL;
    unsigned flip=0;
    for(unsigned axis=0;axis<2;axis++){
        if(uv[axis]==128&&uv[axis+2]==0)flip|=1u<<axis;
        else if(uv[axis]!=0||uv[axis+2]!=128)return NULL;
    }
    return glow_mesh_pages[slot]+flip;
}
static void glow_mesh_page_release(unsigned slot)
{
    if(slot>=GLOW_MASK_PAGES||!glow_mesh_pages[slot])return;
    /* Caller owns the existing page GPU retirement barrier, not this helper. */
    linearFree(glow_mesh_pages[slot]);glow_mesh_pages[slot]=NULL;
    glow_mesh_bytes-=4*sizeof(FhGlowMesh);
}
