#include "fh_fragment.h"
#include "fh_texture_dependencies.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
enum {TEXTURES=4,NATIVE_GEOMETRY_SLOTS=3};
typedef struct {FhTexture source,secondary;unsigned bytes,valid,mode;} Texture;
typedef struct {unsigned fur;FhTexture fur_textures[2];} NativeGeometryKey;
typedef struct {unsigned id;void *storage;NativeGeometryKey *key;} NativeGeometry;
static Texture textures[TEXTURES];
static NativeGeometry native_geometry[NATIVE_GEOMETRY_SLOTS];
static unsigned notifications;
static void note_fur_invalidation(const void *address,size_t bytes){(void)address;(void)bytes;notifications++;}
#define fh_fur_cache_invalidate_range note_fur_invalidation
#include "texture-cache-invalidate.inc"
#undef fh_fur_cache_invalidate_range
int main(void){
    unsigned char base_pixels[64],mask_pixels[64],palette[512],other[64];
    memset(base_pixels,200,sizeof(base_pixels));memset(mask_pixels,0,sizeof(mask_pixels));
    memset(palette,255,sizeof(palette));memset(other,0,sizeof(other));
    FhTexture base={.pixels=base_pixels,.width=8,.height=8,.format=1};
    FhTexture mask={.pixels=mask_pixels,.palette=palette,.width=8,.height=8,.format=9,.palette_format=0,.palette_count=256};
    uint32_t before[64],after[64];
    assert(fh_texture_pack_fur(before,8,8,&base,&mask));
    for(unsigned mode=0;mode<4;mode++)textures[mode]=(Texture){base,mask,256,1,mode};
    NativeGeometryKey key={1,{base,mask}};
    native_geometry[0]=(NativeGeometry){17,other,&key};
    palette[0]=0; /* CI8 mask alpha changes without any texel/index change. */
    assert(fh_texture_pack_fur(after,8,8,&base,&mask));
    assert(memcmp(before,after,sizeof(before)));
    texture_cache_invalidate(palette,1);
    assert(!textures[3].valid && "secondary palette did not invalidate packed texture");
    assert(!textures[1].valid && textures[0].valid && textures[2].valid);
    assert(!native_geometry[0].id && native_geometry[0].storage==other);
    for(unsigned mode=0;mode<4;mode++)textures[mode].valid=1;
    texture_cache_invalidate(other,sizeof(other));
    for(unsigned mode=0;mode<4;mode++)assert(textures[mode].valid);
    texture_cache_invalidate(palette+sizeof(palette),1);
    assert(textures[3].valid);
    texture_cache_invalidate(palette+sizeof(palette)-1,1);
    assert(!textures[3].valid);
    textures[3].valid=1;texture_cache_invalidate(mask_pixels,0);assert(textures[3].valid);
    texture_cache_invalidate(base_pixels+63,1);
    for(unsigned mode=0;mode<4;mode++)assert(!textures[mode].valid);
    for(unsigned mode=0;mode<4;mode++)textures[mode].valid=1;
    texture_cache_invalidate(NULL,0);
    for(unsigned mode=0;mode<4;mode++)assert(!textures[mode].valid);
    FhTexture high={.pixels=(void*)(UINTPTR_MAX-15),.width=8,.height=8,.format=1};
    assert(fh_texture_source_overlap(&high,(void*)(UINTPTR_MAX-1),1));
    assert(fh_texture_source_overlap(&high,(void*)(UINTPTR_MAX-20),10));
    assert(!fh_texture_source_overlap(&high,(void*)1,4));
    printf("PASS: production invalidation revokes packed secondary palette; output changes; texel/palette boundaries, modes, global invalidation, wrapped ranges and GPU storage lifetime; %u notifications\n",notifications);
}
