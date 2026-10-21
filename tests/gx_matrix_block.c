#include "fh_gx_matrix_block.h"
#ifdef FH_MATRIX_BLOCK_ARM
volatile unsigned test_checks,test_failure;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned line){test_failure=line;for(;;)__asm__ volatile("nop");}
#define assert(v) do{if(!(v))test_fail(__LINE__);}while(0)
#define checks test_checks
#define PASSES 5
#else
#include <assert.h>
static unsigned checks;
#define PASSES 200
#endif
#include <stdio.h>
typedef struct {float p[64][3][4],n[64][3][4],t[64][3][4],x[64][3][4];} Banks;
static Banks actual,expected,before;
static unsigned seed=0xa1643029;
static unsigned next(void){seed=seed*1664525+1013904223;return seed;}
static void original(Banks *b,const float pos[12],const float normal[12],unsigned id,unsigned tex,unsigned flags){
    memcpy(b->p[id/3],pos,48);
    if(flags&2){
        float (*dst)[4];
        if(tex>=64)dst=b->x[(tex-64)/3];
        else if(tex<30)dst=b->p[tex/3];else dst=b->t[tex/3];
        memcpy(dst,normal,48);
    }
    if(flags&1)memcpy(b->n[id/3],normal,48);
}
#ifdef FH_MATRIX_BLOCK_ARM
void test_main(void){
#else
int main(void){
#endif
    uint32_t packet[FH_MATRIX_BLOCK_WORDS];
    for(unsigned pass=0;pass<PASSES;pass++)for(unsigned count=1;count<=12;count++)for(unsigned flags=0;flags<4;flags++){
        memset(&actual,(int)pass,sizeof(actual));expected=actual;
        packet[1]=count|(flags<<8);
        unsigned words=2+count*(flags?25:13);
        for(unsigned i=0;i<count;i++){
            float pos[12],normal[12];
            for(unsigned j=0;j<12;j++){unsigned a=next(),b=next();memcpy(pos+j,&a,4);memcpy(normal+j,&b,4);}
            unsigned id=(next()%64)*3,tex;
            switch(i%3){case 0:tex=(next()%10)*3;break;case 1:tex=30+(next()%12)*3; if(tex>=64)tex=63;break;default:tex=64+(next()%64)*3;}
            uint32_t *r=fh_gx_matrix_block_row(packet,i);
            fh_gx_matrix_block_position(r,pos,id,tex);
            if(flags)fh_gx_matrix_block_normal(r,normal);
            original(&expected,pos,normal,id,tex,flags);
            memset(pos,0,48);memset(normal,0,48);
        }
        assert(fh_gx_matrix_block_apply(packet,words,actual.p,actual.n,actual.t,actual.x));
        assert(!memcmp(&actual,&expected,sizeof(actual)));checks+=2;
        before=actual;
        assert(!fh_gx_matrix_block_apply(packet,words-1,actual.p,actual.n,actual.t,actual.x));
        assert(!memcmp(&actual,&before,sizeof(actual)));checks+=2;
        unsigned at=2+(count-1)*(flags?25:13),saved=packet[at];packet[at]|=0x10000;
        assert(!fh_gx_matrix_block_apply(packet,words,actual.p,actual.n,actual.t,actual.x));
        assert(!memcmp(&actual,&before,sizeof(actual)));checks+=2;
        packet[at]=saved;packet[1]|=0x400;
        assert(!fh_gx_matrix_block_apply(packet,words,actual.p,actual.n,actual.t,actual.x));
        assert(!memcmp(&actual,&before,sizeof(actual)));checks+=2;
    }
#ifdef FH_MATRIX_BLOCK_ARM
    test_pass();
#else
    printf("PASS %u matrix bank/original-setter and atomic rejection checks\n",checks);
#endif
}
