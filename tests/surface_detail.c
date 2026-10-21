#include "fh_surface_detail.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    unsigned checks=0;
    for(unsigned joints=0;joints<4;joints++)for(unsigned tail=0;tail<=10;tail++)
    for(unsigned refs=0;refs<2;refs++)for(unsigned aux=0;aux<2;aux++)for(unsigned need=1;need<=8;need++){
        unsigned indexed[8]={0},next=0;
        if(joints>1){
            /* Independent descriptor replay of modelSetVtxDcrs: forward
             * environment indices, then unused projected slots backwards. */
            if(refs){if(aux){indexed[next++]=1;indexed[next++]=1;}indexed[next++]=1;}
            for(unsigned i=0;i<tail && i<8;i++)indexed[7-i]=1;
        }
        int base=fh_surface_uv_base(joints,tail,refs,aux,need),first=-1;
        if(joints<=1 || tail<=8)for(unsigned begin=next;begin+need<=8;begin++){
            unsigned free=1;for(unsigned c=begin;c<begin+need;c++)if(indexed[c])free=0;
            if(free){first=begin;break;}
        }
        assert(base==first);
        if(base>=0)for(unsigned c=base;c<(unsigned)base+need;c++)assert(!indexed[c]);
        checks++;
    }
    assert(fh_surface_uv_base(12,0,1,0,2)==1); /* Prior TEX0 collision. */
    assert(fh_surface_uv_base(12,2,1,1,2)==3);
    assert(fh_surface_uv_base(12,5,1,1,2)==-1);
    printf("surface detail: %u indexed-coordinate layouts checked\n",checks);
}
