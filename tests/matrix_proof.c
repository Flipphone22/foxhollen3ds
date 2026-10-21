#include "fh_matrix_proof.h"
#include <assert.h>
#include <stdio.h>
static FhMatrixProofCache cache;
static uint32_t seed=0x2938fabe;
static uint32_t random_word(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(void){
    float inputs[3][3][4]={0};
    for(unsigned part=0;part<3;part++)for(unsigned r=0;r<3;r++)inputs[part][r][r]=1;
    assert(fh_matrix_proof_scalar(inputs[0],inputs[1],inputs[2])==15);
    unsigned checks=0;
    for(unsigned i=0;i<60000;i++){
        unsigned lane=i%36;float *value=&inputs[lane/12][lane/4%3][lane%4],old=*value;
        static const uint32_t special[]={0,0x80000000,0x7fc12345,0x7f800000,0xff800000,1,0x41800000,0x41800001,0x4b189680,0x4b189681};
        uint32_t bits=i%7==0?special[i/7%10]:random_word();memcpy(value,&bits,4);
        unsigned fp=(i%16)<<22,expected=fh_matrix_proof_scalar(inputs[0],inputs[1],inputs[2]);
        for(unsigned repeat=0;repeat<3;repeat++){
            assert(fh_matrix_proof(&cache,inputs[0],inputs[1],inputs[2],fp)==expected);checks++;
        }
        *value=old;
        assert(fh_matrix_proof(&cache,inputs[0],inputs[1],inputs[2],fp)==15);checks++;
    }
    assert(cache.hits>=120000 && cache.misses>10000);
    printf("%u cached/fresh decisions, %u hits, %u misses, %zu bytes; mutation of all 36 input words, collisions, eviction and FP keys\n",checks,cache.hits,cache.misses,sizeof(cache));
}
