#include "fh_owned_draw_sources.h"
#include "fh_source_identity.h"
#include "fh_compiled_geometry.h"
#include "fh_model_resources.h"
#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sanitizer/asan_interface.h>

static unsigned char arena[256*1024] __attribute__((aligned(32)));
static FhOwnedSources pool;
const FhOwnedSourceLease *fh_owned_active_sources;
static unsigned char compiled_storage[512*1024] __attribute__((aligned(8)));
void *fh_geometry_compiled_storage(void){return compiled_storage;}
int fh_memory_compare(const void *a,const void *b,size_t bytes){return memcmp(a,b,bytes);}
void *fh_memory_copy(void *a,const void *b,size_t bytes){return memcpy(a,b,bytes);}
static unsigned checks;
void fh_platform_panic(const char *message){fprintf(stderr,"%s\n",message);abort();}
/* This fixture exercises owned copies; model resource fences have a separate
 * production-directory fixture and must never be called with this mode off. */
void fh_gx_dispatch_barrier(void){assert(!fh_model_resources_mode);}
static int ready(const void *data,size_t bytes,void *blocked){return data!=blocked && bytes>0;}
/* Keep the production operation out of the caller's constant propagation.
 * An empty retirement must not read/write the source directory at all. */
__attribute__((noinline)) int test_release(FhOwnedSources *p,FhOwnedSourceLease *l){return fh_owned_source_release(p,l);}
__attribute__((noinline)) int test_forget(FhOwnedSources *p,FhOwnedSourceLease *l){return fh_owned_source_forget(p,l,NULL,0);}
static void empty_retirement(void){
    unsigned char bytes[8]={1};FhOwnedSourceLease live={0},empty={0},bad={.count=FH_OWNED_LEASE_SOURCES+1};
    assert(fh_owned_source_acquire(&pool,&live,bytes,sizeof(bytes)));
    __asan_poison_memory_region(pool.entry,sizeof(pool.entry));
    assert(test_release(&pool,&empty) && test_forget(&pool,&empty));
    assert(!test_release(&pool,&bad) && !test_forget(&pool,&bad));
    assert(!test_release(NULL,&empty) && !test_forget(NULL,&empty));
    __asan_unpoison_memory_region(pool.entry,sizeof(pool.entry));
    assert(pool.entry[live.source[0].slot].pins==1);
    assert(fh_owned_source_release(&pool,&live));checks+=8;
}
static void pressure(void){
    unsigned char space[128] __attribute__((aligned(8))),a[64],b[64],before[128];
    FhOwnedSources p;FhOwnedSourceLease first={0},second={0},failed={0};
    memset(a,1,sizeof(a));memset(b,2,sizeof(b));assert(fh_owned_sources_init(&p,space,sizeof(space)));
    assert(fh_owned_source_acquire(&p,&first,a,sizeof(a)));
    memset(a,3,sizeof(a));assert(fh_owned_source_acquire(&p,&second,a,sizeof(a)));
    assert(first.source[0].data[0]==1 && second.source[0].data[0]==3);
    memcpy(before,space,sizeof(before));
    assert(!fh_owned_source_acquire(&p,&failed,b,sizeof(b)) && !failed.count);
    assert(!memcmp(before,space,sizeof(before)));
    FhOwnedSourceLease stale=first;
    assert(fh_owned_source_release(&p,&first));
    assert(fh_owned_source_acquire(&p,&failed,b,sizeof(b)));
    assert(!fh_owned_source_release(&p,&stale));
    assert(second.source[0].data[0]==3);
    assert(fh_owned_source_release(&p,&second));assert(fh_owned_source_release(&p,&failed));
    assert(p.peak<=sizeof(space));checks+=10;
    FhOwnedSourceLease alias={0};
    assert(fh_owned_source_acquire(&p,&alias,a,64));
    assert(fh_owned_source_acquire(&p,&alias,a+7,19) && alias.count==1);
    assert(fh_owned_source_resolve(&alias,a+7,19)==alias.source[0].data+7);
    assert(!fh_owned_source_resolve(&alias,a+60,5));
    a[8]^=1;assert(!fh_owned_source_acquire(&p,&alias,a+7,19));
    assert(fh_owned_source_release(&p,&alias));checks+=6;
}
static void invalid_release(void){
    FhOwnedSourceLease a={0},b={0};unsigned char input[32]={0};
    assert(fh_owned_source_acquire(&pool,&a,input,32));
    assert(fh_owned_source_acquire(&pool,&b,input,32));
    assert(a.source[0].data==b.source[0].data);
    FhOwnedSourceLease broken=a;broken.source[1]=a.source[0];broken.source[2]=a.source[0];broken.count=3;
    assert(!fh_owned_source_release(&pool,&broken));
    assert(pool.entry[a.source[0].slot].pins==2);
    assert(fh_owned_source_release(&pool,&a));assert(fh_owned_source_release(&pool,&b));checks+=7;
}
static void pinned_islands(void){
    unsigned char storage[128] __attribute__((aligned(8))),input[8][16];
    FhOwnedSources p;FhOwnedSourceLease held[8]={0},extra={0};
    assert(fh_owned_sources_init(&p,storage,sizeof(storage)));
    for(unsigned i=0;i<8;i++){memset(input[i],i,16);assert(fh_owned_source_acquire(&p,&held[i],input[i],16));}
    /* Cursor is at the end, start and middle remain live. Allocation must
     * find a later hole without moving either immutable reader address. */
    assert(fh_owned_source_release(&p,held+1));assert(fh_owned_source_release(&p,held+3));
    input[1][0]=99;assert(fh_owned_source_acquire(&p,&extra,input[1],16));
    assert(extra.source[0].data==storage+16);
    for(unsigned i=0;i<8;i++)if(held[i].count){
        assert(held[i].source[0].data==storage+i*16 && held[i].source[0].data[0]==i);
        assert(fh_owned_source_release(&p,held+i));
    }
    assert(fh_owned_source_release(&p,&extra));checks+=25;
}
static void identity_versions(void){
    unsigned char list[16]={0x90,0,3,0,1,2};
    FhOwnedSourceLease old={0},newer={0};
    assert(fh_owned_source_acquire(&pool,&old,list,sizeof(list)));
    list[3]=2;assert(fh_owned_source_acquire(&pool,&newer,list,sizeof(list)));
    assert(old.source[0].data!=newer.source[0].data);
    fh_owned_active_sources=&old;
    assert(fh_source_identity(old.source[0].data+3,1)==list+3);
    unsigned version=fh_source_version(old.source[0].data,16);assert(version);
    const FhCompiledMesh *mesh=fh_compiled_mesh(old.source[0].data,16,1,0);assert(mesh && mesh->unique==3);
    /* Same original pointer, DIFFERENT owned bytes: a cached topology must
     * reject the old content instead of treating identity as validity. */
    fh_owned_active_sources=&newer;
    assert(fh_source_identity(newer.source[0].data,16)==list);
    assert(fh_source_version(newer.source[0].data,16)!=version);
    mesh=fh_compiled_mesh(newer.source[0].data,16,1,0);assert(mesh && mesh->unique==2);
    fh_owned_active_sources=&old;
    mesh=fh_compiled_mesh(old.source[0].data,16,1,0);assert(mesh && mesh->unique==3);
    fh_owned_active_sources=NULL;
    assert(fh_owned_source_release(&pool,&old));assert(fh_owned_source_release(&pool,&newer));checks+=12;
}
static void shared_versions(void){
    unsigned char input[64]={7};FhOwnedSourceLease owner={0},draw={0},newer={0},bad={0};
    assert(fh_owned_source_acquire(&pool,&owner,input,sizeof(input)));
    unsigned copied=pool.copies,reused=pool.reuses;
    /* Sharing is an ownership transfer, not a reread of the current original.
     * Test it AFTER overwrite to detect an accidental source dereference. */
    memset(input,9,sizeof(input));
    assert(fh_owned_source_share(&pool,&draw,&owner,input+3,17));
    assert(pool.copies==copied && pool.reuses==reused);
    assert(fh_owned_source_resolve(&draw,input,64)==owner.source[0].data);
    assert(fh_owned_source_acquire(&pool,&newer,input,64));
    assert(!fh_owned_source_share(&pool,&newer,&owner,input,64));
    FhOwnedSourceLease stale=owner;stale.source[0].generation++;
    assert(!fh_owned_source_share(&pool,&bad,&stale,input,64) && !bad.count);
    assert(!fh_owned_source_share(&pool,&bad,&owner,input+60,5));
    assert(fh_owned_source_release(&pool,&owner));
    assert(draw.source[0].data[0]==7 && newer.source[0].data[0]==9);
    assert(fh_owned_source_release(&pool,&draw));assert(fh_owned_source_release(&pool,&newer));checks+=12;
}
static void texture_dependencies(void){
    unsigned char list[16]={0x90,0,3,0,1,2},pixels[32]={0};
    FhGx state={0};FhOwnedSourceLease lease={0};state.draw.stages=1;
    state.draw.textures[0]=(FhTexture){.pixels=pixels,.width=8,.height=8,.format=8};
    /* Stale unused binding deliberately has no readable backing. Capture and
     * binding must not dereference it. Activating it must decline before read. */
    const void *deferred=(const void*)(uintptr_t)1;
    state.draw.textures[7]=(FhTexture){.pixels=deferred,.width=8,.height=8,.format=8};
    assert(fh_texture_dependencies(&state.draw)==1);
    assert(fh_owned_draw_capture(&pool,&lease,&state,list,16,ready,(void*)deferred));
    FhGx consumer=state;assert(fh_owned_draw_bind(&consumer,&lease,list,16));
    assert(consumer.draw.textures[7].pixels==deferred);
    assert(fh_owned_source_release(&pool,&lease));
    state.draw.indirect_count=1;state.draw.indirect_order[0][1]=7;
    assert(fh_texture_dependencies(&state.draw)==129);
    assert(!fh_owned_draw_capture(&pool,&lease,&state,list,16,ready,(void*)deferred) && !lease.count);
    state.draw.indirect_count=0;state.draw.tev[0].map=7;
    assert(!fh_owned_draw_capture(&pool,&lease,&state,list,16,ready,(void*)deferred) && !lease.count);
    state.draw.stages=17;assert(fh_texture_dependencies(&state.draw)==255);checks+=9;
}
static void resource_epochs(unsigned untracked){
    unsigned char list[16]={0x90,0,3,0,1,2},pixels[32]={0};
    uint16_t palette[16]={0,0xffff};
    FhGx state={0};state.draw.stages=1;
    state.draw.textures[0]=(FhTexture){.pixels=pixels,.palette=palette,.width=8,.height=8,
        .format=8,.palette_format=1,.palette_count=16};
    FhOwnedSourceLease resources={0},draws[3]={0};
    assert(fh_owned_draw_capture_resources(&pool,draws,NULL,NULL,&resources,1,&state,list,16,ready,NULL));
    unsigned compared=pool.reuses;
    assert(fh_owned_draw_capture_resources(&pool,draws+1,NULL,NULL,&resources,1,&state,list,16,ready,NULL));
    /* List acquisition still checks bytes, texture readers share the same
     * immutable resource version. A partial write invalidates that version. */
    assert(pool.reuses==compared+1);
    const unsigned char *old=fh_owned_source_resolve(draws,pixels,32);
    assert(old==fh_owned_source_resolve(draws+1,pixels,32));
    pixels[0]=0x10;
    if(!untracked)assert(fh_owned_source_forget(&pool,&resources,pixels+1,1));
    assert(fh_owned_draw_capture_resources(&pool,draws+2,NULL,NULL,&resources,1,&state,list,16,ready,NULL));
    const unsigned char *newer=fh_owned_source_resolve(draws+2,pixels,32);
    assert(old!=newer && old[0]==0 && newer[0]==0x10);
    assert(fh_owned_source_release(&pool,&resources));
    memset(pixels,0xa5,sizeof(pixels));memset(palette,0xa5,sizeof(palette));
    unsigned char outputs[3][128] __attribute__((aligned(8)));
    for(unsigned i=0;i<3;i++){
        FhGx consumer=state;assert(fh_owned_draw_bind(&consumer,draws+i,list,16));
        assert(fh_texture_convert(outputs[i],128,8,8,&consumer.draw.textures[0],32));
        assert(fh_owned_source_release(&pool,draws+i));
    }
    assert(!memcmp(outputs[0],outputs[1],128) && memcmp(outputs[0],outputs[2],128));checks+=20;
}
static unsigned published,completed,stopping;
typedef struct {
    FhGx gx;
    FhOwnedSourceLease lease;
    const void *list;
    unsigned number;
    unsigned char expected_pixels[128];
} Job;
static Job job;
static void *reader(void *unused){
    (void)unused;unsigned last=0;
    while(!__atomic_load_n(&stopping,__ATOMIC_ACQUIRE)){
        unsigned next=__atomic_load_n(&published,__ATOMIC_ACQUIRE);
        if(next==last){sched_yield();continue;}
        /* Producer has already overwritten ALL original source bytes. The
         * actual texture converter and indexed attribute reader see copies. */
        const unsigned char *list=fh_owned_draw_bind(&job.gx,&job.lease,job.list,16);
        fh_owned_active_sources=&job.lease;
        assert(fh_source_identity(list,16)==job.list);
        assert(list && list[0]==0x90 && list[1]==0 && list[2]==3);
        const float (*positions)[3]=(const void*)job.gx.arrays[GX_VA_POS].data;
        for(unsigned i=0;i<3;i++)assert(positions[list[3+i]][0]==(float)(job.number+i));
        unsigned char converted[128] __attribute__((aligned(8)));
        assert(fh_texture_convert(converted,sizeof(converted),8,8,&job.gx.draw.textures[0],32));
        assert(!memcmp(converted,job.expected_pixels,sizeof(converted)));
        const FhCompiledMesh *mesh=fh_compiled_mesh(list,16,1,0);assert(mesh && mesh->unique==3 && mesh->count==3);
        fh_owned_active_sources=NULL;
        last=next;__atomic_store_n(&completed,last,__ATOMIC_RELEASE);
    }
    return NULL;
}
typedef struct {const void *positions,*list;} ModelContract;
static int immutable(const void *data,size_t bytes,void *context){
    const ModelContract *m=context;return (data==m->positions && bytes==36) || (data==m->list && bytes==16);
}
static void model_batches(void){
    Job *batch=calloc(16,sizeof(*batch));assert(batch);unsigned sequence=256;
    for(unsigned model_number=1;model_number<=8;model_number++){
        float positions[3][3]={{model_number,0,0},{model_number+1,1,0},{model_number+2,0,1}};
        unsigned char list[16]={0x90,0,3,0,1,2},pixels[32];uint16_t palette[16];
        ModelContract contract={positions,list};FhOwnedSourceLease model={0};
        unsigned previous_shares=pool.shares;
        for(unsigned draw=0;draw<16;draw++){
            Job *next=batch+draw;memset(next,0,sizeof(*next));next->number=model_number;next->list=list;
            next->gx.descriptors[GX_VA_POS]=GX_INDEX8;next->gx.draw.stages=1;
            next->gx.arrays[GX_VA_POS]=(FhVtxArray){(const void*)positions,sizeof(positions),12,1};
            for(unsigned i=0;i<32;i++)pixels[i]=(unsigned char)(draw*17+i);
            for(unsigned i=0;i<16;i++)palette[i]=(uint16_t)(model_number*101+draw*257+i*157);
            next->gx.draw.textures[0]=(FhTexture){.pixels=pixels,.palette=palette,.width=8,.height=8,
                .format=8,.palette_format=1,.palette_count=16};
            assert(fh_texture_convert(next->expected_pixels,128,8,8,&next->gx.draw.textures[0],32));
            assert(fh_owned_draw_capture_model(&pool,&next->lease,&model,immutable,&next->gx,list,16,ready,&contract));
            assert(next->lease.source[0].data==batch[0].lease.source[0].data);
            assert(next->lease.source[1].data==batch[0].lease.source[1].data);
        }
        assert(model.count==2 && pool.shares-previous_shares==32);
        /* Closing the model owner does not revoke queued child draws. All
         * geometry and mutable texture originals die before any consumption. */
        assert(fh_owned_source_release(&pool,&model));
        memset(positions,0xa5,sizeof(positions));memset(list,0xa5,sizeof(list));
        memset(pixels,0xa5,sizeof(pixels));memset(palette,0xa5,sizeof(palette));
        for(unsigned draw=0;draw<16;draw++){
            job=batch[draw];batch[draw].lease.count=0;
            __atomic_store_n(&published,++sequence,__ATOMIC_RELEASE);
            while(__atomic_load_n(&completed,__ATOMIC_ACQUIRE)!=sequence)sched_yield();
            assert(fh_owned_source_release(&pool,&job.lease));
        }
        checks+=82;
    }
    free(batch);
}
int main(int argc,char **argv){
    assert(fh_owned_sources_init(&pool,arena,sizeof(arena)));empty_retirement();pressure();invalid_release();pinned_islands();identity_versions();shared_versions();texture_dependencies();
    resource_epochs(argc>1 && !strcmp(argv[1],"untracked-texture-write"));
    pthread_t worker;assert(!pthread_create(&worker,NULL,reader,NULL));
    for(unsigned iteration=1;iteration<=256;iteration++){
        float positions[3][3]={{iteration,0,0},{iteration+1,1,0},{iteration+2,0,1}};
        unsigned char list[16]={0x90,0,3,0,1,2},pixels[32];
        uint16_t palette[16];for(unsigned i=0;i<16;i++)palette[i]=(uint16_t)(iteration+i*157);
        for(unsigned i=0;i<32;i++)pixels[i]=(unsigned char)(iteration+i);
        FhGx gx={0};gx.descriptors[GX_VA_POS]=GX_INDEX8;gx.draw.stages=1;
        gx.arrays[GX_VA_POS]=(FhVtxArray){(const void*)positions,sizeof(positions),12,1};
        gx.draw.textures[0]=(FhTexture){.pixels=pixels,.palette=palette,.width=8,.height=8,
            .format=8,.palette_format=1,.palette_count=16};
        job.gx=gx;job.number=iteration;job.list=list;
        assert(fh_texture_convert(job.expected_pixels,sizeof(job.expected_pixels),8,8,&gx.draw.textures[0],32));
        assert(fh_owned_draw_capture(&pool,&job.lease,&gx,list,sizeof(list),ready,NULL));
        assert(job.lease.count==4);
        FhGx bad=gx,unchanged;bad.arrays[GX_VA_POS].bytes++;unchanged=bad;
        assert(!fh_owned_draw_bind(&bad,&job.lease,list,16));assert(!memcmp(&bad,&unchanged,sizeof(bad)));
        /* A deferred texture must decline the complete transaction and leave
         * the queued draw's pins intact, including shared list/array copies. */
        FhOwnedSourceLease declined={0};
        assert(!fh_owned_draw_capture(&pool,&declined,&gx,list,16,ready,pixels) && !declined.count);
        memset(positions,0x55,sizeof(positions));memset(pixels,0x55,sizeof(pixels));
        memset(palette,0x55,sizeof(palette));memset(list,0x55,sizeof(list));
        __atomic_store_n(&published,iteration,__ATOMIC_RELEASE);
        while(__atomic_load_n(&completed,__ATOMIC_ACQUIRE)!=iteration)sched_yield();
        assert(fh_owned_source_release(&pool,&job.lease));checks+=10;
    }
    model_batches();
    __atomic_store_n(&stopping,1,__ATOMIC_RELEASE);assert(!pthread_join(worker,NULL));
    for(unsigned i=0;i<FH_OWNED_SOURCE_SLOTS;i++)assert(!pool.entry[i].pins);
    printf("%u checks; 384 delayed draws including 8 shared model batches, original arrays/lists/pixels/palettes overwritten; arena peak %u/%u\n",checks,pool.peak,pool.capacity);
    return 0;
}
