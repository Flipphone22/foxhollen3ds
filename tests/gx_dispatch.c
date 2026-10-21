#include <dolphin/gx.h>
#include <dolphin/mtx.h>
#include "fh_gx_dispatch.h"
#include "fh_gx_ring.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Legacy Dolphin callback, supplied by the ARM compatibility headers. */
typedef void (*GXBreakPtCallback)(void);
static FhGxRing queue;
static uint32_t storage[256] __attribute__((aligned(32)));
static unsigned fh_gxd_recording=1,calls,expected_filter,expected_type;
static unsigned char expected_matrix[48],expected_samples[24],expected_weights[7];
static uint32_t *fh_gxd_reserve(unsigned words){uint32_t*r=fh_gx_ring_reserve(&queue,words);assert(r);return r;}
static void fh_gxd_commit(unsigned op){assert(fh_gx_ring_commit(&queue,op));}
static void fh_gxd_barrier(unsigned reason){(void)reason;fh_gx_dispatch_barrier();}
#define LOAD(name,type) static type fh_gxd_load_##name(const uint32_t*r){type v;memcpy(&v,r,sizeof(v));return v;}
LOAD(f32,f32) LOAD(GXColor,GXColor) LOAD(GXColorS10,GXColorS10)
#undef LOAD
#define FH_GX_DISPATCH_RECORDERS
#include "fh_gx_dispatch_ops.h"
#include "test-stubs.h"
void fh_gx_dispatch_barrier(void){assert(!"Unexpected barrier");}
void fh_gxd_GXSetTevSwapModeTable(GXTevSwapSel id,GXTevColorChan r,GXTevColorChan g,GXTevColorChan b,GXTevColorChan a){
    assert(id==GX_TEV_SWAP2 && r==GX_CH_BLUE && g==GX_CH_RED && b==GX_CH_ALPHA && a==GX_CH_GREEN);++calls;
}
void fh_gxd_GXSetTevColorS10(GXTevRegID reg,GXColorS10 color){
    assert(reg==GX_TEVREG1 && color.r==-1024 && color.g==1023 && color.b==-13 && color.a==7);++calls;
}
void fh_gxd_GXSetViewport(f32 x,f32 y,f32 width,f32 height,f32 nearZ,f32 farZ){
    uint32_t bits;memcpy(&bits,&x,4);assert(bits==0x80000000);
    memcpy(&bits,&y,4);assert(bits==0x7fc12345);
    assert(width==400.25f && height==-240.5f && nearZ==0.25f && farZ==1.0f);++calls;
}
void fh_gxd_GXLoadTexMtxImm(const void *matrix,u32 id,GXTexMtxType type){
    assert(id==GX_TEXMTX2 && type==expected_type);
    assert(!memcmp(matrix,expected_matrix,type==GX_MTX2x4?32:48));++calls;
}
void fh_gxd_GXSetCopyFilter(GXBool aa,u8 samples[12][2],GXBool vf,u8 filter[7]){
    assert(aa==GX_TRUE && vf==GX_FALSE);
    if(expected_filter){assert(samples && filter);assert(!memcmp(samples,expected_samples,24) && !memcmp(filter,expected_weights,7));}
    else assert(!samples && !filter);
    ++calls;
}
void fh_gxd_GXSetArray(GXAttr attr,const void*data,u32 bytes,u8 stride,bool little_endian){
    assert(attr==GX_VA_POS && (uintptr_t)data==0x12345678 && bytes==4321 && stride==12 && little_endian);++calls;
}
static void drain(unsigned count){
    const uint32_t *r;unsigned words,op,seen=0;fh_gx_ring_publish(&queue);
    while(fh_gx_ring_peek(&queue,&r,&words,&op)==FH_GX_RING_READY){
        unsigned before=calls;
        for(unsigned truncated=0;truncated<words;truncated++)assert(!fh_gxd_replay(op,r,truncated));
        assert(!fh_gxd_replay(op,r,words+1) && calls==before);
        assert(fh_gxd_replay(op,r,words) && calls==before+1);
        assert(fh_gx_ring_release(&queue,r,words));++seen;
    }
    assert(seen==count);
}
int main(void){
    assert(fh_gx_ring_init(&queue,storage,256));
    for(unsigned iteration=0;iteration<1000;iteration++){
        GXSetTevSwapModeTable(GX_TEV_SWAP2,GX_CH_BLUE,GX_CH_RED,GX_CH_ALPHA,GX_CH_GREEN);
        GXSetTevColorS10(GX_TEVREG1,(GXColorS10){-1024,1023,-13,7});
        uint32_t bits[2]={0x80000000,0x7fc12345};float f[2];memcpy(f,bits,8);
        GXSetViewport(f[0],f[1],400.25f,-240.5f,0.25f,1.0f);
        GXSetArray(GX_VA_POS,(void*)(uintptr_t)0x12345678,4321,12,true);
        float matrix[3][4];for(unsigned i=0;i<48;i++)((unsigned char*)matrix)[i]=(unsigned char)(iteration+i*17);
        memcpy(expected_matrix,matrix,48);expected_type=iteration%2?GX_MTX2x4:GX_MTX3x4;
        GXLoadTexMtxImm(matrix,GX_TEXMTX2,expected_type);memset(matrix,0xcc,48);
        u8 samples[12][2],weights[7];memset(samples,iteration,24);memset(weights,iteration+1,7);
        memcpy(expected_samples,samples,24);memcpy(expected_weights,weights,7);expected_filter=iteration%2;
        GXSetCopyFilter(GX_TRUE,expected_filter?samples:NULL,GX_FALSE,expected_filter?weights:NULL);
        memset(samples,0x5a,24);memset(weights,0xa5,7);
        assert(calls==iteration*6);drain(6);
    }
    assert(calls==6000);assert(!fh_gxd_replay(254,storage,1));
    puts("PASS 6000 typed replays: signed colors, float bits, pointer addresses, nullable snapshots, matrix lengths, source mutation, truncated/oversized rejection");
}
