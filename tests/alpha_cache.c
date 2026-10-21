#include "fh_alpha_cache.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    const unsigned refs[]={0,1,64,127,128,254,255};unsigned checks=0;
    for(unsigned a=0;a<8;a++)for(unsigned b=0;b<8;b++)for(unsigned op=0;op<4;op++)
    for(unsigned i=0;i<7;i++)for(unsigned j=0;j<7;j++){
        unsigned packed=a|(refs[i]<<3)|(op<<11)|(b<<13)|(refs[j]<<16);
        unsigned scalar_func=0,scalar_ref=0;int scalar=mp_alpha_reduce(packed,&scalar_func,&scalar_ref);
        for(unsigned repeat=0;repeat<3;repeat++){
            unsigned function=0,reference=0;int cached=fh_alpha_reduce_cached(packed,&function,&reference);
            assert(cached==scalar);
            if(cached){
                assert(function==scalar_func && reference==scalar_ref);
                for(unsigned alpha=0;alpha<256;alpha++)assert(mp_alpha_pass(packed,alpha)==mp_alpha_compare(alpha,function,reference));
            }
            /* Same hash slot, different complete predicate: force replacement. */
            if(repeat==1){unsigned other=packed^(1u<<3)^(1u<<11),f,r;fh_alpha_reduce_cached(other,&f,&r);}
            checks++;
        }
    }
    printf("PASS: %u cached alpha predicates; exact 8-bit truth tables and collision replacement\n",checks);return 0;
}
