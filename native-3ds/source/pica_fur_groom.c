#include "fh_pica_fur_groom.h"
#include <math.h>
#include <string.h>
static unsigned channel(const FhTexture*t,unsigned x,unsigned y,unsigned green){
    const unsigned char*p=t->pixels;
    unsigned at=((y/4)*(t->width/4)+x/4)*32+((y%4)*4+x%4)*2;
    unsigned word=(unsigned)p[at]<<8|p[at+1];
    if(word&0x8000){unsigned v=(word>>(green?5:0))&31;return (v<<3)|(v>>2);}
    return ((word>>(green?4:0))&15)*17;
}
static int8_t sample(const FhTexture*t,float u,float v,unsigned green){
    /* Source repeat wrapping before integer conversion keeps signed UVs and
     * seam interpolation bounded. Source dimensions are powers of two. */
    float x=(u-floorf(u))*t->width-.5f,y=(v-floorf(v))*t->height-.5f;
    int ix=(int)floorf(x),iy=(int)floorf(y);float fx=x-ix,fy=y-iy;
    unsigned x0=(unsigned)ix&(t->width-1),x1=(unsigned)(ix+1)&(t->width-1);
    unsigned y0=(unsigned)iy&(t->height-1),y1=(unsigned)(iy+1)&(t->height-1);
    float a=channel(t,x0,y0,green)*(1-fx)+channel(t,x1,y0,green)*fx;
    float b=channel(t,x0,y1,green)*(1-fx)+channel(t,x1,y1,green)*fx;
    int byte=(int)floorf(a*(1-fy)+b*fy+.5f);
    if(byte<0)byte=0;
    if(byte>255)byte=255;
    return (int8_t)(byte-128);
}
unsigned fh_pica_fur_groom_compile(const FhSceneAsset*a,const FhPicaModelDraw*d,
    const FhTexture*t,FhPicaFurGroomVertex*out,unsigned capacity){
    if(!a || !d || !t || !t->pixels || !a->gpu || !a->compact || a->block ||
       !(d->flags&512) || (d->format!=5 && d->format!=6) || !d->vertices ||
       d->vertices>8192 || d->vertex_offset>a->gpu_bytes ||
       d->vertices>(a->gpu_bytes-d->vertex_offset)/40 || t->format!=5 ||
       t->width<4 || t->height<4 || t->width>1024 || t->height>1024 ||
       (t->width&(t->width-1)) || (t->height&(t->height-1)) ||
       t->wrap_s!=1 || t->wrap_t!=1 || t->mag_filter!=1 ||
       (out && capacity<d->vertices))return 0;
    if(!out)return d->vertices;
    float scale=d->format==5?1.f/256:1.f/1024;
    for(unsigned i=0;i<d->vertices;i++){
        int16_t uv[2];memcpy(uv,a->gpu+d->vertex_offset+i*40+12,4);
        out[i]=(FhPicaFurGroomVertex){sample(t,uv[0]*scale,uv[1]*scale,0),
                                    sample(t,uv[0]*scale,uv[1]*scale,1),0,0};
    }
    return d->vertices;
}
