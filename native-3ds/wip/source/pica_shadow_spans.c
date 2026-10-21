#include "fh_pica_shadow_spans.h"
#include <string.h>
typedef struct {float lo[3],hi[3];FhShadowSpan gaps[FH_SHADOW_SPAN_CAP-1],pending;
    unsigned count,first,end,visited;} Search;
static void select_gap(Search*s,unsigned first,unsigned end){
    if(first<s->first)first=s->first;
    if(end>s->end)end=s->end;
    if(end<=first || end-first<FH_SHADOW_SPAN_MIN_GAP)return;
    unsigned slot=s->count;
    if(slot<FH_SHADOW_SPAN_CAP-1)s->count++;
    else{
        slot=0;
        for(unsigned i=1;i<s->count;i++)if(s->gaps[i].count<s->gaps[slot].count)slot=i;
        if(end-first<=s->gaps[slot].count)return;
    }
    s->gaps[slot]=(FhShadowSpan){(uint16_t)first,(uint16_t)(end-first)};
}
static void gap(Search*s,unsigned first,unsigned end){
    if(s->pending.count && first==(unsigned)s->pending.first+s->pending.count){
        s->pending.count=(uint16_t)(end-s->pending.first);return;
    }
    if(s->pending.count)select_gap(s,s->pending.first,(unsigned)s->pending.first+s->pending.count);
    s->pending=(FhShadowSpan){(uint16_t)first,(uint16_t)(end-first)};
}
static void search(const FhShadowRangeNode*n,unsigned at,Search*s){
    if(s->visited==FH_SHADOW_SPAN_NODE_BUDGET)return;
    const FhShadowRangeNode*b=n+at;s->visited++;
    unsigned end=(unsigned)b->first+b->count;
    if(!b->count || end<=s->first || b->first>=s->end)return;
    unsigned inside=1;
    for(unsigned k=0;k<3;k++){
        if(s->lo[k]>b->hi[k] || s->hi[k]<b->lo[k]){gap(s,b->first,end);return;}
        inside&=s->lo[k]<=b->lo[k] && s->hi[k]>=b->hi[k];
    }
    if(inside || b->count<=FH_SHADOW_RANGE_LEAF)return;
    search(n,at*2+1,s);search(n,at*2+2,s);
}
FhShadowSpans fh_pica_shadow_spans(const FhShadowRangeNode*n,const float lo[3],const float hi[3],int16_t y){
    FhShadowSpans result={0};
    FhShadowRange broad=fh_pica_shadow_range(n,lo,hi,y);
    result.visited=broad.visited;
    if(!broad.count)return result;
    Search s={.lo={lo[0],lo[1]-y,lo[2]},.hi={hi[0],hi[1]-y,hi[2]},
        .first=broad.first,.end=broad.first+broad.count};
    search(n,0,&s);
    if(s.pending.count)select_gap(&s,s.pending.first,(unsigned)s.pending.first+s.pending.count);
    result.visited+=s.visited;
    for(unsigned i=1;i<s.count;i++){
        FhShadowSpan g=s.gaps[i];unsigned j=i;
        while(j && s.gaps[j-1].first>g.first){s.gaps[j]=s.gaps[j-1];j--;}
        s.gaps[j]=g;
    }
    unsigned first=s.first;
    for(unsigned i=0;i<=s.count;i++){
        unsigned end=i<s.count?s.gaps[i].first:s.end;
        if(end>first){
            result.span[result.count++]=(FhShadowSpan){(uint16_t)first,(uint16_t)(end-first)};
            result.points+=end-first;
        }
        if(i<s.count)first=(unsigned)s.gaps[i].first+s.gaps[i].count;
    }
    return result;
}
FhShadowSpans fh_pica_shadow_spans_cached(FhShadowSpansCache*c,const FhShadowRangeNode*n,
    const float lo[3],const float hi[3],int16_t y){
    if(!n || !n->count)return (FhShadowSpans){0};
    uint32_t key[6],hash=2166136261u;
    memcpy(key,lo,12);memcpy(key+3,hi,12);
    for(unsigned i=0;i<6;i++)hash=(hash^key[i])*16777619u;
    hash=(hash^(uint16_t)y)*16777619u;
    for(unsigned i=0;i<FH_SHADOW_RANGE_CACHE_SLOTS;i++){
        const FhShadowSpansEntry*e=c->entry+i;
        if(e->tree==n && e->hash==hash && e->y==y && !memcmp(e->key,key,sizeof(key))){
            FhShadowSpans result=e->result;result.visited=0;return result;
        }
    }
    FhShadowSpans result=fh_pica_shadow_spans(n,lo,hi,y);
    FhShadowSpansEntry*e=c->entry+c->next;
    memcpy(e->key,key,sizeof(key));e->hash=hash;e->tree=n;e->y=y;e->result=result;
    c->next=(c->next+1)%FH_SHADOW_RANGE_CACHE_SLOTS;return result;
}
