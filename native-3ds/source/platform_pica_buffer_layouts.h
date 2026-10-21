/* SDK fetch formats prepared independently of per-instance live storage.
 * The exact flags are checked against pinned BufInfo_Add in the ARM tests. */
#include "fh_pica_buffers.h"
#define FH_FETCH(stride,count,order) {order,((stride)<<16)|((count)<<28)}
static const FhPicaBufferLayout buffer_layouts[10]={
    {1,{FH_FETCH(64,4,0xff10)}},
    {1,{FH_FETCH(64,5,0xe3210)}},
    {1,{FH_FETCH(20,3,0xd10)}},
    {1,{FH_FETCH(20,4,0x3210)}},
    {1,{FH_FETCH(64,5,0x43210)}},
    {2,{FH_FETCH(64,5,0x43210),FH_FETCH(4,1,6)}},
    {1,{FH_FETCH(40,6,0x543210)}},
    {2,{FH_FETCH(40,6,0x543210),FH_FETCH(4,1,6)}},
    {2,{FH_FETCH(8,1,0),FH_FETCH(32,5,0x54321)}},
    {3,{FH_FETCH(8,1,0),FH_FETCH(32,5,0x54321),FH_FETCH(4,1,6)}}
};
#undef FH_FETCH
static void fh_pica_world_buffers(unsigned compact,unsigned textured,const void *vertices){
    const void *streams[3]={vertices,NULL,NULL};
    if(!fh_pica_buffers_bind(buffer_layouts+(!!compact)*2+!!textured,streams))
        fh_platform_panic("PICA prepared world storage");
}
static void fh_pica_actor_buffers(unsigned compact,const void *vertices,const void *positions,
        const void *attributes,const void *groom){
    const void *streams[3]={positions?positions:vertices,positions?attributes:groom,positions?groom:NULL};
    unsigned layout=positions?8+!!groom:4+(!!compact)*2+!!groom;
    if(!fh_pica_buffers_bind(buffer_layouts+layout,streams))fh_platform_panic("PICA prepared actor storage");
}
