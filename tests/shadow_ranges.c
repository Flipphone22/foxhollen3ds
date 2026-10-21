#include "fh_pica_shadow_ranges.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned rng=17;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
static unsigned checks;
static void run(unsigned count,unsigned indexed){
    FhShadowPoint*p=calloc(count,sizeof(*p));uint16_t*ix=malloc(count*2);
    unsigned nodes=fh_pica_shadow_range_nodes(count);
    FhShadowRangeNode*tree=malloc(nodes*sizeof(*tree)),*copy=malloc(nodes*sizeof(*tree));
    assert(p&&ix&&tree&&copy);
    for(unsigned i=0;i<count;i++){
        ix[i]=(uint16_t)(count-i-1);
        for(unsigned k=0;k<3;k++){
            int lo=(int)(next()%800)-400,hi=lo+(int)(next()%300)-50;
            p[i].lo[k]=(int16_t)lo;p[i].hi[k]=(int16_t)hi;
        }
    }
    assert(fh_pica_shadow_range_compile(p,count,indexed?ix:NULL,count,tree,nodes));
    memcpy(copy,tree,nodes*sizeof(*tree));
    for(unsigned j=0;j<100;j++){
        int16_t y=(int16_t)next();float lo[3],hi[3];
        for(unsigned k=0;k<3;k++){
            lo[k]=(int)(next()%1200)-600+(k==1?y:0);
            hi[k]=lo[k]+next()%700;
        }
        FhShadowRange range=fh_pica_shadow_range(tree,lo,hi,y);
        assert(range.first<=count && range.count<=count-range.first && range.visited<=2*nodes);
        for(unsigned i=0;i<count;i++){
            const FhShadowPoint*v=p+(indexed?ix[i]:i);unsigned matches=1;
            for(unsigned k=0;k<3;k++)matches&=lo[k]<=v->hi[k]+(k==1?y:0) && hi[k]>=v->lo[k]+(k==1?y:0);
            if(matches)assert(i>=range.first && i<range.first+range.count);
            checks++;
        }
        assert(!memcmp(copy,tree,nodes*sizeof(*tree)));
    }
    assert(!fh_pica_shadow_range_compile(p,count,indexed?ix:NULL,count,tree,nodes-1));
    assert(!memcmp(copy,tree,nodes*sizeof(*tree)));
    ix[count/2]=(uint16_t)count;
    assert(!fh_pica_shadow_range_compile(p,count,ix,count,tree,nodes));
    assert(!memcmp(copy,tree,nodes*sizeof(*tree)));
    assert(!fh_pica_shadow_range_compile(p,count,NULL,count,(FhShadowRangeNode*)p,nodes));
    free(copy);free(tree);free(ix);free(p);
}
int main(void){
    for(unsigned n=1;n<=2200;n+=n/7+1){run(n,0);run(n,1);}
    run(65535,0);run(65535,1);
    FhShadowPoint p[128]={0};FhShadowRangeNode tree[15];
    for(unsigned i=0;i<128;i++)for(unsigned k=0;k<3;k++)p[i].lo[k]=p[i].hi[k]=(int16_t)(i*80);
    assert(fh_pica_shadow_range_compile(p,128,NULL,128,tree,15));
    for(unsigned i=0;i<128;i++)for(unsigned j=0;j<3;j++){
        int16_t y=(int16_t[]){0,32767,-32768}[j];
        float q[3]={(float)(i*80),(float)(i*80)+y,(float)(i*80)};
        FhShadowRange range=fh_pica_shadow_range(tree,q,q,y);
        assert(range.first==(i/16)*16 && range.count==16);
    }
    float lo[3]={-1,-1,-1},hi[3]={-1,-1,-1};
    assert(!fh_pica_shadow_range(tree,lo,hi,0).count);
    assert(!fh_pica_shadow_range_nodes(0)&&!fh_pica_shadow_range_nodes(65536));
    assert(fh_pica_shadow_range_compile(NULL,0,NULL,0,NULL,0));
    assert(!fh_pica_shadow_range(NULL,lo,hi,0).count);
    printf("PASS %u point-in-query inclusion checks; direct/reversed index order, signed offsets, exact boundary leaves, atomic capacity/index/alias failure\n",checks);
}
