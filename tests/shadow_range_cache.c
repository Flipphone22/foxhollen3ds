#include "fh_pica_shadow_ranges.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned checks;
static FhShadowRange check(FhShadowRangeCache*c,const FhShadowRangeNode*n,float x,float r,int16_t y){
    float lo[3]={x,-r+y,-10},hi[3]={x,r+y,10};
    FhShadowRange a=fh_pica_shadow_range(n,lo,hi,y),b=fh_pica_shadow_range_cached(c,n,lo,hi,y);
    assert(a.first==b.first&&a.count==b.count);checks++;return b;
}
int main(void){
    FhShadowPoint p[128]={0},reverse[128]={0};FhShadowRangeNode tree[15],other[15];
    for(unsigned i=0;i<128;i++){
        p[i].lo[0]=p[i].hi[0]=(int16_t)(i*80);reverse[127-i]=p[i];
    }
    assert(fh_pica_shadow_range_compile(p,128,NULL,128,tree,15));
    assert(fh_pica_shadow_range_compile(reverse,128,NULL,128,other,15));
    FhShadowRangeCache cache={0};
    /* Actual FNV collision, not corrupted cache: different spatial results. */
    FhShadowRange a=check(&cache,tree,160,342,0);assert(a.first==0&&a.visited);
    a=check(&cache,tree,1280,1,0);assert(a.first==16&&a.visited);
    assert(cache.entry[0].hash==cache.entry[1].hash);
    assert(!check(&cache,tree,160,342,0).visited);
    a=check(&cache,other,160,342,0);assert(a.first==112&&a.visited);
    assert(!check(&cache,other,160,342,0).visited);
    for(unsigned epoch=0;epoch<100;epoch++){
        memset(&cache,0,sizeof(cache));
        for(unsigned k=0;k<8;k++)assert(check(&cache,tree,k*1280.f,1,32767).visited);
        for(unsigned repeat=0;repeat<100;repeat++)for(unsigned k=0;k<8;k++)
            assert(!check(&cache,tree,k*1280.f,1,32767).visited);
        /* Misses, evictions, empty spans, alternate tree and signed offsets. */
        for(unsigned k=0;k<300;k++){
            float x=(float)((k*173u)%12000)-200;
            int16_t y=(int16_t[]){0,-32768,32767}[k%3];
            check(&cache,k&1?tree:other,x,(float)(k%800+1),y);
        }
        /* Reusing the same tree address for a NEW owner requires zeroing cache. */
        memcpy(tree,other,sizeof(tree));memset(&cache,0,sizeof(cache));
        assert(check(&cache,tree,160,342,0).first==112);
        assert(fh_pica_shadow_range_compile(p,128,NULL,128,tree,15));memset(&cache,0,sizeof(cache));
        assert(check(&cache,tree,160,342,0).first==0);
    }
    printf("PASS %u exact cached/uncached queries, real hash collision, eight live keys, eviction, signed offsets, route identity and owner-address reuse\n",checks);
}
