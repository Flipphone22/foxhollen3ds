#include "fh_texture_pack.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
typedef uint32_t u32;
typedef uint64_t u64;
u32 __ctru_heap_size=48*1024*1024,__stacksize__=512*1024;
static FhTexturePack pack;
static int fh_hardware_large_memory,fail_alloc;
static unsigned fake_used,allocations;
struct usage { unsigned uordblks; };
static struct usage test_usage(void){return (struct usage){fake_used};}
static void *test_malloc(size_t n){if(fail_alloc)return NULL;allocations++;return malloc(n);}
static void test_free(void *p){assert(pack.window!=p);assert(allocations);allocations--;free(p);}
static void fh_platform_log(const char *s){fputs(s,stdout);}
#define malloc test_malloc
#define free test_free
#define mallinfo test_usage
#include "platform_pica_texture_window.h"
#undef malloc
#undef free
#undef mallinfo
static unsigned word(const unsigned char *p){return (unsigned)p[0]<<24|(unsigned)p[1]<<16|(unsigned)p[2]<<8|p[3];}
static void compare_all(const char *path,int cached){
    assert(fh_texture_pack_open(&pack,path,128*1024));
    if(cached)texture_window_init();
    uint64_t calls=pack.read_calls,bytes=pack.read_bytes;
    FILE *oracle=fopen(path,"rb");assert(oracle);
    for(unsigned i=0;i<pack.count;i++){
        const unsigned char *d=pack.directory+i*pack.stride;
        unsigned source=word(d+12),gpu=word(d+24);
        unsigned char *original=malloc(source+gpu),*out=malloc(gpu);assert(original&&out);
        assert(!fseek(oracle,word(d+16),SEEK_SET));assert(fread(original,1,source+gpu,oracle)==source+gpu);
        FhPackedTexture found;
        assert(fh_texture_pack_find_surface(&pack,original,word(d+4),word(d+8),&found));
        FhPackedTexture exact={.offset=word(d+16)+source,.format=word(d+20),.bytes=gpu,.crc=word(d+28),.original_bytes=word(d+4)*word(d+8)*4};
        assert(fh_texture_pack_read(&pack,&exact,out,gpu));assert(!memcmp(out,original+source,gpu));
        /* Reuse must preserve exact bytes, including entries larger than the cache. */
        assert(fh_texture_pack_read(&pack,&exact,out,gpu));assert(!memcmp(out,original+source,gpu));
        free(out);free(original);
        out=malloc(found.bytes);original=malloc(found.bytes);assert(out&&original);
        assert(!fseek(oracle,found.offset,SEEK_SET));assert(fread(original,1,found.bytes,oracle)==found.bytes);
        assert(fh_texture_pack_read(&pack,&found,out,found.bytes));assert(!memcmp(out,original,found.bytes));
        free(out);free(original);
    }
    printf("hardware=%s cached=%d entries=%u calls=%llu bytes=%llu\n",fh_hardware_large_memory?"new":"old",cached,pack.count,(unsigned long long)(pack.read_calls-calls),(unsigned long long)(pack.read_bytes-bytes));
    fclose(oracle);texture_window_report();fh_pica_texture_window_release();fh_texture_pack_close(&pack);
    assert(!allocations&&!pack.window);
}
int main(int argc,char **argv){
    assert(argc==2);texture_window_init();assert(!allocations);
    for(int hw=0;hw<2;hw++){
        fh_hardware_large_memory=hw;
        assert(fh_texture_pack_open(&pack,argv[1],128*1024));
        fake_used=__ctru_heap_size;texture_window_init();assert(!allocations);
        fake_used=0;fail_alloc=1;texture_window_init();assert(!allocations);
        fail_alloc=0;texture_window_init();assert(allocations==1);
        assert(texture_window_bytes==(hw?256u:64u)*1024);
        texture_window_init();assert(allocations==1);
        unsigned released=fh_pica_texture_window_release();assert(released==(hw?256u:64u)*1024);
        assert(!fh_pica_texture_window_release()&&!allocations);
        texture_window_init();fh_texture_pack_close(&pack); /* failed reader may close first */
        assert(fh_pica_texture_window_release()==released&&!allocations);
        compare_all(argv[1],0);compare_all(argv[1],1);
    }
    puts("owner lifetime, heap admission, allocation failure and all source/GPU bytes PASS");
}
