/* Production copy submission against an ordered device queue. Each draw
 * writes a new image epoch; each copy/sample must see its own preceding draw,
 * even when the same framebuffer and capture are reused before CPU joins.
 * Pixel conversion is covered separately by the actual graphics fixture. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
typedef uint32_t u32;typedef uint8_t u8;typedef uint64_t u64;
#include "fh_gpu_transfer.h"
#define GX_BUFFER_DIM(w,h) ((w)|((h)<<16))
#define SYSCLOCK_ARM11 1000
#define GX_CMDLIST_FLUSH 2
#define R_FAILED(result) ((result)<0)
typedef struct {unsigned numEntries,lastEntry,maxEntries;} gxCmdQueue_s;
static struct {gxCmdQueue_s gxQueue;} context;
#define C3Di_GetContext() (&context)
static unsigned gpu_copy_fences,gpu_copy_fence_count,frame_number;
static unsigned efb_ordered_view,efb_view_waits_avoided,pending_sample;
static void *depth_snapshot_storage;
static void *sampled_capture_storage=(void*)0x305fa000;
static unsigned capture_dma_rows,capture_dma_contiguous;
static u64 ticks,queue_join_ticks;
static unsigned pending_draw,source_epoch,capture_epoch[50],depth_epoch;
static unsigned mode,mask,steps,active,completed,stall_at,sampled,pending_returns,capacity_drains;
static struct {unsigned kind,value;} commands[32];
static jmp_buf panic;
static char failure[128];
static void fh_platform_panic(const char *s){snprintf(failure,sizeof(failure),"%s",s);longjmp(panic,1);}
static void execute_one(void){
    gxCmdQueue_s *q=&context.gxQueue;
    if(!active || q->lastEntry==q->numEntries || completed==stall_at)return;
    unsigned kind=commands[q->lastEntry].kind,value=commands[q->lastEntry++].value;++completed;
    if(kind==0)source_epoch=value;
    else if(kind==1)capture_epoch[value]=source_epoch;
    else if(kind==2)depth_epoch=source_epoch;
    else {
        if(mode==2){if(source_epoch!=value)fh_platform_panic("wrong borrowed view epoch");}
        else if(mode){if(depth_epoch!=value)fh_platform_panic("wrong depth epoch");}
        else for(unsigned i=0;i<50;i++)if(capture_epoch[i]!=value)fh_platform_panic("wrong color epoch");
        ++sampled;
    }
}
static void append(unsigned kind,unsigned value){
    gxCmdQueue_s *q=&context.gxQueue;
    if(q->numEntries==q->maxEntries)fh_platform_panic("capacity overflow");
    commands[q->numEntries++]=(typeof(commands[0])){kind,value};
    if(mask&(1u<<(steps++&15)))execute_one();
}
static u64 svcGetSystemTick(void){return ticks;}
static void gxCmdQueueRun(gxCmdQueue_s *q){(void)q;active=1;if(mask&1)execute_one();}
static int gxCmdQueueWait(gxCmdQueue_s *q,int timeout){assert(!timeout);return q->lastEntry==q->numEntries;}
static void svcSleepThread(int ns){assert(ns==100000);++ticks;execute_one();}
static void boot_queue_trace(void){}
static void fh_platform_log(const char *s){(void)s;}
static void C3D_FrameSplit(unsigned flags){
    assert(flags==GX_CMDLIST_FLUSH);gxCmdQueue_s *q=&context.gxQueue;
    if(q->numEntries+4>=q->maxEntries){
        /* The production boundary must complete its bounded capacity wait
         * before the SDK's queue recycling step. */
        if(q->lastEntry!=q->numEntries)fh_platform_panic("unjoined capacity reuse");
        q->numEntries=q->lastEntry=0;++capacity_drains;
    }
    if(pending_draw){append(0,pending_draw);pending_draw=0;}
    if(pending_sample){append(3,pending_sample);pending_sample=0;}
}
static const char *fh_renderer_texture_copy_error(uintptr_t a,size_t b,unsigned c,uintptr_t d,size_t e,unsigned f,unsigned g,unsigned h){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;return NULL;
}
static const char *fh_renderer_capture_dma_error(uintptr_t a,uintptr_t b,unsigned n){
    if(mode)assert(a==0x1f300000 && b==0x1f0c5000 && n==422400);
    else assert(a>=0x1f000000 && a+n<=0x1f067200 && b>=0x305fa000 && b+n<=0x306fa000);
    return NULL;
}
static int GX_RequestDma(u32 *src,u32 *dst,unsigned bytes){
    if(mode){assert((uintptr_t)dst==0x1f0c5000 && bytes==422400);append(2,0);}
    else {
        unsigned row=((uintptr_t)src-0x1f000000)/8448;
        assert(row<50 && bytes==8448 && (uintptr_t)dst==0x305fa000+row*16384);
        append(1,row);
    }
    return 0;
}
static void C3D_SyncTextureCopy(u32*a,unsigned b,u32*c,unsigned d,unsigned e,unsigned f){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;fh_platform_panic("unexpected raw transfer");
}
#include "copy-fence-under-test.inc"
static void run(unsigned schedule,unsigned depth,unsigned stall){
    memset(&context,0,sizeof(context));context.gxQueue.maxEntries=32;
    memset(capture_epoch,0,sizeof(capture_epoch));depth_epoch=source_epoch=0;
    ticks=queue_join_ticks=0;gpu_copy_fences=0;gpu_copy_fence_count=0;
    mode=depth;mask=schedule;steps=active=completed=sampled=pending_returns=capacity_drains=0;
    stall_at=stall;failure[0]=0;pending_draw=0;
    depth_snapshot_storage=depth?(void*)0x1f0c5000:NULL;
    if(!setjmp(panic)){
        for(unsigned epoch=1;epoch<=3;epoch++){
            pending_draw=epoch;
            if(depth)copy_owned_gpu((u32*)0x1f300000,422400,0,(u32*)0x1f0c5000,422400,0,422400,8);
            else copy_owned_gpu((u32*)0x1f000000,422400,0x210,(u32*)0x305fa000,1048576,0x01f00210,422400,12);
            pending_returns+=context.gxQueue.lastEntry<context.gxQueue.numEntries;
            append(3,epoch); /* The next GPU draw samples this copy. */
        }
        gpu_copy_wait("CPU readback",gpu_copy_fence_count);
        assert(stall==UINT32_MAX && sampled==3 && !pending_draw);
        if(!schedule)assert(pending_returns==3);
        if(!depth)assert(capacity_drains>=5);
    }else{
#ifdef FH_NEGATIVE_SPLIT
        assert(strstr(failure,"wrong") || strstr(failure,"capacity"));return;
#else
        assert(stall!=UINT32_MAX && strstr(failure,"timeout") && completed==stall && ticks>=5000);
#endif
    }
}
static void run_view(unsigned schedule,unsigned ordered,unsigned stall){
    memset(&context,0,sizeof(context));context.gxQueue.maxEntries=32;
    ticks=queue_join_ticks=0;source_epoch=0;mode=2;mask=schedule;
    steps=active=completed=sampled=capacity_drains=0;
    stall_at=stall;failure[0]=0;pending_draw=pending_sample=0;
    efb_ordered_view=ordered;efb_view_waits_avoided=gpu_copy_fence_count=0;
    if(!setjmp(panic)){
        for(unsigned epoch=1;epoch<=48;epoch++){
            pending_draw=epoch;efb_view_boundary("view producer");
            pending_sample=epoch;efb_view_boundary("view consumer");
            /* Next iteration overwrites the same source without a CPU join. */
        }
        gpu_copy_wait("CPU readback",++gpu_copy_fence_count);
        assert(stall==UINT32_MAX && sampled==48 && !pending_draw && !pending_sample);
        assert(capacity_drains>0 && efb_view_waits_avoided==(ordered?96:0));
    }else{
#ifdef FH_NEGATIVE_SPLIT
        assert(strstr(failure,"wrong"));return;
#else
        assert(stall!=UINT32_MAX && strstr(failure,"timeout") && completed==stall && ticks>=5000);
#endif
    }
}
int main(void){
#ifdef FH_NEGATIVE_SPLIT
    run(0,1,UINT32_MAX);assert(strstr(failure,"wrong depth epoch"));puts("PASS missing producer split rejected");
#else
    unsigned cases=0;
    for(unsigned m=0;m<64;m++)for(unsigned depth=0;depth<2;depth++){
        run(m*1049u,depth,UINT32_MAX);++cases;
        /* Delays before the producer, within the copy, and at the consumer. */
        for(unsigned stall=0;stall<(depth?9:156);stall++){
            run(m*1049u,depth,stall);++cases;
        }
    }
    for(unsigned m=0;m<16;m++)for(unsigned ordered=0;ordered<2;ordered++){
        run_view(m*1049u,ordered,UINT32_MAX);++cases;
        for(unsigned stall=0;stall<96;stall++){run_view(m*1049u,ordered,stall);++cases;}
    }
    printf("PASS %u ordered copy schedules: image epochs, reuse, pending returns, capacity and bounded stalls\n",cases);
#endif
}
