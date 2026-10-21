#include "fh_pica_shadow_points.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void bounds(const FhShadowPoint*v,const unsigned*order,unsigned count,int y,const FhShadowPointRoute*r){
    for(unsigned i=0;i<count;i++){
        const int16_t*p[3]={v[order[i]].a,v[order[i]].b,v[order[i]].c};
        for(unsigned j=0;j<3;j++)for(unsigned k=0;k<3;k++){
            int q=p[j][k]+(k==1?y:0);unsigned bits=(unsigned)q&65535;int wrapped=bits<32768?(int)bits:(int)bits-65536;
            assert(q>=r->query_lo[k]&&q<=r->query_hi[k]);
            assert(wrapped>=r->visible_lo[k]&&wrapped<=r->visible_hi[k]);
        }
    }
}
int main(void){
    unsigned cases=0;
    for(unsigned ng=1;ng<=8;ng++)for(unsigned mask=0;mask<(1u<<ng);mask++)for(unsigned pose=0;pose<4;pose++){
        FhShadowPoint v[36]={0};FhShadowTrackDraw d[8]={0};uint16_t indices[36];unsigned all[36],alt[36],n=0,na=0;
        int y=(int[]){0,28672,32767,-32768}[pose];
        for(unsigned g=0;g<ng;g++){
            d[g].first=n;d[g].count=g+1;d[g].flags=(mask&(1u<<g))?28:10;
            for(unsigned j=0;j<g+1;j++,n++){
                all[n]=n;if(mask&(1u<<g))alt[na++]=n;
                int16_t p[9]={(int16_t)n,-4096,0,(int16_t)(n+3),4095,2,(int16_t)(n-5),0,9};
                memcpy(v+n,p,sizeof(p));
            }
        }
        FhShadowPointRoutes routes,measured;
        assert(fh_pica_shadow_points_routes(v,n,d,ng,NULL,0,y,&measured));
        assert(fh_pica_shadow_points_routes(v,n,d,ng,indices,36,y,&routes));
        assert(!memcmp(&routes,&measured,sizeof(routes)));
        assert(routes.route[0].count==n&&!routes.route[0].indexed);
        assert(routes.route[1].count==na&&routes.route[1].indexed==(na&&na!=n));
        assert(routes.extra_indices==(na==n?0:na));
        if(routes.extra_indices)for(unsigned i=0;i<na;i++)assert(indices[i]==alt[i]);
        bounds(v,all,n,y,routes.route);bounds(v,alt,na,y,routes.route+1);
        FhShadowPointRoutes before=routes;fh_pica_shadow_points_bounds(v,indices,y,&routes);
        assert(!memcmp(&routes,&before,sizeof(routes)));
        if(routes.extra_indices){
            uint16_t copy[36];memcpy(copy,indices,sizeof(indices));
            assert(!fh_pica_shadow_points_routes(v,n,d,ng,indices,routes.extra_indices-1,y,&routes));
            assert(!memcmp(&routes,&before,sizeof(routes))&&!memcmp(copy,indices,sizeof(indices)));
            assert(!fh_pica_shadow_points_routes(v,n,d,ng,(uint16_t*)v,36,y,&routes));
        }
        for(unsigned i=0;i<n;i++){v[i].a[1]=4095;v[i].b[1]=-4096;v[i].c[1]=-2048;}
        fh_pica_shadow_points_bounds(v,indices,y,&routes);
        bounds(v,all,n,y,routes.route);bounds(v,alt,na,y,routes.route+1);
        cases++;
    }
    FhShadowPointRoutes empty;
    assert(fh_pica_shadow_points_routes(NULL,0,NULL,0,NULL,0,0,&empty));
    assert(!empty.extra_indices&&!empty.route[0].count&&!empty.route[1].count);
    printf("PASS %u ordered surface routes; direct/full/subset indices; separate unwrapped/wrapped bounds; atomic capacity/alias rejection\n",cases);
}
