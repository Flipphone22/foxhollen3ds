#include "fh_pica_command_chain.h"
#include <string.h>
#include <stddef.h>
#define HEADER(reg, mask) ((uint32_t)(reg) | (uint32_t)(mask)<<16)
#define SIZE0 HEADER(0x238,15)
#define SIZE1 HEADER(0x239,15)
#define ADDR0 HEADER(0x23a,15)
#define ADDR1 HEADER(0x23b,15)
#define JUMP0 HEADER(0x23c,15)
#define JUMP1 HEADER(0x23d,15)
#define NOP HEADER(0x25e,0)

int fh_pica_chain_draw(uint32_t dst[32],const uint32_t src[30]){
    /* Exact native triangle packet. No untrusted/general command interpreter;
     * its three load-dependent values are index buffer, count and vertex base.
     * Includes the source's two primitive-cache clears and draw-mode teardown.
     */
    static const uint32_t headers[15]={
        HEADER(0x25e,2),HEADER(0x25f,15),HEADER(0x227,15),
        HEADER(0x228,15),HEADER(0x22a,15),HEADER(0x229,2),
        HEADER(0x253,2),HEADER(0x245,1),HEADER(0x22f,15),
        HEADER(0x245,1),HEADER(0x229,2),HEADER(0x253,2),
        HEADER(0x231,15),HEADER(0x25e,8),HEADER(0x25e,8)};
    static const uint32_t values[15]={0,1,0,0,0,0x100,0x100,0,1,1,0,0,1,0,0};
    if(!dst||!src||((uintptr_t)dst&15)||((uintptr_t)src&3))return 0;
    uintptr_t a=(uintptr_t)dst,b=(uintptr_t)src;
    if(a>=b ? a-b<120 : b-a<128)return 0;
    for(unsigned i=0;i<15;i++){
        if(src[2*i+1]!=headers[i])return 0;
        if(i!=0&&i!=2&&i!=3&&i!=4&&src[2*i]!=values[i])return 0;
    }
    /* GPU_GEOMETRY_PRIM, as emitted by native indexed triangle compiler. */
    if(src[0]!=0x300||!src[6]||src[8]||!(src[4]&0x80000000u))return 0;
    memcpy(dst,src,120);dst[30]=1;dst[31]=JUMP1;return 1;
}
unsigned fh_pica_chain_call(uint32_t *w,unsigned capacity,unsigned *cursor,
                           uint32_t target,FhPicaChainReturn *fix){
    return fh_pica_chain_group_call(w,capacity,cursor,target,128,fix);
}
unsigned fh_pica_chain_group_call(uint32_t *w,unsigned capacity,unsigned *cursor,
                           uint32_t target,unsigned bytes,FhPicaChainReturn *fix){
    if(!w||!cursor||!fix||((uintptr_t)w&15)||(*cursor&1)||*cursor>capacity||
       !bytes||bytes>65536||(bytes&15)||!target||(target&15)||target>UINT32_MAX-bytes)return 0;
    unsigned pad=*cursor&2,need=12+pad;
    if(need>capacity-*cursor)return 0;
    unsigned at=*cursor;
    if(pad){w[at++]=0;w[at++]=NOP;}
    *fix=(FhPicaChainReturn){at};
    w[at]=bytes/8;w[at+1]=SIZE0;
    w[at+2]=0;w[at+3]=SIZE1;
    w[at+4]=target>>3;w[at+5]=ADDR0;
    w[at+6]=0;w[at+7]=ADDR1;
    w[at+8]=0;w[at+9]=NOP;
    w[at+10]=1;w[at+11]=JUMP0;
    *cursor+=need;return need;
}
int fh_pica_chain_seal(uint32_t *w,unsigned words,uint32_t phys,
                       const FhPicaChainReturn *fix,unsigned count){
    if(!w||((uintptr_t)w&15)||!words||(words&3)||!phys||(phys&15)||
       words>(UINT32_MAX-phys)/4||words>0x1ffffcu||(count&&!fix))return 0;
    unsigned last=0;
    for(unsigned i=0;i<count;i++){
        unsigned at=fix[i].word;
        if((at&3)||at>words||words-at<=12||(i&&at<last)||
           !w[at]||w[at]>8192||(w[at]&1)||w[at+1]!=SIZE0||w[at+2]||w[at+3]!=SIZE1||
           !w[at+4]||(w[at+4]&1)||w[at+4]>(UINT32_MAX-w[at]*8)/8||
           w[at+5]!=ADDR0||w[at+6]||w[at+7]!=ADDR1||
           w[at+8]||w[at+9]!=NOP||w[at+10]!=1||w[at+11]!=JUMP0)return 0;
        uint32_t target=w[at+4]*8;
        if(target>=phys ? target-phys<words*4 : phys-target<w[at]*8)return 0;
        last=at+12;
    }
    for(unsigned i=0;i<count;i++){
        unsigned at=fix[i].word,ret=at+12;
        w[at+2]=(words-ret)/2;
        w[at+6]=(phys+ret*4)>>3;
    }
    return 1;
}
