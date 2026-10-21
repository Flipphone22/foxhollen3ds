#include <3ds.h>
#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include "fh_gpu_cache.h"
static unsigned direct,service,published,cleaned,fail_direct,fail_service;
static unsigned worker_owner;
static unsigned publication_phase,world_publications,particle_publications;
/* Owned-resource publishers are tested separately. Here their order must
 * precede both command-cache cleaning and exposure to a running GX queue. */
void fh_minimal_publish_world_indices(void){
    assert(publication_phase==0);publication_phase=1;++world_publications;
}
void fh_pica_glow_publish(void){
    assert(publication_phase==1);publication_phase=2;++particle_publications;
}
int fh_audio_worker_is_current(void){return worker_owner;}
static jmp_buf failure;
u64 svcGetSystemTick(void){static u64 t;return ++t;}
void fh_platform_log(const char *message){(void)message;}
void fh_platform_panic(const char *message){(void)message;longjmp(failure,1);}
Result svcFlushProcessDataCache(unsigned handle,u32 address,unsigned bytes){
    assert(publication_phase==2);
    (void)handle;assert(address && bytes);direct++;
    if(fail_direct)return -1;
    cleaned++;publication_phase=3;return 0;
}
Result GSPGPU_FlushDataCache(const void *data,unsigned bytes){
    assert(publication_phase==2);
    assert(data && bytes);service++;if(fail_service)return -1;cleaned++;publication_phase=3;return 0;
}
Result __real_GX_ProcessCommandList(u32 *data,u32 bytes,u8 flags){
    assert(data && bytes);
    assert(!(flags&GX_CMDLIST_FLUSH));assert(cleaned>published);assert(publication_phase==3);publication_phase=0;
    published++;return 0;
}
Result __wrap_GX_ProcessCommandList(u32*,u32,u8);
int main(void){
    u32 commands[32]={0};
    __wrap_GX_ProcessCommandList(commands,sizeof(commands),GX_CMDLIST_FLUSH);
    worker_owner=1; /* Command writer may be the shared render/audio owner. */
    __wrap_GX_ProcessCommandList(commands,sizeof(commands),0); /* internal split */
    assert(published==2 && direct==2 && !service);
    fail_direct=1;__wrap_GX_ProcessCommandList(commands,sizeof(commands),0);
    __wrap_GX_ProcessCommandList(commands,sizeof(commands),GX_CMDLIST_FLUSH);
    assert(published==4 && direct==3 && service==2);
    fail_service=1;
    if(!setjmp(failure)){__wrap_GX_ProcessCommandList(commands,sizeof(commands),0);assert(0);}
    assert(published==4); /* never submit an unflushed segment */
    assert(world_publications==5 && particle_publications==5);
    fh_gpu_cache_report();puts("PASS: native-only world/particle publication before commands, explicit/internal splits, direct flush, sticky fallback, failed flush withheld");
}
