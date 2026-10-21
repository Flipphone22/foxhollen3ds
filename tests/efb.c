#include "fh_graphics.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static uint32_t pixels[400*FH_EFB_NATIVE_HEIGHT];
static unsigned char storage[1024*1024*4+32];
int main(void) {
    for(unsigned x=0;x<400;x++) for(unsigned y=0;y<FH_EFB_NATIVE_HEIGHT;y++) pixels[x*FH_EFB_NATIVE_HEIGHT+FH_EFB_NATIVE_HEIGHT-1-y]=(x&255)<<24|(y&255)<<16|((x+y)&255)<<8|((3*x+y)&255);
    unsigned checks=0;
    const unsigned formats[]={0,1,2,3,4,5,6,0x20,0x22,0x23,0x27,0x28,0x29,0x2a,0x2b,0x2c};
    for(unsigned f=0;f<16;f++) for(unsigned alpha=0;alpha<2;alpha++) for(unsigned size=0;size<5;size++) {
        unsigned w=(unsigned[]){1,7,32,319,640}[size],h=(unsigned[]){1,9,24,239,480}[size],fmt=formats[f];
        uint32_t rect[]={11,17,617,451};size_t bytes=fh_efb_bytes(w,h,fmt);assert(bytes && bytes<=sizeof(storage)-32);
        memset(storage,0xaa,bytes+32);assert(fh_efb_convert(storage,bytes,pixels,rect,w,h,fmt,alpha));
        for(unsigned i=0;i<32;i++) assert(storage[bytes+i]==0xaa);
        unsigned bw=(fmt==0 || fmt==0x20 || fmt==1 || fmt==2 || fmt==0x22 || (fmt>=0x27 && fmt<=0x2a))?8:4;
        unsigned bh=fmt==0 || fmt==0x20?8:4,tile_bytes=fmt==6?64:32;
        for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
            unsigned sx=(11+x*617/w)*400/640,sy=(17+y*451/h)/2;
            unsigned r=sx&255,g=sy&255,b=(sx+sy)&255,a=alpha?(3*sx+sy)&255:255,I=(77*r+150*g+29*b)>>8;
            unsigned k=(y%bh)*bw+x%bw,offset=((y/bh)*((w+bw-1)/bw)+x/bw)*tile_bytes;
            unsigned char *p=storage+offset;unsigned expected=0,actual=0;
            switch(fmt) {
            case 0:case 0x20:expected=(fmt==0?I:r)>>4;actual=(p[k/2]>>((k&1)?0:4))&15;break;
            case 1:expected=I;actual=p[k];break;
            case 2:case 0x22:expected=(a&240)|((fmt==2?I:r)>>4);actual=p[k];break;
            case 3:case 0x23:expected=(a<<8)|(fmt==3?I:r);actual=p[2*k]*256+p[2*k+1];break;
            case 4:expected=(r/8)*2048+(g/4)*32+b/8;actual=p[2*k]*256+p[2*k+1];break;
            case 5:expected=a>=224?32768+(r/8)*1024+(g/8)*32+b/8:(a/32)*4096+(r/16)*256+(g/16)*16+b/16;actual=p[2*k]*256+p[2*k+1];break;
            case 6:expected=(r<<24)|(g<<16)|(b<<8)|a;actual=(unsigned)p[2*k+1]<<24|(unsigned)p[32+2*k]<<16|(unsigned)p[33+2*k]<<8|p[2*k];break;
            case 0x27:expected=a;actual=p[k];break;
            case 0x28:expected=r;actual=p[k];break;
            case 0x29:expected=g;actual=p[k];break;
            case 0x2a:expected=b;actual=p[k];break;
            case 0x2b:expected=(g<<8)|r;actual=p[2*k]*256+p[2*k+1];break;
            case 0x2c:expected=(b<<8)|g;actual=p[2*k]*256+p[2*k+1];break;
            }
            assert(actual==expected);checks++;
        }
    }
    uint32_t bad[]={639,0,2,1};assert(!fh_efb_convert(storage,sizeof(storage),pixels,bad,1,1,6,1));
    uint32_t shadow[]={0,0,512,512};
    assert(fh_efb_convert_filtered(storage,sizeof(storage),pixels,shadow,256,256,6,1,1));
    for(unsigned y=240;y<256;y++)for(unsigned x=0;x<256;x++) {
        unsigned char *p=storage+((y/4)*64+x/4)*64;unsigned k=(y%4)*4+x%4;
        assert(p[2*k+1]==((x*400/320)&255) && p[32+2*k]==y);checks++;
    }
    unsigned char *unfiltered=malloc(256*256*4);assert(unfiltered);
    shadow[2]=shadow[3]=256;
    assert(fh_efb_convert(unfiltered,256*256*4,pixels,shadow,256,256,6,1));
    assert(fh_efb_convert_filtered(storage,sizeof(storage),pixels,shadow,256,256,6,1,1));
    assert(!memcmp(storage,unfiltered,256*256*4));free(unfiltered);checks+=256*256;
    for(unsigned parity=0;parity<4;parity++) {
        uint32_t rect[]={parity%2,parity/2,638,478};
        assert(fh_efb_convert_filtered(storage,sizeof(storage),pixels,rect,319,239,6,1,1));
        for(unsigned y=0;y<239;y++) for(unsigned x=0;x<319;x++) {
            unsigned sum[4]={0};
            for(unsigned dy=0;dy<2;dy++) for(unsigned dx=0;dx<2;dx++) {
                unsigned sx=(rect[0]+2*x+(parity?dx:0))*400/640,sy=(rect[1]+2*y+(parity?dy:0))/2;
                sum[0]+=sx&255;sum[1]+=sy&255;sum[2]+=(sx+sy)&255;sum[3]+=(3*sx+sy)&255;
            }
            unsigned char *p=storage+((y/4)*80+x/4)*64;unsigned k=(y%4)*4+x%4;
            assert(p[2*k+1]==sum[0]/4 && p[32+2*k]==sum[1]/4 && p[33+2*k]==sum[2]/4 && p[2*k]==sum[3]/4);checks++;
        }
    }
    /* Aligned row fast path must preserve padding and retain the byte-safe
     * fallback for destinations with each possible alignment. */
    for(unsigned alignment=0;alignment<4;alignment++) {
        uint32_t rect[]={2,4,638,476};unsigned char expected[192],actual[196];
        memset(actual,0xaa,sizeof(actual));
        assert(fh_efb_convert(expected,sizeof(expected),pixels,rect,7,9,4,0));
        assert(fh_efb_convert(actual+alignment,192,pixels,rect,7,9,4,0));
        assert(!memcmp(actual+alignment,expected,192));
        for(unsigned i=0;i<alignment;i++)assert(actual[i]==0xaa);
        for(unsigned i=alignment+192;i<sizeof(actual);i++)assert(actual[i]==0xaa);
        checks+=192;
    }
    for(unsigned y=0;y<400;y++) for(unsigned x=0;x<FH_EFB_NATIVE_HEIGHT;x++) {
        pixels[y*FH_EFB_NATIVE_HEIGHT+x]=0xa5000000|((x*65537+y*43)&0xffffff);
    }
    for(unsigned fmt=0x11;fmt<=0x13;fmt+=2) {
        uint32_t rect[]={3,5,636,474};unsigned w=318,h=237,bw=fmt==0x11?8:4;
        assert(fh_efb_depth_convert(storage,sizeof(storage),pixels,rect,w,h,fmt));
        for(unsigned y=0;y<h;y++) for(unsigned x=0;x<w;x++) {
            unsigned sx=FH_EFB_NATIVE_HEIGHT-1-(5+2*y)/2,sy=(3+2*x)*400/640;
            unsigned depth=0xffffff-((sx*65537+sy*43)&0xffffff);
            assert(fh_efb_depth_pixel(pixels,3+2*x,5+2*y)==depth);
            unsigned char *p=storage+((y/4)*((w+bw-1)/bw)+x/bw)*32;unsigned k=(y%4)*bw+x%bw;
            if(fmt==0x11)assert(p[k]==depth>>16);else assert(p[2*k]*256+p[2*k+1]==depth>>8);checks++;
        }
    }
    assert(!fh_efb_depth_convert(storage,sizeof(storage),pixels,bad,1,1,0x11));
    /* Exact tiled Z8 path: full PAL/native height, offscreen shadows,
     * cropped origins, every destination alignment, and untouched guards. */
    const uint32_t depth_rects[][4]={{0,0,640,480},{0,0,512,512},{0,0,640,528},{2,4,16,8},{14,12,624,480}};
    for(unsigned trial=0;trial<5;trial++)for(unsigned alignment=0;alignment<4;alignment++) {
        const uint32_t *rect=depth_rects[trial];unsigned w=rect[2]/2,h=rect[3]/2,bytes=w*h;
        memset(storage,0xaa,bytes+32);
        assert(fh_efb_depth_convert(storage+alignment,bytes,pixels,rect,w,h,0x11));
        for(unsigned y=0;y<h;y++)for(unsigned x=0;x<w;x++) {
            unsigned expected=fh_efb_depth_pixel(pixels,rect[0]+2*x,rect[1]+2*y)>>16;
            unsigned offset=((y/4)*(w/8)+x/8)*32+(y&3)*8+(x&7);
            assert(storage[alignment+offset]==expected);checks++;
        }
        for(unsigned i=0;i<alignment;i++)assert(storage[i]==0xaa);
        for(unsigned i=alignment+bytes;i<bytes+32;i++)assert(storage[i]==0xaa);
    }
    assert(!fh_efb_bytes(0,1,6));assert(!fh_efb_bytes(32,32,0xff));
    printf("PASS %u EFB color/depth samples, half-scale parity, crop/scale, opaque alpha, edge tiles and bounds\n",checks);
}
