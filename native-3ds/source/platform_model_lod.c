#include "fh_asset_paths.h"
#include "fh_model_lod.h"
#include "fh_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>
#include <zlib.h>
unsigned fh_model_lod_applied,fh_model_lod_rejected;
static unsigned initialized,count;
static unsigned char *directory;
static FILE *pack;
static long pack_bytes;
void fh_model_lod_shutdown(void){
    if(pack)fclose(pack);
    pack=NULL;free(directory);directory=NULL;count=0;initialized=0;pack_bytes=0;
}
static unsigned be32(const unsigned char*p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int reserve(unsigned bytes){
    extern uint32_t __ctru_heap_size,__stacksize__;
    return ((uint64_t)(unsigned)mallinfo().uordblks+__stacksize__+bytes+64)*100<= (uint64_t)__ctru_heap_size*85;
}
void fh_model_lod_prepare(void *model,unsigned bytes,unsigned id){
    if(!initialized){
        initialized=1;unsigned char header[16];
        pack=fopen(fh_asset_path(FH_ASSET_MODEL),"rb");if(!pack)return;
        if(fseek(pack,0,SEEK_END) || (pack_bytes=ftell(pack))<16 || pack_bytes>32*1024*1024 ||
           fseek(pack,0,SEEK_SET) || fread(header,1,16,pack)!=16 || memcmp(header,"FHMLPK1",8) ||
           (count=be32(header+8))>4096 || 16+count*24>(unsigned)pack_bytes || !reserve(count*24))goto bad;
        if(!count){
            if(pack_bytes!=16 || be32(header+12))goto bad;
            fh_platform_log("Original model topology enabled; no decimation patches\n");return;
        }
        directory=malloc(count*24);if(!directory || fread(directory,24,count,pack)!=count ||
           (unsigned)crc32(0,directory,count*24)!=be32(header+12))goto bad;
        for(unsigned i=0;i<count;i++){
            const unsigned char *e=directory+i*24;unsigned offset=be32(e+12),size=be32(e+16);
            if(offset<16+count*24 || size<24 || size>1024*1024 || offset>(unsigned)pack_bytes || size>(unsigned)pack_bytes-offset)goto bad;
        }
        fh_platform_log("Prepared model topology pack enabled; render data only\n");
    }
    if(!pack)return;
    unsigned crc=0,hashed=0;
    for(unsigned i=0;i<count;i++){
        const unsigned char *e=directory+i*24;
        if(be32(e)!=id || be32(e+4)!=bytes)continue;
        if(!hashed){crc=crc32(0,model,bytes);hashed=1;}
        if(be32(e+8)!=crc)continue;
        unsigned offset=be32(e+12),size=be32(e+16);
        if(!reserve(size)){fh_model_lod_rejected++;return;}
        unsigned char *patch=malloc(size);if(!patch){fh_model_lod_rejected++;return;}
        int ok=!fseek(pack,offset,SEEK_SET) && fread(patch,1,size,pack)==size &&
            (unsigned)crc32(0,patch,size)==be32(e+20) && fh_model_lod_patch(model,bytes,id,patch,size);
        free(patch);if(ok)fh_model_lod_applied++;else fh_model_lod_rejected++;
        return;
    }
    return;
bad:
    free(directory);directory=NULL;count=0;fclose(pack);pack=NULL;
    fh_model_lod_rejected++;fh_platform_log("Prepared model pack rejected; original assets retained\n");
}
