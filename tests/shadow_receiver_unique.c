#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fh_pica_shadow_receivers.h"
int deny_receiver_scratch;
int main(void){
    unsigned char raw[8*96]={0};
    FhSceneAsset a={.gpu=raw,.gpu_bytes=sizeof(raw),.block=1,.direct=1,.compact=1,.groups=8};
    FhPicaWorldPlan*p=calloc(1,sizeof(*p)+8*sizeof(*p->draw));p->count=8;
    for(unsigned i=0;i<8;i++){
        int16_t*pos=(int16_t*)(raw+i*96);uint16_t*ix=(uint16_t*)(raw+i*96+64);
        for(unsigned v=0;v<3;v++){pos[v*10]=(i%4)*4+(v==1);pos[v*10+1]=(v==2);pos[v*10+2]=0;pos[v*10+3]=1;}
        /* Cyclic duplicate within each group; distinct storage across groups. */
        const uint16_t indices[6]={0,1,2,1,2,0};memcpy(ix,indices,sizeof(indices));
        p->draw[i]=(FhPicaWorldDraw){.vertex=pos,.index=ix,.vertices=3,.indices=6,.format=7,.cull=1};
    }
    unsigned count=0,bytes=fh_pica_shadow_batches_compile(&a,p,NULL,NULL,0,&count);assert(count==2);
    unsigned cpu=sizeof(*p)+count*(sizeof(*p->draw)+sizeof(FhPicaShadowReceiver));
    FhPicaWorldPlan*out=calloc(1,cpu);unsigned char*gpu=malloc(bytes+32);memset(gpu+bytes,0xab,32);
    for(unsigned cycle=0;cycle<1000;cycle++){
        assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
        assert(out->draw[0].indices==12 && !out->draw[1].indices);
        for(unsigned k=0;k<12;k++)assert(out->draw[0].index[k]==k);
        /* Move one corner by one source unit: it must not be welded away. */
        int16_t*z=(int16_t*)(raw+4*96+4);*z=1;
        assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
        assert(out->draw[0].indices==12 && out->draw[1].indices==3);*z=0;
        /* Opposite winding is a separate oriented receiver. */
        uint16_t*ix=(uint16_t*)(raw+5*96+64);ix[0]=1;ix[1]=0;
        assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
        assert(out->draw[1].indices==3);ix[0]=0;ix[1]=1;
        for(unsigned k=0;k<32;k++)assert(gpu[bytes+k]==0xab);
    }
    deny_receiver_scratch=1;
    assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
    assert(out->draw[0].indices==24 && out->draw[1].indices==24);
    for(unsigned b=0;b<2;b++)for(unsigned g=0;g<4;g++){
        const uint16_t expected[6]={0,1,2,1,2,0};
        for(unsigned k=0;k<6;k++)assert(out->draw[b].index[g*6+k]==expected[k]+g*3);
    }
    deny_receiver_scratch=0;
    assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
    assert(out->draw[0].indices==12 && !out->draw[1].indices);
    free(out);free(gpu);free(p);
    puts("PASS 1000 exact/cyclic/cross-batch/empty-batch/mutation/winding/order/guard cycles; allocation failure retains original geometry");
}
