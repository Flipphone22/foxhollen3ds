#include "fh_material_register.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
int fh_material_compile_reference(FhMaterial*,const FhDrawState*);
static uint32_t rng=0x7532419;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
int main(void){
    unsigned accepted=0,rejected=0,mutations=0;
    for(unsigned trial=0;trial<160000;trial++){
        FhDrawState s={0};s.stages=1;s.texgens=next()%9;
        for(unsigned i=0;i<4;i++)for(unsigned c=0;c<4;c++){
            s.swaps[i][c]=c;s.tev_colors[i][c]=(next()%257)/256.f;
        }
        unsigned texture=trial&1,reg=next()%8,alpha=next()%4;
        FhTevStage *t=s.tev;
        *t=(FhTevStage){.color={15,texture?8:15,texture?reg:15,texture?15:reg},
            .alpha={7,texture?4:7,texture?alpha:7,texture?7:alpha},
            .color_clamp=1,.alpha_clamp=1,.map=next()%8,.coord=next()%8,
            .tex_swap=next()%4,.color_dest=next()%4,.alpha_dest=next()%4};
        if(trial%5==0){
            static const float exceptional[]={-0.f,-1.f,2.f,NAN,INFINITY,-INFINITY,1e-38f};
            s.tev_colors[reg/2][reg&1?3:0]=exceptional[(trial/5)%7];
        }
        if(trial%3==0){
            /* Mutate every stage field, including its structural selectors.
             * Accepted mutations still must match the original full compiler. */
            unsigned *words=(unsigned*)t;unsigned slot=next()%(sizeof(*t)/4);
            words[slot]=next()%32;mutations++;
        }
        FhMaterial fast,reference,actual;
        if(!fh_material_register(&fast,&s)){rejected++;continue;}
        accepted++;
        assert(fh_material_compile_reference(&reference,&s));
        if(memcmp(&fast,&reference,sizeof(fast))){
            fprintf(stderr,"Mismatch trial %u texture %u\n",trial,texture);return 1;
        }
        assert(fh_material_compile(&actual,&s));
        assert(!memcmp(&actual,&reference,sizeof(actual)));
    }
    assert(accepted>100000 && rejected>10000);
    printf("%u exact complete material plans; %u declined states; %u structural mutations\n",accepted,rejected,mutations);
}
