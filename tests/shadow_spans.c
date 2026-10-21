#include "fh_pica_shadow_spans.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned rng=123,checks;
static unsigned next(void){rng=rng*1664525u+1013904223u;return rng;}
static void run(unsigned count,unsigned indexed){
    FhShadowPoint*p=calloc(count,sizeof(*p));uint16_t*ix=malloc(count*2);
    unsigned nodes=fh_pica_shadow_range_nodes(count);
    FhShadowRangeNode*tree=malloc(nodes*sizeof(*tree));
    FhShadowSpansCache cache={0};assert(p&&ix&&tree);
    for(unsigned i=0;i<count;i++){
        ix[i]=(uint16_t)(count-i-1);
        for(unsigned k=0;k<3;k++){
            int x=(int)((i/256)%7)*200+(int)(next()%60)-30;
            p[i].lo[k]=(int16_t)x;p[i].hi[k]=(int16_t)(x+next()%80);
        }
    }
    assert(fh_pica_shadow_range_compile(p,count,indexed?ix:NULL,count,tree,nodes));
    for(unsigned j=0;j<100;j++){
        int16_t y=(int16_t)next();float lo[3],hi[3];
        for(unsigned k=0;k<3;k++){
            lo[k]=(int)(next()%1600)-200+(k==1?y:0);hi[k]=lo[k]+next()%1200;
        }
        FhShadowRange broad=fh_pica_shadow_range(tree,lo,hi,y);
        FhShadowSpans s=fh_pica_shadow_spans_cached(&cache,tree,lo,hi,y);
        assert(s.count<=4 && s.points<=broad.count);
        assert(s.visited<=broad.visited+FH_SHADOW_SPAN_NODE_BUDGET);
        unsigned total=0,end=0;
        for(unsigned k=0;k<s.count;k++){
            assert(s.span[k].count && s.span[k].first>=end);
            end=(unsigned)s.span[k].first+s.span[k].count;
            assert(end<=count);total+=s.span[k].count;
        }
        assert(total==s.points);
        FhShadowSpans hit=fh_pica_shadow_spans_cached(&cache,tree,lo,hi,y);
        assert(!hit.visited && hit.count==s.count && hit.points==s.points && !memcmp(hit.span,s.span,sizeof(s.span)));
        for(unsigned i=0;i<count;i++){
            const FhShadowPoint*v=p+(indexed?ix[i]:i);unsigned matches=1,selected=0;
            for(unsigned k=0;k<3;k++)matches&=lo[k]<=v->hi[k]+(k==1?y:0)&&hi[k]>=v->lo[k]+(k==1?y:0);
            for(unsigned k=0;k<s.count;k++)selected+=i>=s.span[k].first && i<(unsigned)s.span[k].first+s.span[k].count;
            assert(selected<=1 && (!matches || selected));checks++;
        }
    }
    free(tree);free(ix);free(p);
}
int main(void){
    for(unsigned n=1;n<2500;n+=n/5+1){run(n,0);run(n,1);}
    run(65535,0);run(65535,1);
    FhShadowPoint p[2048]={0};FhShadowRangeNode tree[255];
    for(unsigned i=0;i<2048;i++)for(unsigned k=0;k<3;k++)
        p[i].lo[k]=p[i].hi[k]=(i>=256 && i<1792)?100:0;
    assert(fh_pica_shadow_range_compile(p,2048,NULL,2048,tree,255));
    float zero[3]={0};FhShadowSpans s=fh_pica_shadow_spans(tree,zero,zero,0);
    assert(s.points==512 && s.count==2 && s.span[0].first==0 && s.span[1].first==1792);
    assert(!fh_pica_shadow_spans(NULL,zero,zero,0).count);
    printf("PASS %u ordered inclusion checks, direct/indexed routes, offset extremes, exact cache reuse, at most four spans and 256 extra nodes; gap fixture removes 1536/2048 points\n",checks);
}
