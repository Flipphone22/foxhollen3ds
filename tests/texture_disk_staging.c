#include "fh_texture_pack.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
static unsigned measuring,reads,maximum;
static unsigned long long total;
static unsigned char *gpu_output,*window_output;static unsigned gpu_bytes,window_bytes;
size_t __real_fread(void*,size_t,size_t,FILE*);
size_t __wrap_fread(void *dst,size_t size,size_t count,FILE *file){
    if(measuring){
        uintptr_t at=(uintptr_t)dst,begin=(uintptr_t)gpu_output;
        assert(at+size*count<=begin || at>=begin+gpu_bytes);
        begin=(uintptr_t)window_output;
        assert(at+size*count<=begin || at>=begin+window_bytes);
        reads++;total+=size*count;if(size*count>maximum)maximum=size*count;
    }
    return __real_fread(dst,size,count,file);
}
static unsigned word(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
int main(int argc,char **argv){
    assert(argc==2 || argc==3);FhTexturePack p={0};assert(fh_texture_pack_open(&p,argv[1],128*1024));
    window_bytes=argc==3?(unsigned)strtoul(argv[2],NULL,10):0;
    if(window_bytes){window_output=malloc(window_bytes);assert(window_output && fh_texture_pack_window(&p,window_output,window_bytes));}
    unsigned entries=p.count;
    for(unsigned i=0;i<entries;i++){
        const unsigned char *d=p.directory+i*p.stride;
        unsigned source_bytes=word(d+12);gpu_bytes=word(d+24);
        unsigned char *original=malloc(source_bytes+gpu_bytes);gpu_output=malloc(gpu_bytes);
        assert(original && gpu_output && !p.resident);
        assert(!fseek(p.file,word(d+16),SEEK_SET));assert(fread(original,1,source_bytes+gpu_bytes,p.file)==source_bytes+gpu_bytes);
        FhPackedTexture found,e={.offset=word(d+16)+source_bytes,.format=word(d+20),.bytes=gpu_bytes,.crc=word(d+28),.original_bytes=word(d+4)*word(d+8)*4};
        measuring=1;
        assert(fh_texture_pack_find_surface(&p,original,word(d+4),word(d+8),&found));
        assert(fh_texture_pack_read(&p,&e,gpu_output,gpu_bytes));
        measuring=0;
        assert(!memcmp(gpu_output,original+source_bytes,gpu_bytes));
        free(gpu_output);free(original);
    }
    printf("{\"entries\":%u,\"read_calls\":%u,\"requested_bytes\":%llu,\"maximum_request\":%u,\"exact_images\":true,\"direct_gpu_reads\":0}\n",entries,reads,total,maximum);
    fh_texture_pack_close(&p);free(window_output);return 0;
}
