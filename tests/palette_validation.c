#include "fh_palette_validation.h"
#include <assert.h>
#include <stdio.h>
int fh_memory_compare(const void *a,const void *b,size_t n){return memcmp(a,b,n);}
void *fh_memory_copy(void *a,const void *b,size_t n){return memcpy(a,b,n);}
static FhPaletteCache cache;
static unsigned checks;
static uint32_t seed=0x1839429;
static unsigned random_word(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
    float normal[10][3][4]={0},model[30][4]={0};
    for(unsigned m=0;m<10;m++)for(unsigned r=0;r<3;r++)normal[m][r][r]=model[m*3+r][r]=1;
    assert(fh_palette_validate(normal,model)==(FH_PALETTE_VALID|FH_PALETTE_SHARED));
    for(unsigned i=0;i<60000;i++){
        unsigned part=i%240,m=(part/12)%10,r=(part/4)%3,c=part%4;
        float *value=part<120?&normal[m][r][c]:&model[m*3+r][c];
        float old=*value;
        static const uint32_t special[]={0,0x80000000,0x7fc12345,0x7f800000,0xff800000,1,0x41800000,0x41800001,0x4b189680,0x4b189681};
        uint32_t bits=i%7==0?special[(i/7)%10]:random_word();memcpy(value,&bits,4);
        unsigned mode=(i%16)<<22,expected=fh_palette_validate(normal,model);
        for(unsigned repeat=0;repeat<3;repeat++){assert(fh_palette_cached(&cache,normal,model,mode)==expected);checks++;}
        *value=old;
        assert(fh_palette_cached(&cache,normal,model,mode)==fh_palette_validate(normal,model));checks++;
    }
    assert(cache.hits>120000 && cache.misses>10000);
    printf("palette cache: %u exact decisions, %u hits, %u misses; all coefficient lanes, eviction, FP controls and invalid inputs exercised\n",checks,cache.hits,cache.misses);
}
