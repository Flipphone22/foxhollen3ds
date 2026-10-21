#include "gx_internal.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
typedef void (*GXBreakPtCallback)(void);
#include "fh_gx_dispatch_ops.h"
#include "fh_gx_state_filter.h"
#include "fh_gx_matrix_filter.h"
void GXSetTevIndRepeat(GXTevStageID id);
void fhLoadIdentityPosMtx(void);
static FhGxStateFilter filter;
static FhDrawState reference,candidate;
static unsigned checks,removed;
static FhGxMatrixFilter matrices;
static FhGx matrix_reference,matrix_candidate;
static unsigned matrix_removed;
static void matrix_execute(unsigned kind,unsigned id,const void *data,unsigned bytes){
    if(kind==0)GXLoadPosMtxImm(data,id);
    else if(kind==1)GXLoadNrmMtxImm(data,id);
    else if(kind==2)GXLoadTexMtxImm(data,id,bytes==32?GX_MTX2x4:GX_MTX3x4);
    else fhLoadIdentityPosMtx();
}
static void matrix_command(unsigned kind,unsigned id,const void *data,unsigned bytes){
    fh_gx=matrix_reference;matrix_execute(kind,id,data,bytes);matrix_reference=fh_gx;
    fh_gx=matrix_candidate;
    if(kind==3){fh_gx_matrix_filter_identity(&matrices);matrix_execute(kind,id,data,bytes);}
    else if(!fh_gx_matrix_filter_repeat(&matrices,kind,id,data,bytes))matrix_execute(kind,id,data,bytes);
    else matrix_removed++;
    matrix_candidate=fh_gx;assert(!memcmp(&matrix_candidate,&matrix_reference,sizeof(FhGx)));checks++;
}
static void matrix_tests(void){
    fh_gx_matrix_filter_reset(&matrices);
    unsigned char data[52];
    for(unsigned trial=0;trial<2048;trial++){
        for(unsigned i=0;i<sizeof(data);i++)data[i]=(unsigned char)(trial*17+i*31);
        unsigned id=(trial%64)*3,kind=trial%3;
        if(kind==2)id=trial&1?GX_PTTEXMTX0+id:trial%22*3;
        matrix_command(kind,id,data+(trial&3),48);matrix_command(kind,id,data+(trial&3),48);
        if(kind==2){
            data[0]^=1;matrix_command(kind,id,data,32);matrix_command(kind,id,data,32);
            matrix_command(kind,id,data,48);
        }
        /* Low texture matrices alias positions, and partial loads keep row3. */
        matrix_command(0,0,data,48);matrix_command(2,0,data+1,32);matrix_command(0,0,data,48);
        matrix_command(0,27,data,48);matrix_command(3,27,data,48);matrix_command(0,27,data,48);
        if(trial%31==0)fh_gx_matrix_filter_reset(&matrices);
    }
    matrices.epoch=~0u;fh_gx_matrix_filter_reset(&matrices);assert(matrices.epoch==1);
    assert(!fh_gx_matrix_filter_repeat(&matrices,0,1,data,48));
    assert(!fh_gx_matrix_filter_repeat(&matrices,0,192,data,48));
    assert(!fh_gx_matrix_filter_repeat(&matrices,2,GX_PTTEXMTX0+1,data,32));
    assert(matrix_removed>=2048);
    printf("Matrix filter: %u exact overwrites removed; all banks/slots, partial writes, aliases, identity, byte alignment and reset\n",matrix_removed);
}
void fh_platform_panic(const char *s){fprintf(stderr,"GX oracle panic: %s\n",s);abort();}
static void execute(unsigned op,const uint32_t*r){
    unsigned id=r[1];
    switch(op){
    case FH_GXD_GXSetTevColorIn:GXSetTevColorIn(id,r[2],r[3],r[4],r[5]);break;
    case FH_GXD_GXSetTevAlphaIn:GXSetTevAlphaIn(id,r[2],r[3],r[4],r[5]);break;
    case FH_GXD_GXSetTevColorOp:GXSetTevColorOp(id,r[2],r[3],r[4],r[5],r[6]);break;
    case FH_GXD_GXSetTevAlphaOp:GXSetTevAlphaOp(id,r[2],r[3],r[4],r[5],r[6]);break;
    case FH_GXD_GXSetTevOrder:GXSetTevOrder(id,r[2],r[3],r[4]);break;
    case FH_GXD_GXSetTevKColorSel:GXSetTevKColorSel(id,r[2]);break;
    case FH_GXD_GXSetTevKAlphaSel:GXSetTevKAlphaSel(id,r[2]);break;
    case FH_GXD_GXSetTevSwapMode:GXSetTevSwapMode(id,r[2],r[3]);break;
    case FH_GXD_GXSetTevIndirect:GXSetTevIndirect(id,r[2],r[3],r[4],r[5],r[6],r[7],r[8],r[9],r[10]);break;
    case FH_GXD_GXSetTevDirect:GXSetTevDirect(id);break;
    case FH_GXD_GXSetTevIndRepeat:GXSetTevIndRepeat(id);break;
    case FH_GXD_GXSetTevSwapModeTable:GXSetTevSwapModeTable(id,r[2],r[3],r[4],r[5]);break;
    case FH_GXD_GXSetTevOp:GXSetTevOp(id,r[2]);break;
    default:abort();
    }
}
static void command(unsigned op,uint32_t *r,unsigned words){
    fh_gx.draw=reference;execute(op,r);reference=fh_gx.draw;
    fh_gx.draw=candidate;
    if(!fh_gx_state_filter_repeat(&filter,op,r,words))execute(op,r);else removed++;
    candidate=fh_gx.draw;
    assert(!memcmp(&reference,&candidate,sizeof(reference)));checks++;
}
int main(void){
    matrix_tests();
    fh_gx_state_filter_reset(&filter);
    /* A composite setter must invalidate all four independently cached fields. */
    uint32_t color[]={0,2,1,2,3,4},composite[]={0,2,GX_PASSCLR};
    command(FH_GXD_GXSetTevColorIn,color,6);command(FH_GXD_GXSetTevOp,composite,3);command(FH_GXD_GXSetTevColorIn,color,6);
    static const unsigned ops[]={FH_GXD_GXSetTevColorIn,FH_GXD_GXSetTevAlphaIn,FH_GXD_GXSetTevColorOp,FH_GXD_GXSetTevAlphaOp,FH_GXD_GXSetTevOrder,FH_GXD_GXSetTevKColorSel,FH_GXD_GXSetTevKAlphaSel,FH_GXD_GXSetTevSwapMode,FH_GXD_GXSetTevIndirect,FH_GXD_GXSetTevDirect,FH_GXD_GXSetTevIndRepeat,FH_GXD_GXSetTevSwapModeTable,FH_GXD_GXSetTevOp};
    static const unsigned sizes[]={6,6,7,7,5,3,3,4,11,2,2,6,3};
    for(unsigned i=0;i<40000;i++){
        unsigned kind=i%13;uint32_t r[11]={0,(i/13)%16};
        for(unsigned j=2;j<11;j++)r[j]=(i*7+j*3)%4;
        if(kind==11)r[1]%=4;
        if(kind==12)r[2]=(i/13)%5;
        command(ops[kind],r,sizes[kind]);command(ops[kind],r,sizes[kind]);
        if(i%101==0)fh_gx_state_filter_reset(&filter);
    }
    /* Reset/epoch wrap must forget producer state that direct calls may alter. */
    filter.epoch=~0u;fh_gx_state_filter_reset(&filter);assert(filter.epoch==1);
    command(FH_GXD_GXSetTevColorIn,color,6);
    assert(removed>30000);
    printf("%u complete GX-state comparisons, %u redundant overwrites removed; aliases, all slots, resets and epoch wrap\n",checks,removed);
}
