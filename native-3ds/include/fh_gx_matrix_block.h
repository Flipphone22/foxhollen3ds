#ifndef FH_GX_MATRIX_BLOCK_H
#define FH_GX_MATRIX_BLOCK_H
#include <stdint.h>
#include <string.h>
/* One model instruction loads up to twelve position/normal/texture slots.
 * Payloads own their coefficients; only publication is deferred until end.
 * Normal and texture loads share one payload when the game uses the same
 * matrix. No borrowed game matrices and no change to GX load ordering. */
enum { FH_MATRIX_BLOCK_MAX=12,FH_MATRIX_BLOCK_WORDS=2+12*25 };
extern unsigned fh_gx_matrix_block_enabled,fh_gx_matrix_blocks;
extern unsigned fh_gx_matrix_block_matrices,fh_gx_matrix_block_records;
extern unsigned fh_gx_matrix_block_words;
uint32_t *fh_gx_matrix_block_begin(unsigned count,unsigned flags);
void fh_gx_matrix_block_end(uint32_t *packet);
static inline uint32_t *fh_gx_matrix_block_row(uint32_t *p,unsigned i){
    return p?p+2+i*((p[1]>>8)?25:13):NULL;
}
static inline void fh_gx_matrix_block_position(uint32_t *p,const void *m,unsigned id,unsigned tex){
    p[0]=id|(tex<<8);memcpy(p+1,m,48);
}
static inline void fh_gx_matrix_block_normal(uint32_t *p,const void *m){memcpy(p+13,m,48);}
/* Independent of thread/SDK code so malformed packets can be rejected before
 * changing any bank. Texture indices below30 alias position; >=64 select post. */
static inline int fh_gx_matrix_block_apply(const uint32_t *p,unsigned words,
        float position[64][3][4],float normal[64][3][4],
        float texture[64][3][4],float post[64][3][4]){
    if(words<2)return 0;
    unsigned count=p[1]&255,flags=p[1]>>8,stride=flags?25:13;
    if(!count || count>FH_MATRIX_BLOCK_MAX || flags>3 || words!=2+count*stride)return 0;
    for(unsigned i=0;i<count;i++){
        unsigned ids=p[2+i*stride],id=ids&255,tex=(ids>>8)&255;
        unsigned row=tex>=64?tex-64:tex;
        if(ids>>16 || id%3 || id/3>=64 || ((flags&2) && (row%3 || row/3>=64)))return 0;
    }
    for(unsigned i=0;i<count;i++){
        const uint32_t *r=p+2+i*stride;unsigned id=r[0]&255,tex=(r[0]>>8)&255;
        memcpy(position[id/3],r+1,48);
        /* Preserve each original pos -> tex -> normal sequence, including
         * aliases and repeated destination indices inside the same block. */
        if(flags&2)memcpy(tex>=64?post[(tex-64)/3]:tex<30?position[tex/3]:texture[tex/3],r+13,48);
        if(flags&1)memcpy(normal[id/3],r+13,48);
    }
    return 1;
}
#endif
