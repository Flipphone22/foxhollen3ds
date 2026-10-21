#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FH_GX_DISPATCH_BUILD 1
#include "../native-3ds/source/model_resources.c"

void model_alias_write(void *,uint32_t);
void model_bulk_write(void *,const void *,size_t);
void model_bulk_fill(void *,int,size_t);
void model_byte_loop(unsigned char *,size_t);
void model_atomic_write(uint32_t *,uint32_t);
void model_unaligned_write(void *,uint32_t);
static unsigned fences,pending;
static unsigned char *watched;
static unsigned char expected[16];
void fh_gx_dispatch_barrier(void){
    if(pending) {
        assert(!memcmp(watched,expected,sizeof(expected)) && "reader must finish before the store");
        pending=0;
    }
    ++fences;
}
void fh_platform_panic(const char *s){fprintf(stderr,"%s\n",s);abort();}
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
static unsigned arm(unsigned char *data) {
    watched=data;memcpy(expected,data,sizeof(expected));pending=1;
    unsigned v=fh_model_resource_version(data,1);assert(v);return v;
}
static void changed(unsigned before) {
    assert(!pending && "compiler barrier missing");
    assert(fh_model_resource_version(watched,1)!=before);
}
int main(void) {
    unsigned char *data=aligned_alloc(4096,32768),*source=malloc(16384);
    assert(data && source);memset(data,0x5a,32768);memset(source,0x63,16384);
    fh_model_resources_mode=2;
    FhModelResourceRange r={data+4096,8192};
    fh_model_resources_register(data,32768,&r,1);
    unsigned before=arm(data+4100);model_alias_write(data+4100,0x12345678);changed(before);
    before=arm(data+4100);model_atomic_write((uint32_t *)(data+4100),0xfedcba98);changed(before);
    before=arm(data+4100);model_bulk_write(data+4100,source,160);changed(before);
    before=arm(data+4100);model_bulk_fill(data+4100,0x91,8192-4);changed(before);
    before=arm(data+4100);model_byte_loop(data+4100,256);changed(before);
    // First and last pages miss; a long transfer still covers the watched middle.
    before=arm(data+4100);model_bulk_write(data+100,source,16384);changed(before);
    // A packed write crosses into the registered page from an unregistered one.
    before=arm(data+4096);model_unaligned_write(data+4095,0x98765432);changed(before);
    unsigned count=fences;
    model_bulk_fill(data+4096,0,0);model_bulk_write(data+4096,source,0);
    model_alias_write(data+128,123); // Same allocation, undeclared geometry.
    assert(count==fences);
    fh_model_resources_forget(data);count=fences;model_alias_write(data+4100,44);
    assert(count==fences);free(source);free(data);
    puts("PASS: optimized alias/atomic/memcpy/memset/vectorized loop/packed writes, cross-page and long ranges, zero length, gaps, release, reader-before-writer order");
}
