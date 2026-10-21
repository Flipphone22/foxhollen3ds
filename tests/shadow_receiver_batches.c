#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "fh_pica_shadow_receivers.h"
int main(void){
    unsigned char raw[8*80]={0};
    FhSceneAsset a={.gpu=raw,.gpu_bytes=sizeof(raw),.block=1,.direct=1,.compact=1,.groups=8};
    FhPicaWorldPlan*p=calloc(1,sizeof(*p)+8*sizeof(*p->draw));p->count=8;
    for(unsigned i=0;i<8;i++){
        int16_t*position=(int16_t*)(raw+i*80);uint16_t*index=(uint16_t*)(raw+i*80+64);
        for(unsigned v=0;v<3;v++){position[v*10]=i*4+v;position[v*10+1]=v;position[v*10+2]=i;position[v*10+3]=1;index[v]=v;}
        p->draw[i]=(FhPicaWorldDraw){.vertex=position,.index=index,.vertices=3,.indices=3,.format=7,.cull=1};
    }
    for(unsigned cycle=0;cycle<1000;cycle++){
        unsigned count=0,bytes=fh_pica_shadow_batches_compile(&a,p,NULL,NULL,0,&count);assert(count==2 && bytes);
        unsigned cpu=sizeof(*p)+count*(sizeof(*p->draw)+sizeof(FhPicaShadowReceiver));
        FhPicaWorldPlan*out=malloc(cpu);void*gpu=malloc(bytes);memset(out,0xa5,cpu);
        unsigned capacity=count-1;assert(!fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&capacity));
        for(unsigned k=0;k<cpu;k++)assert(((unsigned char*)out)[k]==0xa5);
        assert(!fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes-1,&count));
        memset(out,0,sizeof(*out));assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
        assert(out->count==2);FhPicaShadowReceiver*r=(FhPicaShadowReceiver*)(out->draw+count);
        for(unsigned b=0;b<2;b++){
            assert(r[b].draw==out->draw+b && out->draw[b].indices==12);
            for(unsigned j=0;j<12;j++){
                const int16_t*v=(const int16_t*)((const char*)out->draw[b].vertex+out->draw[b].index[j]*8);
                const FhPicaWorldDraw*d=p->draw+b*4+j/3;
                const int16_t*source=(const int16_t*)((const char*)d->vertex+d->index[j%3]*20);
                assert(!memcmp(v,source,8));
            }
        }
        /* Existing mutation boundary rebuilds packed positions and bounds. */
        ((int16_t*)raw)[1]=(int16_t)(cycle%100+8);
        assert(fh_pica_shadow_batches_compile(&a,p,out,gpu,bytes,&count)==bytes);
        assert(r[0].bounds.hi[1]==(float)(cycle%100+8));
        free(gpu);free(out);
    }
    free(p);puts("PASS 1000 batch lifecycles, mutation refresh, exact position lanes, bounded outputs");
}
