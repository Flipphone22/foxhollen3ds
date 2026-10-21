#define _GNU_SOURCE
#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
typedef void (*GXBreakPtCallback)(void);
#define FH_GX_DISPATCH_BUILD 1
#ifndef FH_GXD_SOURCE
#define FH_GXD_SOURCE "../native-3ds/source/gx_dispatch.c"
#endif
#include FH_GXD_SOURCE
#include "lifetime-stubs.h"
FhGx fh_gx;
static pthread_t reader;
static unsigned stopped,permit,waiting,draws,worker_draws;
static unsigned cache_events;
static unsigned wakes;
static unsigned owned_floor;
static _Thread_local unsigned is_reader;
static unsigned char *source;
static int invalid_array;
void fh_platform_panic(const char *message){fprintf(stderr,"panic: %s\n",message);abort();}
void fh_renderer_dispatch_begin(void){assert(!is_reader);}
#ifdef FH_REAL_MODEL_SCOPE
void fh_gx_require(int condition,const char *message){if(!condition)fh_platform_panic(message);}
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
void *fh_memory_copy(void *a,const void *b,size_t n){return memcpy(a,b,n);}
void *fh_renderer_retained_alloc(unsigned bytes){return malloc(bytes);}
#else
void fh_retained_scope_invalidate(const void *address,size_t bytes){assert(is_reader && address==source+1024 && bytes==64);}
#endif
void fh_renderer_invalidate(const void *address,size_t bytes){assert(is_reader && address==source+1024 && bytes==64);++cache_events;}
void fh_renderer_texture_hint(void) {assert(is_reader);}
#ifndef FH_REAL_MODEL_SCOPE
void fh_retained_reader_scope_begin(const void *const addresses[4],const unsigned bytes[4]){
    assert(is_reader && addresses[0]==source && bytes[0]==256);
    for(unsigned i=1;i<4;i++)assert(!addresses[i] && !bytes[i]);
}
void fh_retained_reader_scope_end(void){assert(is_reader);}
int fh_retained_scope_contains(const void *p,unsigned bytes){
    uintptr_t a=(uintptr_t)p,b=(uintptr_t)source;
    return a>=b && a-b<=256 && bytes<=256-(a-b);
}
#endif
void fh_gxd_GXSetArray(GXAttr attr,const void *data,u32 bytes,u8 stride,bool little_endian){
    assert(attr==GX_VA_POS);fh_gx.arrays[attr]=(FhVtxArray){data,bytes,stride,little_endian};
}
void fh_gxd_GXSetVtxDesc(GXAttr attr,GXAttrType type){assert(attr==GX_VA_POS);fh_gx.descriptors[attr]=type;}
void fh_gxd_GXLoadPosMtxImm(const void *matrix,u32 id){assert(is_reader && id==0);memcpy(fh_gx.position[0],matrix,48);}
void fh_gxd_GXCallDisplayList(const void *data,u32 bytes){
    if(is_reader)assert(fh_gxd_pending() && __atomic_load_n(&queue.read,__ATOMIC_RELAXED)==owned_floor);
    assert(!invalid_array);assert(bytes==64 && data==source+512);
    for(unsigned i=0;i<64;i++)assert(((const unsigned char*)data)[i]==(unsigned char)(i^0x5a));
    for(unsigned i=0;i<256;i++)assert(source[i]==(unsigned char)i);
    if(is_reader)for(unsigned i=0;i<48;i++)assert(((unsigned char*)fh_gx.position[0])[i]==(unsigned char)(draws^(i?0:0x80)));
#ifdef FH_TEST_MATRIX_BLOCK
    if(is_reader)for(unsigned slot=0;slot<10;slot++)for(unsigned i=0;i<48;i++){
        assert(((unsigned char*)fh_gx.normal[slot])[i]==(unsigned char)draws);
        assert(((unsigned char*)fh_gx.texture[10+slot])[i]==(unsigned char)draws);
    }
#endif
    ++draws;if(is_reader)++worker_draws;
}
static void *consume_thread(void *unused){
    (void)unused;is_reader=1;
    while(!__atomic_load_n(&stopped,__ATOMIC_ACQUIRE)){
        if(__atomic_load_n(&permit,__ATOMIC_ACQUIRE))fh_gxd_consume_batch(64);else sched_yield();
    }
    return NULL;
}
int fh_gxd_platform_start(void){assert(!pthread_create(&reader,NULL,consume_thread,NULL));return 1;}
void fh_gxd_platform_wake(void){++wakes;}
void fh_gxd_platform_wait(void){
    __atomic_store_n(&waiting,1,__ATOMIC_RELEASE);
    while(fh_gxd_pending())sched_yield();
}
static void *release_reader(void *unused){
    (void)unused;
    while(!__atomic_load_n(&waiting,__ATOMIC_ACQUIRE))sched_yield();
    __atomic_store_n(&permit,1,__ATOMIC_RELEASE);return NULL;
}
int main(int argc,char **argv){
    invalid_array=argc>1 && !strcmp(argv[1],"invalid-array");
    source=mmap(NULL,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);
    assert(source!=MAP_FAILED && (uintptr_t)source<=UINT32_MAX-4096);
    fh_gx_dispatch_enabled=2;
    fh_gx_dispatch_batch_enabled=1;
    fh_gx_matrix_filter_enabled=1;
#ifdef FH_TEST_MATRIX_BLOCK
    fh_gx_matrix_block_enabled=1;
#endif
    for(unsigned pass=0;pass<200;pass++){
        for(unsigned i=0;i<256;i++)source[i]=(unsigned char)i;
        for(unsigned i=0;i<64;i++)source[512+i]=(unsigned char)(i^0x5a);
        __atomic_store_n(&waiting,0,__ATOMIC_RELEASE);__atomic_store_n(&permit,0,__ATOMIC_RELEASE);
#ifdef FH_REAL_MODEL_SCOPE
        const void *ranges[4]={source,NULL,NULL,NULL};unsigned lengths[4]={256,0,0,0};
        fh_retained_model_scope_enabled=1;
        assert(fh_retained_model_scope_begin(ranges,lengths));
        assert(!fh_retained_model_scope_begin(ranges,lengths));
#else
        const void *ranges[4]={source,NULL,NULL,NULL};unsigned lengths[4]={256,0,0,0};
        assert(fh_gx_dispatch_begin(ranges,lengths));
#endif
        fh_gx_dispatch_lease_list(source+512,64);
        owned_floor=__atomic_load_n(&queue.read,__ATOMIC_ACQUIRE);
        fh_gx_dispatch_matrix_range(source+1024,64);
        memset(source+1024,pass,48);
        GXLoadPosMtxImm(source+1024,0);GXLoadPosMtxImm(source+1024,0);
#ifdef FH_TEST_MATRIX_BLOCK
        /* Fill the reserved packet itself; the reader cannot observe it
         * before end publishes. The two final rows revisit slots0/1. */
        uint32_t *block=fh_gx_matrix_block_begin(12,3);assert(block);
        if(argc>1 && !strcmp(argv[1],"unfinished-block"))fh_gx_dispatch_barrier();
        if(argc>1 && !strcmp(argv[1],"nested-command"))GXSetVtxDesc(GX_VA_POS,GX_INDEX16);
        unsigned published=__atomic_load_n(&queue.published,__ATOMIC_ACQUIRE);
        for(unsigned i=0;i<12;i++){
            uint32_t *row=fh_gx_matrix_block_row(block,i);
            fh_gx_matrix_block_position(row,source+1024,(i%10)*3,30+(i%10)*3);
            fh_gx_matrix_block_normal(row,source+1024);
        }
        assert(__atomic_load_n(&queue.published,__ATOMIC_ACQUIRE)==published);
        fh_gx_matrix_block_end(block);
#endif
        source[1024]^=0x80;
        GXLoadPosMtxImm(source+1024,0);GXLoadPosMtxImm(source+1024,0);
        memset(source+1024,0xa5,48); /* Copied packets cannot borrow this input. */
        GXSetArray(GX_VA_POS,source+(invalid_array?256:0),256,6,false);
        GXSetVtxDesc(GX_VA_POS,GX_INDEX16);GXCallDisplayList(source+512,64);
        assert(!fh_gx_dispatch_cache_write(source+1023,64));
        assert(!fh_gx_dispatch_cache_write(source+1024,65));
        assert(!fh_gx_dispatch_cache_write(NULL,0));
        memset(source+1024,pass,64);assert(fh_gx_dispatch_cache_write(source+1024,64));
        assert(draws==pass); /* Reader is deliberately stalled. */
        pthread_t release;assert(!pthread_create(&release,NULL,release_reader,NULL));
#ifdef FH_REAL_MODEL_SCOPE
        fh_retained_scope_end();
        assert(!fh_retained_scope_contains(source,256));
#else
        fh_gx_dispatch_end();
#endif
        assert(draws==pass+1 && worker_draws==pass+1 && !fh_gxd_pending());
        assert(cache_events==pass+1);
        assert(!pthread_join(release,NULL));
        unsigned idle_wakes=wakes;
        for(unsigned idle=0;idle<40;idle++)fh_gx_dispatch_barrier();
        assert(wakes==idle_wakes);
        /* Ownership returned only after all queued readers retired. */
        memset(source,0xcc,4096);
    }
#ifdef FH_REAL_MODEL_SCOPE
    /* A completed asynchronous fur/model scope may be followed by a source-
     * only model. Keep the real reader stalled: no command may depend on it. */
    __atomic_store_n(&permit,0,__ATOMIC_RELEASE);
    for(unsigned i=0;i<256;i++)source[i]=(unsigned char)i;
    for(unsigned i=0;i<64;i++)source[512+i]=(unsigned char)(i^0x5a);
    const void *ranges[4]={source,NULL,NULL,NULL};unsigned lengths[4]={256,0,0,0};
    unsigned scopes=fh_gx_dispatch_scopes,records=fh_gx_dispatch_records,waits=fh_gx_dispatch_waits;
    fh_retained_model_scope_enabled=2;assert(fh_retained_model_scope_begin(ranges,lengths));
    fh_gx_dispatch_lease_list(source+512,64);fh_gx_dispatch_matrix_range(source+1024,64);
    GXSetArray(GX_VA_POS,source,256,6,false);GXSetVtxDesc(GX_VA_POS,GX_INDEX16);
    GXCallDisplayList(source+512,64);
    assert(draws==201 && worker_draws==200 && !fh_gxd_pending());
    assert(!fh_gx_dispatch_cache_write(source+1024,64));
    fh_retained_scope_end();
    assert(fh_gx_dispatch_scopes==scopes && fh_gx_dispatch_records==records && fh_gx_dispatch_waits==waits);
    assert(!fh_retained_scope_contains(source,256));memset(source,0xcc,4096);
#endif
    __atomic_store_n(&stopped,1,__ATOMIC_RELEASE);assert(!pthread_join(reader,NULL));
    assert(fh_gx_dispatch_batch_records==fh_gx_dispatch_records && fh_gx_dispatch_batches<fh_gx_dispatch_batch_records);
    assert(fh_gx_matrix_filter_checks==800 && fh_gx_matrix_filtered_records==400);
#ifdef FH_TEST_MATRIX_BLOCK
    assert(fh_gx_matrix_blocks==200 && fh_gx_matrix_block_records==200 && fh_gx_matrix_block_matrices==2400);
    assert(queue.write>RING_WORDS); /* Actual reservation wrap, not a toy queue. */
#endif
    assert(!munmap(source,4096));puts("PASS 200 delayed-consumer scopes: leased lists/arrays remain live through end barrier");
}
