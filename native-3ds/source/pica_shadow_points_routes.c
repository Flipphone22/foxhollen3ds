#include "fh_pica_shadow_points.h"
#include <string.h>

static void include(FhShadowPointRoute*r,const FhShadowPoint*p,int16_t y,unsigned first){
    const int16_t*pt[3]={p->a,p->b,p->c};
    for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
        int32_t q=(int32_t)pt[j][k]+(k==1?y:0);unsigned bits=(unsigned)q&65535;
        int16_t visible=(int16_t)(bits<32768?(int)bits:(int)bits-65536);
        if(first && !j){r->query_lo[k]=r->query_hi[k]=q;r->visible_lo[k]=r->visible_hi[k]=visible;}
        else{
            if(q<r->query_lo[k])r->query_lo[k]=q;
            if(q>r->query_hi[k])r->query_hi[k]=q;
            if(visible<r->visible_lo[k])r->visible_lo[k]=visible;
            if(visible>r->visible_hi[k])r->visible_hi[k]=visible;
        }
    }
}
int fh_pica_shadow_points_routes(const FhShadowPoint*v,unsigned n,const FhShadowTrackDraw*d,unsigned nd,
    uint16_t*alt,unsigned capacity,int16_t y,FhShadowPointRoutes*out){
    if(!out || (!v && n) || (!d && nd) || n>65535)return 0;
    unsigned cursor=0,alternate=0;
    for(unsigned i=0;i<nd;i++){
        if(d[i].first!=cursor || !d[i].count || d[i].count>n-cursor || !(d[i].flags&8))return 0;
        cursor+=d[i].count;if(d[i].flags&4)alternate+=d[i].count;
    }
    if(cursor!=n)return 0;
    FhShadowPointRoutes result={0};result.extra_indices=alternate==n?0:alternate;
    if(alt){
        if(capacity<result.extra_indices)return 0;
        uintptr_t a=(uintptr_t)alt,b=(uintptr_t)v;
        if(a>UINTPTR_MAX-result.extra_indices*2 || b>UINTPTR_MAX-n*sizeof(*v))return 0;
        if(result.extra_indices && a<b+n*sizeof(*v) && b<a+result.extra_indices*2)return 0;
    }
    result.route[0].count=n;result.route[1].count=alternate;result.route[1].indexed=!!result.extra_indices;
    unsigned used=0;
    for(unsigned g=0;g<nd;g++)for(unsigned i=d[g].first;i<d[g].first+d[g].count;i++){
        include(result.route,v+i,y,i==0);
        if(d[g].flags&4){
            include(result.route+1,v+i,y,used==0);
            if(alt && result.extra_indices)alt[used]=(uint16_t)i;
            used++;
        }
    }
    *out=result;return 1;
}
void fh_pica_shadow_points_bounds(const FhShadowPoint*v,const uint16_t*alt,int16_t y,FhShadowPointRoutes*r){
    for(unsigned route=0;route<2;route++){
        FhShadowPointRoute*b=r->route+route;
        if(route && b->count==r->route[0].count && !b->indexed){*b=r->route[0];continue;}
        for(unsigned i=0;i<b->count;i++)include(b,v+(b->indexed?alt[i]:i),y,i==0);
    }
}
