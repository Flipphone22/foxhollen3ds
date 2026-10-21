#include "fh_model_lod.h"
#include <string.h>
#include <zlib.h>
static unsigned be16(const unsigned char *p){return (unsigned)p[0]*256+p[1];}
static unsigned be32(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static int within(size_t at,size_t size,size_t total){return at<=total && size<=total-at;}
/* Big-endian wire format: FHMLD01\0, id, original bytes, original CRC32,
 * patch count, then {list index,new size,new GX bytes} records; size aligns32.
 * Validate everything first: a rejected patch must leave the model untouched. */
int fh_model_lod_patch(void *data,size_t bytes,unsigned id,const void *patch,size_t patch_bytes){
    unsigned char *model=data;const unsigned char *p=patch;
    unsigned block=id>>31,header=block?164:252;
    if(!model || !p || bytes<header || bytes>4*1024*1024 || patch_bytes<24 ||
       memcmp(p,"FHMLD01",8) || be32(p+8)!=id || be32(p+12)!=bytes ||
       be32(p+16)!=(unsigned)crc32(0,model,bytes))return 0;
    unsigned count=be32(p+20),table=be32(model+(block?0x68:0xd0)),lists=model[block?0xa1:0xf5],all=lists+(block?0:model[0xf6]);
    if(!count || count>lists || table<header || !within(table,all*28,bytes))return 0;
    size_t at=24;unsigned previous=0;
    for(unsigned i=0;i<count;i++){
        if(!within(at,8,patch_bytes))return 0;
        unsigned index=be32(p+at),size=be32(p+at+4);at+=8;
        if(index>=lists || (i && index<=previous) || !size || size>65535 || (size&31))return 0;
        previous=index;
        unsigned entry=table+index*28,offset=be32(model+entry),old_size=be16(model+entry+4);
        if(size>old_size || offset<header || !within(offset,old_size,bytes) ||
           (offset<table+all*28 && table<offset+old_size) || !within(at,size,patch_bytes))return 0;
        /* Refuse overlapping display-list storage, including unchanged/shadow
         * lists. In-place edits cannot silently change a second render stream. */
        for(unsigned j=0;j<all;j++)if(j!=index){
            unsigned other=be32(model+table+j*28),n=be16(model+table+j*28+4);
            if(n && (uint64_t)offset<(uint64_t)other+n && (uint64_t)other<(uint64_t)offset+old_size)return 0;
        }
        at+=size;
    }
    if(at!=patch_bytes)return 0;
    at=24;
    for(unsigned i=0;i<count;i++){
        unsigned index=be32(p+at),size=be32(p+at+4);at+=8;
        unsigned entry=table+index*28,offset=be32(model+entry),old_size=be16(model+entry+4);
        memcpy(model+offset,p+at,size);memset(model+offset+size,0,old_size-size);
        model[entry+4]=size>>8;model[entry+5]=size;at+=size;
    }
    return 1;
}
