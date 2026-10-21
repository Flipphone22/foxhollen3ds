#include <dolphin/ar.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fh_platform.h"
static unsigned pages,callbacks;
void *fh_platform_alloc(size_t size,size_t alignment) { void *p=NULL;assert(!posix_memalign(&p,alignment,size));if(size==65536) pages++;return p; }
void fh_platform_free(void *p) {free(p);}
void fh_platform_panic(const char *message) { fprintf(stderr,"%s\n",message);abort(); }
static void completed(uintptr_t request) { assert(request);callbacks++; }
int main(void) {
    unsigned char input[100000],output[100000];ARQRequest request;
    assert(!ARCheckInit());assert(ARInit(NULL,0)==0x4000 && ARCheckInit());assert(!pages);
    ARQInit();
    ARQPostRequest(&request,0,1,0,0x23456,(uintptr_t)output,sizeof(output),completed);
    for(unsigned i=0;i<sizeof(output);i++) assert(output[i]==0);
    assert(!pages && callbacks==1);
    for(unsigned i=0;i<sizeof(input);i++) input[i]=(i*117+13)&255;
    ARQPostRequest(&request,0,0,0,(uintptr_t)input,65536-7,sizeof(input),completed);
    assert(pages==3 && callbacks==2);
    ARQPostRequest(&request,0,1,0,65536-7,(uintptr_t)output,sizeof(output),completed);
    assert(!memcmp(input,output,sizeof(input)) && callbacks==3);
    unsigned char *contiguous=ARGetStorageAddress();
    assert(!memcmp(contiguous+65536-7,input,sizeof(input)) && contiguous[0]==0 && contiguous[16*1024*1024-1]==0);
    memset(input,42,sizeof(input));
    ARQPostRequest(&request,0,0,0,(uintptr_t)input,65536-7,sizeof(input),completed);
    assert(!memcmp(contiguous+65536-7,input,sizeof(input)));
    assert(ARAlloc(UINT32_MAX)==0);assert(ARAlloc(16*1024*1024-0x4000)==0x4000);assert(ARAlloc(1)==0);
    puts("PASS: lazy zero pages, cross-page DMA, callbacks, contiguous view, address exhaustion");
}
