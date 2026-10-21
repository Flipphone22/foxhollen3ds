#include "fh_pica_shadow_track.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void half(unsigned char*p,unsigned n){p[0]=(unsigned char)(n>>8);p[1]=(unsigned char)n;}
static void word(unsigned char*p,unsigned n){half(p,n>>16);half(p+2,n);}
int main(void){
    unsigned char blob[256]={0};unsigned vp=164,tp=182,gp=190;
    word(blob+0x58,vp);word(blob+0x4c,tp);word(blob+0x50,gp);
    half(blob+0x90,3);half(blob+0x98,1);half(blob+0x9a,1);half(blob+0x8e,7);
    half(blob+vp+2,(unsigned)-9);half(blob+vp+6+2,1);half(blob+vp+6+4,800);half(blob+vp+12,800);half(blob+vp+12+2,3);
    half(blob+tp+2,1);half(blob+tp+4,2);half(blob+tp+6,65535);
    half(blob+gp+4,100);half(blob+gp+6,(unsigned)-100);half(blob+gp+8,100);half(blob+gp+12,100);
    word(blob+gp+16,4);half(blob+gp+20,1);
    unsigned measured=0;
    assert(fh_pica_shadow_track_raw_scratch(blob,sizeof blob,&measured));
    assert(measured==20+8+2*sizeof(FhShadowTrackGroup));
    FhShadowTrackIndexedSize size={0};int16_t offset=123;
    assert(fh_pica_shadow_track_raw_compile(blob,sizeof blob,NULL,0,NULL,0,NULL,0,&size,&offset));
    assert(size.base.corners==3 && size.vertices==3 && offset==7 && size.scratch_bytes<=1536*1024);
    FhShadowTrackVertex v[3];uint16_t ix[3];FhShadowTrackDraw d[1];
    assert(fh_pica_shadow_track_raw_compile(blob,sizeof blob,v,3,ix,3,d,1,&size,&offset));
    assert(v[ix[0]].position[1]==5 && d[0].flags==10);
    for(unsigned n=0;n<gp+22;n++)assert(!fh_pica_shadow_track_raw_compile(blob,n,NULL,0,NULL,0,NULL,0,&size,&offset));
    for(unsigned field=0;field<3;field++)for(unsigned n=0;n<1024;n++){
        unsigned char invalid[256];memcpy(invalid,blob,sizeof blob);
        unsigned at=field==0?0x58:field==1?0x4c:0x50;
        word(invalid+at,n&1?0xffffffffu-n:n+256);
        FhShadowTrackIndexedSize before=size;int16_t before_offset=offset;
        unsigned before_measured=measured;
        assert(!fh_pica_shadow_track_raw_scratch(invalid,sizeof invalid,&measured));
        assert(measured==before_measured);
        FhShadowTrackVertex old[3];memcpy(old,v,sizeof v);
        assert(!fh_pica_shadow_track_raw_compile(invalid,sizeof invalid,v,3,ix,3,d,1,&size,&offset));
        assert(!memcmp(&size,&before,sizeof size) && offset==before_offset && !memcmp(v,old,sizeof v));
    }
    assert(!fh_pica_shadow_track_raw_compile(blob,sizeof blob,v,2,ix,3,d,1,&size,&offset));
    assert(!fh_pica_shadow_track_raw_compile(blob,sizeof blob,v,3,ix,2,d,1,&size,&offset));
    assert(!fh_pica_shadow_track_raw_compile(blob,sizeof blob,v,3,ix,3,d,0,&size,&offset));
    puts("PASS raw collision decode; 212 truncations,3072 invalid spans,atomic capacity rejection");
}
