#include "fh_pica_shadow_track.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    int16_t p[4][3]={{0,-9,0},{0,1,800},{800,3,0},{800,3,800}};
    uint16_t ix[2][4]={{0,1,2,65535},{2,1,3,65535}};
    FhShadowTrackGroup g[2]={{0,0,100,-100,100,0,100,0,4},{.first=2}};
    FhShadowTrackSource s={p,ix,g,4,2,1,7};
    FhShadowTrackVertex v[6];FhShadowTrackDraw d[1];uint16_t indices[6];
    FhShadowTrackSize plain;FhShadowTrackIndexedSize size;
    for(unsigned cycle=0;cycle<1000;cycle++){
        s.y_offset=(int16_t)(cycle%31-15);
        assert(fh_pica_shadow_track_compile(&s,NULL,0,NULL,0,&plain));assert(plain.corners==6);
        assert(fh_pica_shadow_track_indexed_compile(&s,NULL,0,NULL,0,NULL,0,&size));
        memset(v,0x5a,sizeof(v));memset(d,0x5a,sizeof(d));memset(indices,0x5a,sizeof(indices));
        unsigned char before[sizeof(v)+sizeof(d)+sizeof(indices)];
        memcpy(before,v,sizeof(v));memcpy(before+sizeof(v),d,sizeof(d));memcpy(before+sizeof(v)+sizeof(d),indices,sizeof(indices));
        FhShadowTrackIndexedSize old=size;
        assert(!fh_pica_shadow_track_indexed_compile(&s,v,size.vertices-1,indices,6,d,1,&size));
        assert(!memcmp(&size,&old,sizeof(size)));assert(!memcmp(v,before,sizeof(v)));
        assert(!memcmp(d,before+sizeof(v),sizeof(d)));assert(!memcmp(indices,before+sizeof(v)+sizeof(d),sizeof(indices)));
        assert(!fh_pica_shadow_track_indexed_compile(&s,v,6,indices,5,d,1,&size));
        assert(!fh_pica_shadow_track_indexed_compile(&s,v,6,indices,6,d,0,&size));
        ix[1][2]=4;assert(!fh_pica_shadow_track_indexed_compile(&s,v,6,indices,6,d,1,&size));ix[1][2]=3;
        g[1].first=1;assert(!fh_pica_shadow_track_compile(&s,v,6,d,1,&plain));g[1].first=2;
        assert(fh_pica_shadow_track_indexed_compile(&s,v,6,indices,6,d,1,&size));
        assert(v[indices[0]].position[1]==-2+s.y_offset);
        assert(d[0].count==6 && d[0].flags==10 && size.scratch_bytes<=1024*1024);
        g[0].flags=0;assert(fh_pica_shadow_track_indexed_compile(&s,v,6,indices,6,d,1,&size));
        assert(!size.base.corners && !size.vertices);g[0].flags=4;
    }
    puts("PASS1000 compiler lifecycles/capacity atomicity/index/group/negative-coordinate/empty cases");
}
