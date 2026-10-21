#include "fh_pica_shadow_track.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    FhShadowTrackVertex v[6]={{{0,-10,0,1},{0,1,0}},{{4,20,0,1},{0,1,0}},{{0,5,4,1},{0,1,0}},
        {{8,50,-3,1},{0,1,0}},{{12,70,0,1},{0,1,0}},{{8,30,4,1},{0,1,0}}};
    const uint16_t ix[9]={0,1,2,3,4,5,2,1,4};
    FhShadowTrackDraw d[3]={{.first=0,.count=3,.flags=10},{.first=3,.count=3,.flags=28},{.first=6,.count=3,.flags=10}};
    uint16_t alt[9];FhShadowTrackRoutes routes;
    for(unsigned mask=0;mask<8;mask++){
        for(unsigned i=0;i<3;i++)d[i].flags=(mask&(1<<i))?28:10;
        assert(fh_pica_shadow_track_routes(v,6,ix,9,d,3,NULL,0,&routes));
        unsigned count=0;uint16_t expected[9];
        for(unsigned i=0;i<3;i++)if(mask&(1<<i))for(unsigned j=0;j<3;j++)expected[count++]=ix[i*3+j];
        assert(routes.route[0].count==9 && routes.route[1].count==count);
        assert(routes.extra_indices==(count==9?0:count));
        assert(routes.route[0].lo[0]==0 && routes.route[0].lo[1]==-10 && routes.route[0].lo[2]==-3);
        assert(routes.route[0].hi[0]==12 && routes.route[0].hi[1]==70 && routes.route[0].hi[2]==4);
        memset(alt,0x5a,sizeof alt);FhShadowTrackRoutes old=routes;
        if(routes.extra_indices){
            assert(!fh_pica_shadow_track_routes(v,6,ix,9,d,3,alt,routes.extra_indices-1,&routes));
            assert(alt[0]==0x5a5a && !memcmp(&old,&routes,sizeof old));
            assert(!fh_pica_shadow_track_routes(v,6,ix,9,d,3,(uint16_t*)ix,9,&routes));
        }
        assert(fh_pica_shadow_track_routes(v,6,ix,9,d,3,alt,9,&routes));
        if(routes.extra_indices)assert(!memcmp(alt,expected,count*2));else assert(alt[0]==0x5a5a);
        d[1].first=2;assert(!fh_pica_shadow_track_routes(v,6,ix,9,d,3,alt,9,&routes));d[1].first=3;
        assert(!fh_pica_shadow_track_routes(v,4,ix,9,d,3,alt,9,&routes));
    }
    assert(fh_pica_shadow_track_routes(NULL,0,NULL,0,NULL,0,NULL,0,&routes));
    assert(!routes.extra_indices && !routes.route[0].count && !routes.route[1].count);
    puts("PASS source surface routes: ordered subsets,bounds,full alias,empty,atomic rejection,no overlap writes");
}
