#include "fh_pica_distortion_mesh.h"
#include <math.h>
_Static_assert(sizeof(FhDistortionVertex)==12,"Distortion vertex fetch extent");
_Static_assert(FH_DISTORTION_VERTICES<65536,"Distortion u16 indices");
static unsigned add_radius(double r[128],unsigned n,double x){
    if(n>128)return 129;
    unsigned at=0;while(at<n&&r[at]<x)at++;
    if(at<n&&r[at]==x)return n;
    if(n==128)return 129;
    for(unsigned i=n;i>at;i--)r[i]=r[i-1];
    r[at]=x;return n+1;
}
static int clamp_pixel(int x){return x<0?0:x>255?255:x;}
static int16_t offset_round(double x){
    double lo=floor(x),fraction=x-lo;int value=(int)lo;
    if(fraction>.5||(fraction==.5&&(value&1)))value++;
    return (int16_t)value;
}
static unsigned pixel(unsigned x,unsigned y,unsigned tiled){
    return tiled?(y>>2)*32+(y&3)*2+(x&3)*8+(x>>2)*0x800:(y*256+x)*2;
}
static FhDistortionVertex vertex(const uint8_t *field,unsigned tiled,double x,double y){
    FhDistortionVertex v={{(float)((x+128)/256),(float)((y+128)/256)},{0,0}};
    double px=x+127.5,py=y+127.5;int ix=(int)floor(px),iy=(int)floor(py);
    double fx=px-ix,fy=py-iy;
    unsigned x0=(unsigned)clamp_pixel(ix),x1=(unsigned)clamp_pixel(ix+1);
    unsigned y0=(unsigned)clamp_pixel(iy),y1=(unsigned)clamp_pixel(iy+1);
    for(unsigned c=0;c<2;c++){
        double a=field[pixel(x0,y0,tiled)+c]*(1-fx)+field[pixel(x1,y0,tiled)+c]*fx;
        double b=field[pixel(x0,y1,tiled)+c]*(1-fx)+field[pixel(x1,y1,tiled)+c]*fx;
        v.offset[c]=offset_round((a*(1-fy)+b*fy-128)*256);
    }
    return v;
}
int fh_pica_distortion_mesh(const uint8_t *field,size_t field_bytes,unsigned tiled,
    FhDistortionVertex *v,size_t nv,uint16_t *indices,size_t ni){
    if(!field||field_bytes!=FH_DISTORTION_FIELD_BYTES||tiled>1||!v||!indices||
       nv<FH_DISTORTION_VERTICES||ni<FH_DISTORTION_INDICES)return 0;
    double radii[128];unsigned count=0;
    for(double radius=.125;radius<128;radius*=1.1){
        count=add_radius(radii,count,radius);if(count>128)return 0;
    }
    count=add_radius(radii,count,128);
    for(unsigned i=1;i<=16;i++)count=add_radius(radii,count,i*.125);
    const unsigned extra[]={3,4,6,8,12,16,110,111,112,113,114,192};
    for(unsigned i=0;i<sizeof(extra)/sizeof(*extra);i++){
        count=add_radius(radii,count,extra[i]);if(count>128)return 0;
    }
    if(count!=FH_DISTORTION_RINGS)return 0;
    /* Evaluate directions once per segment, not once per resident vertex. */
    double direction[FH_DISTORTION_SEGMENTS][2];
    const double tau=6.283185307179586476925286766559;
    for(unsigned i=0;i<FH_DISTORTION_SEGMENTS;i++){
        double angle=i*tau/FH_DISTORTION_SEGMENTS;
        direction[i][0]=cos(angle);direction[i][1]=sin(angle);
    }
    v[0]=vertex(field,tiled,0,0);
    for(unsigned ring=0;ring<count;ring++)for(unsigned j=0;j<FH_DISTORTION_SEGMENTS;j++)
        v[1+ring*FH_DISTORTION_SEGMENTS+j]=vertex(field,tiled,radii[ring]*direction[j][0],radii[ring]*direction[j][1]);
    unsigned at=0,n=FH_DISTORTION_SEGMENTS;
    for(unsigned j=0;j<n;j++){indices[at++]=0;indices[at++]=1+j;indices[at++]=1+(j+1)%n;}
    for(unsigned ring=0;ring<count-1;ring++)for(unsigned j=0;j<n;j++){
        unsigned inner=1+ring*n,outer=inner+n,k=(j+1)%n;
        indices[at++]=inner+j;indices[at++]=outer+j;indices[at++]=outer+k;
        indices[at++]=inner+j;indices[at++]=outer+k;indices[at++]=inner+k;
    }
    return 1;
}
