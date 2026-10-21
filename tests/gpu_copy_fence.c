/* Device-boundary model for the production copy fence. Model independent
 * producer and transfer latency; old submission permits both to be pending. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
typedef uint32_t u32;
typedef uint8_t u8;
typedef uint64_t u64;
#include "fh_gpu_transfer.h"
#define GX_BUFFER_DIM(w,h) ((w)|((h)<<16))
typedef struct { unsigned numEntries,lastEntry,maxEntries; } gxCmdQueue_s;
static struct { gxCmdQueue_s gxQueue; } context;
static unsigned gpu_copy_fences=1,gpu_copy_fence_count,frame_number;
static u64 ticks,queue_join_ticks;
static void *depth_snapshot_storage;
static void *sampled_capture_storage=(void*)0x305fa000;
static unsigned capture_dma_rows,capture_dma_contiguous;
static unsigned delay,remaining,stuck,phase,submitted,trace,capacity_drains,depth_case;
static unsigned retired,producer_done,peak_pending,halt_row;
static char failure[128];
static jmp_buf panic;
#define SYSCLOCK_ARM11 1000
#define GX_CMDLIST_FLUSH 2
#define R_FAILED(result) ((result)<0)
#define C3Di_GetContext() (&context)
static u64 svcGetSystemTick(void){return ticks;}
static void gxCmdQueueRun(gxCmdQueue_s *q){(void)q;}
static int gxCmdQueueWait(gxCmdQueue_s *q,int timeout){assert(timeout==0);return q->lastEntry==q->numEntries;}
static void svcSleepThread(int ns){
    assert(ns==100000);++ticks;
    if(stuck==phase && (phase==1 || retired==halt_row))return;
    if(remaining)--remaining;
    if(!remaining){
        if(phase==1){context.gxQueue.lastEntry=context.gxQueue.numEntries;producer_done=1;}
        else if(context.gxQueue.lastEntry<context.gxQueue.numEntries){++context.gxQueue.lastEntry;++retired;}
        remaining=delay;
    }
}
static void boot_queue_trace(void){++trace;}
static void fh_platform_panic(const char *text){snprintf(failure,sizeof(failure),"%s",text);longjmp(panic,1);}
static void fh_platform_log(const char *text){(void)text;}
static void C3D_FrameSplit(unsigned flags){
    assert(flags==GX_CMDLIST_FLUSH);
    if(context.gxQueue.numEntries+4>=32){
        assert(context.gxQueue.lastEntry==context.gxQueue.numEntries);
        context.gxQueue.numEntries=context.gxQueue.lastEntry=0;++capacity_drains;
    }
}
static const char *fh_renderer_texture_copy_error(uintptr_t a,size_t b,unsigned c,uintptr_t d,size_t e,unsigned f,unsigned g,unsigned h){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;return NULL;
}
static const char *fh_renderer_capture_dma_error(uintptr_t a,uintptr_t b,unsigned c){
    assert(a>=0x1f000000 && a+c<=0x1f600000);
    if(depth_case==1)assert(b==0x1f0c5000 && c==422400);
    else assert(b>=0x305fa000 && b+c<=0x306fa000);return NULL;
}
static void C3D_SyncTextureCopy(u32*a,unsigned b,u32*c,unsigned d,unsigned e,unsigned f){
    (void)a;(void)b;(void)c;(void)d;(void)e;(void)f;
    if(!producer_done)fh_platform_panic("unretired producer");
    if(context.gxQueue.numEntries>=32)fh_platform_panic("queue capacity exceeded");
    unsigned idle=context.gxQueue.lastEntry==context.gxQueue.numEntries;
    ++submitted;++context.gxQueue.numEntries;phase=2;if(idle)remaining=delay;
    unsigned pending=context.gxQueue.numEntries-context.gxQueue.lastEntry;
    if(pending>peak_pending)peak_pending=pending;
    if(!remaining && (stuck!=phase || retired<halt_row)){++context.gxQueue.lastEntry;++retired;}
}
static int GX_RequestDma(u32*a,u32*b,unsigned c){
    if(depth_case==1){assert((uintptr_t)a==0x1f300000 && (uintptr_t)b==0x1f0c5000 && c==422400);}
    else if(depth_case==2){assert((uintptr_t)a==0x1f000000 && (uintptr_t)b==0x305fa000 && c==819200);}
    else {
        assert((uintptr_t)a==0x1f000000+submitted*8448);
        assert((uintptr_t)b==0x305fa000+submitted*16384);
        assert(c==8448);
    }
    C3D_SyncTextureCopy(a,0,b,0,c,8);return 0;
}
#include "copy-fence-under-test.inc"
static void run(unsigned latency,unsigned halt,unsigned depth,unsigned row){
    depth_case=depth;depth_snapshot_storage=depth==1?(void*)0x1f0c5000:NULL;
    capture_dma_rows=capture_dma_contiguous=0;
    memset(&context,0,sizeof(context));context.gxQueue.numEntries=5;context.gxQueue.maxEntries=32;
    ticks=queue_join_ticks=0;delay=remaining=latency;stuck=halt;phase=1;
    submitted=trace=gpu_copy_fence_count=capacity_drains=0;failure[0]=0;
    retired=producer_done=peak_pending=0;halt_row=row;
    if(!remaining && stuck!=phase){context.gxQueue.lastEntry=5;producer_done=1;}
    if(!setjmp(panic)){
        if(depth==2)copy_owned_gpu((u32*)0x1f000000,819200,0,(u32*)0x305fa000,1048576,0,819200,8);
        else if(depth)copy_owned_gpu((u32*)0x1f300000,422400,0,(u32*)0x1f0c5000,422400,0,422400,8);
        else copy_owned_gpu((u32*)0x1f000000,422400,0x210,(u32*)0x305fa000,1048576,0x01f00210,422400,12);
#if defined(FH_NEGATIVE_CONSUMER) || defined(FH_NEGATIVE_CAPTURE_CONSUMER)
        if(context.gxQueue.lastEntry!=context.gxQueue.numEntries){strcpy(failure,"unretired consumer");return;}
#endif
        assert(!halt && submitted==(depth?1:50) && capacity_drains==!depth && context.gxQueue.lastEntry==(depth?6:27));
        assert(retired==submitted && queue_join_ticks==(depth?2:51)*latency);
        assert(capture_dma_contiguous==(depth==2) && capture_dma_rows==(depth?0:50));
        if(!depth && latency)assert(peak_pending==27);
    }else{
#ifdef FH_NEGATIVE_FENCE
        if(!strcmp(failure,"unretired producer"))return;
#endif
#ifdef FH_NEGATIVE_CAPACITY
        if(!strcmp(failure,"queue capacity exceeded"))return;
#endif
        assert(halt && trace==1 && ticks>=5000 && ticks<=5001+51*latency);
        assert(strstr(failure,halt==1?"producer timeout":depth==2?"capture DMA timeout":depth?"depth DMA timeout":row<23?"row capacity timeout":"row transfer timeout"));
        assert(halt==1?submitted==0:retired==row);
    }
}
int main(void){
#ifdef FH_NEGATIVE_FENCE
    run(3,0,0,0);assert(!strcmp(failure,"unretired producer"));puts("PASS missing fence negative control");
#elif defined(FH_NEGATIVE_CAPACITY)
    run(3,0,0,0);assert(!strcmp(failure,"queue capacity exceeded"));puts("PASS missing capacity reserve negative control");
#elif defined(FH_NEGATIVE_CONSUMER)
    run(3,0,0,0);assert(!strcmp(failure,"unretired consumer"));puts("PASS missing consumer fence negative control");
#elif defined(FH_NEGATIVE_CAPTURE_CONSUMER)
    run(3,0,2,0);assert(!strcmp(failure,"unretired consumer"));puts("PASS missing contiguous capture consumer fence negative control");
#else
    unsigned cases=0;
    for(unsigned depth=0;depth<3;depth++)for(unsigned latency=0;latency<32;latency++){
        run(latency,0,depth,0);run(latency,1,depth,0);cases+=2;
        for(unsigned row=0;row<(depth?1:50);row++){run(latency,2,depth,row);++cases;}
    }
    printf("PASS %u schedules; all color/depth row stalls, producer/consumer fences and capacity drains\n",cases);
#endif
}
