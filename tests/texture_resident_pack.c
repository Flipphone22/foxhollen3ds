#include "fh_texture_pack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned word(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
int main(int argc,char **argv){
    assert(argc==2);FhTexturePack pack={0};assert(fh_texture_pack_open(&pack,argv[1],128*1024));
    unsigned bytes=fh_texture_pack_resident_size(&pack);unsigned char *cache=malloc(bytes);assert(cache);
    assert(fh_texture_pack_resident_load(&pack,cache,bytes));
    uint64_t disk=pack.read_bytes;
    for(unsigned i=0;i<pack.count;i++){
        const unsigned char *d=pack.directory+i*pack.stride;
        unsigned source_bytes=word(d+12),gpu_bytes=word(d+24);
        unsigned char *original=malloc(source_bytes+gpu_bytes),*out=malloc(gpu_bytes);assert(original && out);
        assert(!fseek(pack.file,word(d+16),SEEK_SET));assert(fread(original,1,source_bytes+gpu_bytes,pack.file)==source_bytes+gpu_bytes);
        assert(!memcmp(cache+pack.resident_offsets[i*2],original,source_bytes));
        FhPackedTexture e={.offset=word(d+16)+source_bytes,.format=word(d+20),.bytes=gpu_bytes,.crc=word(d+28),.original_bytes=word(d+4)*word(d+8)*4};
        assert(fh_texture_pack_read(&pack,&e,out,gpu_bytes));assert(!memcmp(out,original+source_bytes,gpu_bytes));
        FhPackedTexture found;assert(fh_texture_pack_find_surface(&pack,original,word(d+4),word(d+8),&found));
        assert(pack.read_bytes==disk);free(out);free(original);
    }
    printf("resident pack: %u entries exact source/GPU bytes; %u allocated, %u used, zero runtime SD reads\n",pack.count,bytes,pack.resident_bytes);
    fh_texture_pack_close(&pack);free(cache);return 0;
}
