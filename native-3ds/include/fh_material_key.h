#ifndef FH_MATERIAL_KEY_H
#define FH_MATERIAL_KEY_H
#include "fh_material_register.h"
#include "fh_memory_bytes.h"
#include "fh_hash_bytes.h"
/* A synchronous view of the exact existing serialized material key. Do not
 * copy a live view: its canonical/register spans point into this object.
 * Cache entries still own all bytes; no game-pointer identity is trusted. */
typedef struct {
    struct {const void *data;unsigned bytes;} spans[23];
    unsigned count,bytes,raster[6];
    FhRegisterMaterial registers;
} FhMaterialKey;
_Static_assert(sizeof(FhRegisterMaterial)%4==0 && sizeof(FhTevStage)%4==0,"Material hash spans must end on word boundaries");
static void fh_material_key_span(FhMaterialKey *key,const void *data,unsigned bytes){
    key->spans[key->count++]=(typeof(key->spans[0])){data,bytes};key->bytes+=bytes;
}
static int fh_material_key_init(FhMaterialKey *key,const FhDrawState *state){
    key->count=key->bytes=0;
    if(fh_material_register_key(&key->registers,state)){
        fh_material_key_span(key,&key->registers,sizeof(key->registers));return 1;
    }
    fh_material_key_span(key,state->tev,state->stages*sizeof(FhTevStage));
    fh_material_key_span(key,state->tev_colors,128);
    fh_material_key_span(key,state->swaps,72);
    fh_material_key_span(key,&state->constant_raster_mask,36);
    for(unsigned i=0;i<8;i++){
        fh_material_key_span(key,&state->textures[i].width,12);
        fh_material_key_span(key,&state->textures[i].wrap_s,16);
    }
    key->raster[0]=state->blend_mode;key->raster[1]=state->alpha_write;
    key->raster[2]=state->color_write;key->raster[3]=state->depth_test;
    key->raster[4]=state->depth_write;key->raster[5]=state->depth_compare;
    fh_material_key_span(key,key->raster,sizeof(key->raster));
    fh_material_key_span(key,state->alpha_compare,20);
    fh_material_key_span(key,&state->indirect_count,36);return 0;
}
static int fh_material_key_equal(const FhMaterialKey *key,const void *stored,unsigned bytes){
    if(bytes!=key->bytes)return 0;
    const unsigned char *p=stored;
    for(unsigned i=0;i<key->count;i++){
        if(fh_memory_compare(p,key->spans[i].data,key->spans[i].bytes))return 0;
        p+=key->spans[i].bytes;
    }
    return 1;
}
static unsigned fh_material_key_hash(const FhMaterialKey *key){
    unsigned hash=2166136261u;
    for(unsigned i=0;i<key->count;i++)hash=fh_hash_bytes(hash,key->spans[i].data,key->spans[i].bytes);
    return hash;
}
static void fh_material_key_copy(void *out,const FhMaterialKey *key){
    unsigned char *p=out;
    for(unsigned i=0;i<key->count;i++){
        fh_copy_bytes(p,key->spans[i].data,key->spans[i].bytes);p+=key->spans[i].bytes;
    }
}
#endif
