#include "fh_graphics.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t packed[400*FH_EFB_NATIVE_HEIGHT];
static unsigned char expected[1024*1024*4+8],actual[1024*1024*4+8];
int main(void) {
    unsigned seed=317,checks=0;
    for(unsigned i=0;i<400*FH_EFB_NATIVE_HEIGHT;i++){
        seed=seed*1664525u+1013904223u;packed[i]=seed;
    }
    const unsigned formats[]={0,1,2,3,4,5,6,0x20,0x22,0x23,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x11,0x13};
    const unsigned requests[][7]={
        {0,0,640,528,320,264,1},{1,1,638,526,319,263,1},
        {11,17,617,451,319,239,0},{2,4,638,476,7,9,0},
        {0,0,640,480,640,480,0},{638,526,2,2,1,1,1}
    };
    for(unsigned stride=264;stride<=512;stride=stride==264?272:stride==272?512:513){
        size_t bytes=400*stride*sizeof(uint32_t);
        uint32_t *padded=malloc(bytes),*before=malloc(bytes);assert(padded&&before);
        memset(padded,0xcd,bytes);
        for(unsigned x=0;x<400;x++)memcpy(padded+x*stride,packed+x*264,264*4);
        memcpy(before,padded,bytes);
        for(unsigned f=0;f<sizeof(formats)/sizeof(*formats);f++)
        for(unsigned r=0;r<sizeof(requests)/sizeof(*requests);r++)
        for(unsigned alpha=0;alpha<2;alpha++)for(unsigned alignment=0;alignment<4;alignment++){
            const unsigned *q=requests[r];unsigned format=formats[f];
            size_t n=fh_efb_bytes(q[4],q[5],format);assert(n+8<=sizeof(actual));
            memset(actual,0xaa,n+8);memset(expected,0xaa,n+8);
            if(format==0x11 || format==0x13){
                assert(fh_efb_depth_convert(expected+alignment,n,packed,q,q[4],q[5],format));
                assert(fh_efb_depth_convert_stride(actual+alignment,n,padded,stride,q,q[4],q[5],format));
            }else{
                assert(fh_efb_convert_filtered(expected+alignment,n,packed,q,q[4],q[5],format,alpha,q[6]));
                assert(fh_efb_convert_filtered_stride(actual+alignment,n,padded,stride,q,q[4],q[5],format,alpha,q[6]));
            }
            assert(!memcmp(actual,expected,n+8));checks++;
        }
        for(unsigned y=0;y<528;y+=7)for(unsigned x=0;x<640;x+=11){
            assert(fh_efb_depth_pixel_stride(padded,stride,x,y)==fh_efb_depth_pixel(packed,x,y));checks++;
        }
        assert(!memcmp(before,padded,bytes));free(before);free(padded);
    }
    const uint32_t rect[]={0,0,2,2};
    const unsigned invalid[]={263,1025};
    for(unsigned i=0;i<2;i++){
        unsigned stride=invalid[i];
        assert(!fh_efb_convert_filtered_stride(actual,sizeof(actual),packed,stride,rect,1,1,6,1,1));
        assert(!fh_efb_depth_convert_stride(actual,sizeof(actual),packed,stride,rect,1,1,0x11));
    }
    puts("Invalid stride checks passed");
    printf("PASS %u strided EFB format/filter/alignment/point comparisons; source and output guards unchanged\n",checks);
}
