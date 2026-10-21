#include "fh_pica_shadow_track.h"
#include <stddef.h>
#include <stdint.h>
static int routes(const FhShadowTrackVertex*v,unsigned nv,const uint16_t*ix,unsigned ni,
    const FhShadowTrackDraw*d,unsigned nd,uint16_t*alternate,unsigned capacity,FhShadowTrackRoutes*out,unsigned moving){
    if(!out || (!v && nv && !moving) || (!ix && ni) || (!d && nd) || nv>65535 || ni>65535 || ni%3)return 0;
    FhShadowTrackRoutes result={0};unsigned cursor=0,alt=0;
    for(unsigned i=0;i<nd;i++){
        if(d[i].first!=cursor || !d[i].count || d[i].count%3 || d[i].count>ni-cursor ||
           !(d[i].flags&8))return 0;
        cursor+=d[i].count;if(d[i].flags&4)alt+=d[i].count;
    }
    if(cursor!=ni)return 0;
    for(unsigned i=0;i<ni;i++)if(ix[i]>=nv)return 0;
    result.extra_indices=alt==ni?0:alt;
    if(alternate){
        if(capacity<result.extra_indices)return 0;
        /* Never destroy the original stream while preparing the alternate. */
        uintptr_t dst=(uintptr_t)alternate,src=(uintptr_t)ix;
        if(dst>UINTPTR_MAX-result.extra_indices*2 || src>UINTPTR_MAX-ni*2)return 0;
        if(result.extra_indices && dst<src+ni*2 && src<dst+result.extra_indices*2)return 0;
    }
    result.route[0].count=ni;result.route[1].count=alt;
    result.route[1].index_offset=result.extra_indices?ni:0;
    unsigned used[2]={0,0};
    for(unsigned group=0;group<nd;group++){
        unsigned is_alt=(d[group].flags&4)!=0;
        for(unsigned j=d[group].first;j<d[group].first+d[group].count;j++){
            const int16_t*p=moving?NULL:v[ix[j]].position;
            if(alternate && result.extra_indices && is_alt)alternate[used[1]]=ix[j];
            for(unsigned route=0;route<1+is_alt;route++){
                if(moving){
                    if(j==d[group].first)for(unsigned k=0;k<3;k++){
                        if(!used[route] || d[group].lo[k]<result.route[route].lo[k])result.route[route].lo[k]=d[group].lo[k];
                        if(!used[route] || d[group].hi[k]>result.route[route].hi[k])result.route[route].hi[k]=d[group].hi[k];
                    }
                }
                else if(!used[route])for(unsigned k=0;k<3;k++)result.route[route].lo[k]=result.route[route].hi[k]=p[k];
                else for(unsigned k=0;k<3;k++){
                    if(p[k]<result.route[route].lo[k])result.route[route].lo[k]=p[k];
                    if(p[k]>result.route[route].hi[k])result.route[route].hi[k]=p[k];
                }
                used[route]++;
            }
        }
    }
    *out=result;return 1;
}
int fh_pica_shadow_track_routes(const FhShadowTrackVertex*v,unsigned nv,const uint16_t*ix,unsigned ni,
    const FhShadowTrackDraw*d,unsigned nd,uint16_t*alternate,unsigned capacity,FhShadowTrackRoutes*out){
    return routes(v,nv,ix,ni,d,nd,alternate,capacity,out,0);
}
int fh_pica_shadow_track_live_routes(const uint16_t*ix,unsigned ni,const FhShadowTrackDraw*d,unsigned nd,
    uint16_t*alternate,unsigned capacity,FhShadowTrackRoutes*out){
    return routes(NULL,ni,ix,ni,d,nd,alternate,capacity,out,1);
}
