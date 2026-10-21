#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FH_GX_DISPATCH_BUILD 1
#include "../native-3ds/source/model_resources.c"
static unsigned fences,pending;
static const void *reader_address;
static unsigned reader_version;
void fh_gx_dispatch_barrier(void){
    /* A queued reader must observe the old generation until it retires. */
    if(pending && reader_address)assert(fh_model_resource_version(reader_address,1)==reader_version);
    fences++;pending=0;
}
void fh_platform_panic(const char *message){fprintf(stderr,"%s\n",message);exit(86);}
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
int main(int argc,char **argv){
    (void)argv;unsigned char data[1024],copy[1024];memset(data,0x5a,sizeof(data));memcpy(copy,data,sizeof(copy));
    FhModelResourceRange spans[]={{data+32,64},{data+96,32},{data+192,32}};
    fh_model_resources_mode=2;pending=1;
    fh_model_resources_register(data,sizeof(data),spans,3);assert(!pending && fences==1);
    unsigned first=fh_model_resource_version(data+32,96);assert(first);
    assert(fh_model_resource_version(data+192,32)==first);
    assert(!fh_model_resource_version(data+31,1) && !fh_model_resource_version(data+128,1));
    assert(!fh_model_resource_version(data+32,161)); /* Cannot bridge a gap. */
    unsigned observed=0;
    assert(fh_model_resource_match(&observed,data+32,copy+32,64));assert(observed==first);
    assert(fh_model_resource_match(&observed,data+32,copy+32,64));assert(fh_model_resources_checks==1);
    if(argc>1){data[40]^=1;fh_model_resource_match(&observed,data+32,copy+32,64);return 1;}
    fh_model_resources_written(data+128,64);assert(fh_model_resource_version(data+32,64)==first);
    data[40]^=1;fh_model_resources_written(data+40,1);
    assert(fh_model_resource_version(data+32,64)!=first);
    assert(!fh_model_resource_match(&observed,data+32,copy+32,64));assert(!observed);
    memcpy(copy,data,sizeof(copy));assert(fh_model_resource_match(&observed,data+32,copy+32,64));
    pending=1;fh_model_resources_escape(data);assert(!pending && !fh_model_resource_version(data+32,64));
    fh_model_resources_register(data,sizeof(data),spans,3);assert(!fh_model_resource_version(data+32,64));
    /* A pointer may escape and write long after the immediate caller returns. */
    data[41]^=1;assert(!fh_model_resource_match(&observed,data+32,copy+32,64));
    pending=1;fh_model_resources_forget(data);assert(!pending);
    fh_model_resources_register(data,sizeof(data),spans,3);
    assert(fh_model_resource_version(data+32,64)>first);
    FhModelResourceRange bad={data+sizeof(data)-1,2};
    fh_model_resources_register(data,sizeof(data),&bad,1);assert(!fh_model_resource_version(data+32,64));
    fh_model_resources_register(data,sizeof(data),spans,3);
    unsigned overlap_checks=0;
    for(unsigned offset=0;offset<256;offset++)for(unsigned size=0;size<=256;size++){
        unsigned before=fh_model_resource_version(data+32,64);
        fh_model_resources_written(data+offset,size);
        unsigned changed=before!=fh_model_resource_version(data+32,64);
        assert(changed==(unsigned)(overlaps((uintptr_t)data+offset,size,(uintptr_t)data+32,96)||overlaps((uintptr_t)data+offset,size,(uintptr_t)data+192,32)));
        overlap_checks++;
    }
    unsigned before=fh_model_resource_version(data+32,64);fh_model_resources_written(NULL,0);
    assert(fh_model_resource_version(data+32,64)>before);
    /* Two model instances embedded in one object, plus an unrelated owner. */
    unsigned char object[1024],saved[64];memset(object,0x63,sizeof(object));
    FhModelResourceRange a={object+96,64},b={object+608,64};
    fh_model_resources_register(object+64,192,&a,1);
    fh_model_resources_register(object+576,192,&b,1);
    unsigned av=fh_model_resource_version(a.data,a.bytes),bv=fh_model_resource_version(b.data,b.bytes);
    unsigned unrelated=fh_model_resource_version(data+32,64);assert(av && bv);
    memcpy(saved,a.data,sizeof(saved));observed=0;
    assert(fh_model_resource_match(&observed,a.data,saved,sizeof(saved)));
    reader_address=a.data;reader_version=av;pending=1;
    fh_model_resources_write_begin(object+100,1);
    assert(!pending && fh_model_resource_version(a.data,a.bytes)!=av);
    assert(fh_model_resource_version(b.data,b.bytes)==bv);
    object[100]^=1;assert(!fh_model_resource_match(&observed,a.data,saved,sizeof(saved)));
    reader_address=NULL;
    fh_model_resources_escape(object+64);
    /* Moving an instance header cannot restore trust in an escaped alias. */
    fh_model_resources_register(object+80,176,&a,1);
    assert(!fh_model_resource_version(a.data,a.bytes));
    assert(fh_model_resource_version(b.data,b.bytes)==bv);
    reader_address=b.data;reader_version=bv;pending=1;
    fh_model_resources_forget_allocation(object,sizeof(object));
    assert(!pending && !fh_model_resource_version(a.data,a.bytes) && !fh_model_resource_version(b.data,b.bytes));
    reader_address=NULL;
    assert(fh_model_resource_version(data+32,64)==unrelated);
    fh_model_resources_register(object+80,176,&a,1);
    assert(fh_model_resource_version(a.data,a.bytes)>av);
    fh_model_resources_forget_allocation(object,sizeof(object));
    /* Table pressure cannot borrow a version from another allocation. */
    unsigned char *pool=malloc((OWNERS+2)*1024);assert(pool);
    for(unsigned i=0;i<OWNERS+2;i++){
        FhModelResourceRange r={pool+i*1024+32,64};
        fh_model_resources_register(pool+i*1024,1024,&r,1);
    }
    assert(!fh_model_resource_version(pool+(OWNERS+1)*1024+32,64));
    for(unsigned i=0;i<OWNERS+2;i++)fh_model_resources_forget(pool+i*1024);
    free(pool);
    /* A wrapping counter disables the directory; no cache epoch repeats. */
    serial=UINT_MAX;fh_model_resources_written(data+32,1);assert(exhausted && !range_count);
    fh_model_resources_register(data,sizeof(data),spans,3);assert(!fh_model_resource_version(data+32,1));
    printf("PASS: %u overlap cases, gaps, mutation, escape, embedded allocation release/reuse, reader retirement, capacity, wrap; %u fences; %u bytes\n",overlap_checks,fences,fh_model_resources_storage_bytes);
    return 0;
}
