#include "fh_model_lod.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
static void put(unsigned char*p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(void){
    unsigned char original[512],model[512],patch[104];
    memset(original,0x5a,sizeof(original));original[0xf5]=2;original[0xf6]=0;
    put(original+0xd0,256);put(original+256,320);original[260]=0;original[261]=64;
    put(original+284,384);original[288]=0;original[289]=64;
    memcpy(patch,"FHMLD01",8);put(patch+8,123);put(patch+12,512);
    put(patch+16,crc32(0,original,512));put(patch+20,2);
    put(patch+24,0);put(patch+28,32);memset(patch+32,0,32);
    put(patch+64,1);put(patch+68,32);memset(patch+72,0,32);
    for(unsigned n=0;n<sizeof(patch);n++){
        memcpy(model,original,512);assert(!fh_model_lod_patch(model,512,123,patch,n));assert(!memcmp(model,original,512));
    }
    memcpy(model,original,512);assert(!fh_model_lod_patch(model,512,124,patch,104));
    assert(!memcmp(model,original,512));
    put(patch+64,2);assert(!fh_model_lod_patch(model,512,123,patch,104));assert(!memcmp(model,original,512));put(patch+64,1);
    model[400]^=1;assert(!fh_model_lod_patch(model,512,123,patch,104));memcpy(model,original,512);
    assert(fh_model_lod_patch(model,512,123,patch,104));
    for(unsigned i=0;i<512;i++){
        if(i==261 || i==289)assert(model[i]==32);
        else if(i>=320 && i<448)assert(model[i]==0);
        else assert(model[i]==original[i]);
    }
    /* Overlap the shadow/other list storage: patch must reject atomically. */
    put(original+284,352);memcpy(model,original,512);put(patch+16,crc32(0,original,512));
    assert(!fh_model_lod_patch(model,512,123,patch,104));assert(!memcmp(model,original,512));
    puts("model LOD transactional bounds, identity, overlap and unchanged-data checks passed");
}
