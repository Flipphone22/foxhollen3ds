#ifndef FH_GX_STATE_FILTER_H
#define FH_GX_STATE_FILTER_H
#include <stdint.h>
#include <string.h>
/* Producer-only TEV register mirror. Skip only an exact repeat of an overwrite
 * whose aliases are known. No texture/array lifetime or floating-point values
 * are inferred here. The original GX functions remain the semantic oracle. */
typedef struct { uint32_t epoch,op,words,data[10]; } FhGxStateEntry;
typedef struct { FhGxStateEntry cells[16*9+4];uint32_t epoch; } FhGxStateFilter;
static inline void fh_gx_state_filter_reset(FhGxStateFilter *f){
    if(!++f->epoch){memset(f,0,sizeof(*f));f->epoch=1;}
}
static inline int fh_gx_state_filter_repeat(FhGxStateFilter *f,unsigned op,const uint32_t *r,unsigned words){
    if(words<2 || words>11)return 0;
    unsigned id=r[1],field;
    switch(op){
    case FH_GXD_GXSetTevOp:
        /* This helper overwrites four independent fields through other GX
         * setters; keep it and retire each corresponding cached overwrite. */
        if(id<16)for(unsigned i=0;i<4;i++)f->cells[id*9+i].epoch=0;
        return 0;
    case FH_GXD_GXSetTevColorIn:field=0;break;
    case FH_GXD_GXSetTevAlphaIn:field=1;break;
    case FH_GXD_GXSetTevColorOp:field=2;break;
    case FH_GXD_GXSetTevAlphaOp:field=3;break;
    case FH_GXD_GXSetTevOrder:field=4;break;
    case FH_GXD_GXSetTevKColorSel:field=5;break;
    case FH_GXD_GXSetTevKAlphaSel:field=6;break;
    case FH_GXD_GXSetTevSwapMode:field=7;break;
    case FH_GXD_GXSetTevIndirect:
    case FH_GXD_GXSetTevDirect:
    case FH_GXD_GXSetTevIndRepeat:field=8;break;
    case FH_GXD_GXSetTevSwapModeTable:if(id>=4)return 0;id+=16*9;goto selected;
    default:return 0;
    }
    if(id>=16)return 0;
    id=id*9+field;
selected:;
    FhGxStateEntry *e=f->cells+id;
    if(e->epoch==f->epoch && e->op==op && e->words==words && !memcmp(e->data,r+1,(words-1)*4))return 1;
    e->epoch=f->epoch;e->op=op;e->words=words;memcpy(e->data,r+1,(words-1)*4);return 0;
}
#endif
