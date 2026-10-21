#ifndef FH_GX_MATRIX_FILTER_H
#define FH_GX_MATRIX_FILTER_H
#include <stdint.h>
#include <string.h>
/* Producer-owned register bytes, never borrowed game pointers. The translator's
 * position and low texture IDs alias; 2x4 writes preserve the final row. */
typedef struct { unsigned epoch,known;unsigned char bytes[48]; } FhGxMatrixCell;
typedef struct { unsigned epoch;FhGxMatrixCell cells[4][64]; } FhGxMatrixFilter;
static inline void fh_gx_matrix_filter_reset(FhGxMatrixFilter *f){
    if(!++f->epoch){memset(f,0,sizeof(*f));f->epoch=1;}
}
static inline void fh_gx_matrix_filter_identity(FhGxMatrixFilter *f){f->cells[0][9].epoch=0;}
static inline int fh_gx_matrix_filter_repeat(FhGxMatrixFilter *f,unsigned kind,unsigned id,const void *matrix,unsigned bytes){
    unsigned bank=kind;
    if(kind>2 || (bytes!=32 && bytes!=48) || !matrix)return 0;
    if(kind==2){
        if(id>=GX_PTTEXMTX0){bank=3;id-=GX_PTTEXMTX0;}
        else bank=id<GX_TEXMTX0?0:2;
    }
    if(id%3 || id/3>=64)return 0; /* Let the ordinary decoder reject bad IDs. */
    FhGxMatrixCell *cell=&f->cells[bank][id/3];
    unsigned mask=bytes==32?3:7;
    if(cell->epoch!=f->epoch){cell->epoch=f->epoch;cell->known=0;}
    if((cell->known&mask)==mask && !memcmp(cell->bytes,matrix,bytes))return 1;
    memcpy(cell->bytes,matrix,bytes);cell->known|=mask;return 0;
}
#endif
