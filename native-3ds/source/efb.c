/* Adapted from melee-3ds renderer.c efb_copy_cpu_reference, pinned in
 * vendor/melee/PROVENANCE.md. GPL-3.0; notices in native-3ds/licenses. */
#include "fh_graphics.h"
#include <string.h>
#include "fh_efb_rgb565.h"
size_t fh_efb_bytes(unsigned width,unsigned height,unsigned format) {
    if(!width || !height || width>1024 || height>1024) return 0;
    unsigned bw=4,bh=4,bytes=32;
    switch(format) {
    case 0:case 0x20:bw=bh=8;break;
    case 1:case 2:case 0x11:case 0x22:case 0x27:case 0x28:case 0x29:case 0x2a:bw=8;break;
    case 3:case 4:case 5:case 0x13:case 0x23:case 0x2b:case 0x2c:break;
    case 6:bytes=64;break;
    default:return 0;
    }
    return (size_t)((width+bw-1)/bw)*((height+bh-1)/bh)*bytes;
}
int fh_efb_convert_filtered_stride(void *destination,size_t capacity,const uint32_t *pixels,unsigned stride,const uint32_t rect[4],unsigned w,unsigned h,unsigned fmt,int has_alpha,int half_scale) {
    size_t required=fh_efb_bytes(w,h,fmt);
    if(stride<FH_EFB_NATIVE_HEIGHT || stride>1024 || !destination || !pixels || !required || capacity<required || rect[0]>640 || rect[1]>FH_EFB_HEIGHT || rect[2]>640-rect[0] || rect[3]>FH_EFB_HEIGHT-rect[1] || !rect[2] || !rect[3]) return 0;
    if(fmt==0x11 || fmt==0x13) return 0;
    if(half_scale && w==rect[2] && h==rect[3])half_scale=0;
    if(half_scale && (w*2!=rect[2] || h*2!=rect[3]))return 0;
    /* Native-resolution point sampling for aligned copies, shared with GPU
     * reflection/shadow paths. Unaligned requests retain their 2x2 filter. */
    if(half_scale && !(rect[0]&1) && !(rect[1]&1))half_scale=0;
    if(fmt==4 && !half_scale && fh_efb_rgb565_stride(destination,pixels,stride,w,h,rect))return 1;
    unsigned bw=4,bh=4,bytes=32;
    if(fmt==0 || fmt==0x20) bw=bh=8;
    else if(fmt==1 || fmt==2 || fmt==0x22 || (fmt>=0x27 && fmt<=0x2a)) bw=8;
    else if(fmt==6) bytes=64;
    unsigned pitch=(w+bw-1)/bw;unsigned char *dst=destination;
    memset(dst,0,required);
    unsigned source_x[1024],source_y[1024];
    for(unsigned x=0;x<w;x++)source_x[x]=fh_screen_x(rect[0]+x*rect[2]/w)*stride;
    for(unsigned y=0;y<h;y++)source_y[y]=FH_EFB_NATIVE_HEIGHT-1-(rect[1]+y*rect[3]/h)/2;
    /* Tile-local gathers avoid repeatedly walking 400 power-of-two-pitched
     * columns that alias ARM11 cache sets. Adjacent source rows stay hot;
     * output still uses exactly the original GX tile layout and padding. */
    for(unsigned by=0;by<h;by+=bh)for(unsigned bx=0;bx<w;bx+=bw){
        unsigned char *p=dst+((by/bh)*pitch+bx/bw)*bytes;
        unsigned end_x=bx+bw<w?bx+bw:w,end_y=by+bh<h?by+bh:h;
        for(unsigned x=bx;x<end_x;x++)for(unsigned y=by;y<end_y;y++){
        uint32_t color=pixels[source_x[x]+source_y[y]];
        if(half_scale) {
            unsigned sums[4]={0};
            for(unsigned dy=0;dy<2;dy++) for(unsigned dx=0;dx<2;dx++) {
                unsigned tx=fh_screen_x(rect[0]+2*x+dx),ty=(rect[1]+2*y+dy)/2;
                uint32_t sample=pixels[tx*stride+FH_EFB_NATIVE_HEIGHT-1-ty];
                for(unsigned c=0;c<4;c++) sums[c]+=(sample>>(8*c))&255;
            }
            color=0;for(unsigned c=0;c<4;c++) color|=(sums[c]/4)<<(8*c);
        }
        unsigned r=color>>24,g=(color>>16)&255,b=(color>>8)&255,a=has_alpha?color&255:255;
        unsigned k=(y-by)*bw+x-bx;
        unsigned intensity=(77*r+150*g+29*b)>>8,v=0;
        switch(fmt) {
        case 0:case 0x20:v=(fmt==0?intensity:r)>>4;p[k/2]|=v<<((k&1)?0:4);continue;
        case 1:p[k]=intensity;continue;
        case 2:case 0x22:p[k]=(a&0xf0)|((fmt==2?intensity:r)>>4);continue;
        case 3:case 0x23:v=(a<<8)|(fmt==3?intensity:r);break;
        case 4:v=((r>>3)<<11)|((g>>2)<<5)|(b>>3);break;
        case 5:v=a>=224?0x8000|((r>>3)<<10)|((g>>3)<<5)|(b>>3):((a>>5)<<12)|((r>>4)<<8)|((g>>4)<<4)|(b>>4);break;
        case 6:p[2*k]=a;p[2*k+1]=r;p[32+2*k]=g;p[32+2*k+1]=b;continue;
        case 0x27:p[k]=a;continue;
        case 0x28:p[k]=r;continue;
        case 0x29:p[k]=g;continue;
        case 0x2a:p[k]=b;continue;
        case 0x2b:v=(g<<8)|r;break;
        case 0x2c:v=(b<<8)|g;break;
        }
        p[2*k]=v>>8;p[2*k+1]=v;
        }
    }
    return 1;
}
int fh_efb_convert_filtered(void *destination,size_t capacity,const uint32_t *pixels,const uint32_t rect[4],unsigned w,unsigned h,unsigned fmt,int has_alpha,int half_scale) {
    return fh_efb_convert_filtered_stride(destination,capacity,pixels,FH_EFB_NATIVE_HEIGHT,rect,w,h,fmt,has_alpha,half_scale);
}
int fh_efb_convert(void *destination,size_t capacity,const uint32_t *pixels,const uint32_t rect[4],unsigned w,unsigned h,unsigned fmt,int has_alpha) {
    return fh_efb_convert_filtered(destination,capacity,pixels,rect,w,h,fmt,has_alpha,0);
}
uint32_t fh_efb_depth_pixel_stride(const uint32_t *pixels,unsigned stride,unsigned x,unsigned y) {
    unsigned tx=FH_EFB_NATIVE_HEIGHT-1-y/2,ty=fh_screen_x(x);
    /* DisplayTransfer already untiled the opaque depth/stencil words. */
    return 0xffffff-(pixels[ty*stride+tx]&0xffffff);
}
int fh_efb_depth_convert_stride(void *destination,size_t capacity,const uint32_t *pixels,unsigned stride,const uint32_t rect[4],unsigned w,unsigned h,unsigned fmt) {
    size_t bytes=fh_efb_bytes(w,h,fmt);
    if(stride<FH_EFB_NATIVE_HEIGHT || stride>1024 || !destination || !pixels || !bytes || capacity<bytes || (fmt!=0x11 && fmt!=0x13) || rect[0]>640 || rect[1]>FH_EFB_HEIGHT || !rect[2] || !rect[3] || rect[2]>640-rect[0] || rect[3]>FH_EFB_HEIGHT-rect[1]) return 0;
    /* The audited reflection copy samples the native EFB into GX Z8 texels.
     * Walk complete8x4 tiles, gathering four adjacent source words at a time,
     * instead of traversing the whole rotated framebuffer for every row. */
    if(fmt==0x11 && !(w&7) && !(h&3) && !((rect[0]|rect[1])&1) && rect[2]==2*w && rect[3]==2*h) {
        unsigned char *tile=destination;
        unsigned top=FH_EFB_NATIVE_HEIGHT-1-rect[1]/2;
        for(unsigned by=0;by<h;by+=4)for(unsigned bx=0;bx<w;bx+=8,tile+=32) {
            for(unsigned x=0;x<8;x++) {
                const uint32_t *p=pixels+fh_screen_x(rect[0]+2*(bx+x))*stride+top-by;
                tile[x]=(unsigned char)(~p[0]>>16);
                tile[x+8]=(unsigned char)(~p[-1]>>16);
                tile[x+16]=(unsigned char)(~p[-2]>>16);
                tile[x+24]=(unsigned char)(~p[-3]>>16);
            }
        }
        return 1;
    }
    unsigned bw=fmt==0x11?8:4;unsigned char *out=destination;memset(out,0,bytes);
    unsigned source_x[1024],source_y[1024],pitch=(w+bw-1)/bw;
    for(unsigned x=0;x<w;x++)source_x[x]=fh_screen_x(rect[0]+x*rect[2]/w)*stride;
    for(unsigned y=0;y<h;y++)source_y[y]=FH_EFB_NATIVE_HEIGHT-1-(rect[1]+y*rect[3]/h)/2;
    for(unsigned by=0;by<h;by+=4)for(unsigned bx=0;bx<w;bx+=bw){
        unsigned char *p=out+((by/4)*pitch+bx/bw)*32;
        unsigned end_x=bx+bw<w?bx+bw:w,end_y=by+4<h?by+4:h;
        for(unsigned x=bx;x<end_x;x++)for(unsigned y=by;y<end_y;y++){
        unsigned value=0xffffff-(pixels[source_x[x]+source_y[y]]&0xffffff);
        unsigned k=(y-by)*bw+x-bx;
        if(fmt==0x11) p[k]=value>>16;
        else {p[2*k]=value>>16;p[2*k+1]=value>>8;}
        }
    }
    return 1;
}

uint32_t fh_efb_depth_pixel(const uint32_t *pixels,unsigned x,unsigned y) {
    return fh_efb_depth_pixel_stride(pixels,FH_EFB_NATIVE_HEIGHT,x,y);
}
int fh_efb_depth_convert(void *destination,size_t capacity,const uint32_t *pixels,const uint32_t rect[4],unsigned w,unsigned h,unsigned fmt) {
    return fh_efb_depth_convert_stride(destination,capacity,pixels,FH_EFB_NATIVE_HEIGHT,rect,w,h,fmt);
}
