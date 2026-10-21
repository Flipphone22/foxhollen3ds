#include "fh_pica_shadow_ranges.h"
#include <string.h>
_Static_assert(sizeof(FhShadowRangeNode)==16,"compact normal-memory range tree");
unsigned fh_pica_shadow_range_nodes(unsigned count){
    if(!count || count>65535)return 0;
    unsigned leaves=1;while(leaves*FH_SHADOW_RANGE_LEAF<count)leaves*=2;
    return leaves*2-1;
}
static void build(const FhShadowPoint*p,const uint16_t*indices,FhShadowRangeNode*n,
    unsigned slot,unsigned first,unsigned count){
    FhShadowRangeNode*b=n+slot;b->first=(uint16_t)first;b->count=(uint16_t)count;
    if(count<=FH_SHADOW_RANGE_LEAF){
        for(unsigned i=0;i<count;i++){
            const FhShadowPoint*v=p+(indices?indices[first+i]:first+i);
            for(unsigned k=0;k<3;k++){
                if(!i || v->lo[k]<b->lo[k])b->lo[k]=v->lo[k];
                if(!i || v->hi[k]>b->hi[k])b->hi[k]=v->hi[k];
            }
        }
    }else{
        unsigned left=slot*2+1,right=left+1,half=count/2;
        build(p,indices,n,left,first,half);build(p,indices,n,right,first+half,count-half);
        for(unsigned k=0;k<3;k++){
            b->lo[k]=n[left].lo[k]<n[right].lo[k]?n[left].lo[k]:n[right].lo[k];
            b->hi[k]=n[left].hi[k]>n[right].hi[k]?n[left].hi[k]:n[right].hi[k];
        }
    }
}
static int overlap(const void*a,unsigned an,const void*b,unsigned bn){
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;
    if(x>UINTPTR_MAX-an || y>UINTPTR_MAX-bn)return 1;
    return an && bn && x<y+bn && y<x+an;
}
int fh_pica_shadow_range_compile(const FhShadowPoint*p,unsigned points,const uint16_t*indices,
    unsigned count,FhShadowRangeNode*out,unsigned capacity){
    if(points>65535 || count>65535 || (!p && points) || (!indices && count>points))return 0;
    unsigned nodes=fh_pica_shadow_range_nodes(count);
    if(count && (!out || capacity<nodes))return 0;
    if(overlap(out,nodes*sizeof(*out),p,points*sizeof(*p)) ||
       (indices && overlap(out,nodes*sizeof(*out),indices,count*sizeof(*indices))))return 0;
    if(indices)for(unsigned i=0;i<count;i++)if(indices[i]>=points)return 0;
    if(count){memset(out,0,nodes*sizeof(*out));build(p,indices,out,0,0,count);}
    return 1;
}
static int edge(const FhShadowRangeNode*n,unsigned slot,const float lo[3],const float hi[3],
    unsigned last,unsigned*found,unsigned*visited){
    const FhShadowRangeNode*b=n+slot;(*visited)++;
    for(unsigned k=0;k<3;k++)if(lo[k]>b->hi[k] || hi[k]<b->lo[k])return 0;
    if(b->count<=FH_SHADOW_RANGE_LEAF){
        *found=last?b->first+b->count:b->first;return 1;
    }
    unsigned left=slot*2+1,right=left+1;
    return edge(n,last?right:left,lo,hi,last,found,visited) ||
           edge(n,last?left:right,lo,hi,last,found,visited);
}
FhShadowRange fh_pica_shadow_range(const FhShadowRangeNode*n,const float qlo[3],const float qhi[3],int16_t y){
    FhShadowRange result={0};if(!n || !n->count)return result;
    float lo[3]={qlo[0],qlo[1]-y,qlo[2]},hi[3]={qhi[0],qhi[1]-y,qhi[2]};
    unsigned end;
    if(edge(n,0,lo,hi,0,&result.first,&result.visited) &&
       edge(n,0,lo,hi,1,&end,&result.visited))result.count=end-result.first;
    return result;
}
FhShadowRange fh_pica_shadow_range_cached(FhShadowRangeCache*cache,const FhShadowRangeNode*n,
    const float lo[3],const float hi[3],int16_t y){
    if(!n || !n->count)return (FhShadowRange){0};
    uint32_t key[6],hash=2166136261u;
    memcpy(key,lo,12);memcpy(key+3,hi,12);
    for(unsigned i=0;i<6;i++)hash=(hash^key[i])*16777619u;
    hash=(hash^(uint16_t)y)*16777619u;
    for(unsigned i=0;i<FH_SHADOW_RANGE_CACHE_SLOTS;i++){
        const FhShadowRangeCacheEntry*e=cache->entry+i;
        if(e->tree==n && e->hash==hash && e->y==y && !memcmp(e->key,key,sizeof(key)))
            return (FhShadowRange){e->first,e->count,0};
    }
    FhShadowRange result=fh_pica_shadow_range(n,lo,hi,y);
    FhShadowRangeCacheEntry*e=cache->entry+cache->next;
    memcpy(e->key,key,sizeof(key));e->tree=n;e->hash=hash;e->y=y;
    e->first=(uint16_t)result.first;e->count=(uint16_t)result.count;
    cache->next=(cache->next+1)%FH_SHADOW_RANGE_CACHE_SLOTS;
    return result;
}
