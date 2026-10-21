#include <assert.h>
#include <stdio.h>
#include "../native-3ds/source/fragment.c"
static unsigned rng=0x544556;
static unsigned random_word(void){rng^=rng<<13;rng^=rng>>17;rng^=rng<<5;return rng;}
int main(void) {
    unsigned checks=0;
    for(unsigned iteration=0;iteration<1024;iteration++) {
        float values[31];
        for(unsigned i=0;i<28;i++){unsigned special[]={0,0x80000000u,0x7f800000u,0xff800000u,0x7fc01234u,0xffc01234u};unsigned bits=iteration<6?special[(i+iteration)%6]:random_word();memcpy(values+i,&bits,4);}
        values[28]=1;values[29]=.5f;values[30]=0;
        for(unsigned c=0;c<4;c++)for(unsigned arg=0;arg<256;arg++) {
            float expected=argument(arg,c,(float(*)[4])values,values+16,values+20,values[24+c]);
            unsigned slot=argument_slot(arg,c);assert(slot<31);
            assert(!memcmp(&expected,values+slot,4));checks++;
        }
        FhDrawState state={0};FhFragmentProgram program;state.stages=16;
        for(unsigned stage=0;stage<16;stage++)for(unsigned k=0;k<4;k++) {
            state.tev[stage].color[k]=random_word()%16;state.tev[stage].alpha[k]=random_word()%8;
        }
        fh_fragment_prepare(&program,&state);
        for(unsigned stage=0;stage<16;stage++) {
            unsigned sampled=0;
            for(unsigned k=0;k<4;k++)sampled|=state.tev[stage].color[k]==8 || state.tev[stage].color[k]==9 || state.tev[stage].alpha[k]==4;
            assert(!!(program.sample_mask&(1u<<stage))==!!sampled);checks++;
        }
        for(unsigned stage=0;stage<16;stage++)for(unsigned c=0;c<4;c++)for(unsigned k=0;k<4;k++) {
            unsigned arg=(c==3?state.tev[stage].alpha:state.tev[stage].color)[k];
            float expected=argument(arg,c,(float(*)[4])values,values+16,values+20,values[24+c]);
            assert(!memcmp(&expected,values+program.arguments[stage][c][k],4));checks++;
        }
    }
    printf("PASS %u TEV input selections including register components, constants, signed zeros and exceptional floats\n",checks);
}
