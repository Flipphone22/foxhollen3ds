#include "fh_graphics.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
static unsigned spread(unsigned x,unsigned y){
    unsigned r=0;for(unsigned i=0;i<3;i++)r|=((x>>i)&1)<<(i*2)|((y>>i)&1)<<(i*2+1);return r;
}
static unsigned expand(unsigned v,unsigned n){return n==5?(v*8+v/4):(v*4+v/16);}
static void pixel(const unsigned char *block,unsigned selector,unsigned out[4]){
    unsigned a=block[0]*256+block[1],b=block[2]*256+block[3];
    unsigned c[2][3]={{expand(a>>11,5),expand((a>>5)&63,6),expand(a&31,5)},
                      {expand(b>>11,5),expand((b>>5)&63,6),expand(b&31,5)}};
    for(unsigned k=0;k<3;k++)out[k]=selector<2?c[selector][k]:a>b?
        (c[selector==2?0:1][k]*5+c[selector==2?1:0][k]*3)/8:(c[0][k]+c[1][k])/2;
    out[3]=(a<=b && selector==3)?0:255;
}
int main(void){
    unsigned checks=0,max_error=0;double squared=0;
    /* Every 565 endpoint, ordered/reversed/equal pairs and all selectors. */
    for(unsigned a=0;a<65536;a++)for(unsigned variant=0;variant<3;variant++){
        unsigned b=variant==0?a:variant==1?65535-a:(a*177+19)&65535;
        unsigned char source[32];for(unsigned q=0;q<4;q++){
            unsigned char *p=source+q*8;p[0]=a>>8;p[1]=a;p[2]=b>>8;p[3]=b;
            memset(p+4,0x1b,4);
        }
        uint16_t output[64];FhTexture t={.pixels=source,.format=14,.width=8,.height=8};
        assert(fh_texture_convert_cmpr16(output,sizeof(output),8,8,&t,32));
        for(unsigned x=0;x<4;x++){
            unsigned expected[4];pixel(source,x,expected);unsigned p=output[spread(x,0)];
            unsigned got[4]={expand(p>>11,5),expand((p>>6)&31,5),expand((p>>1)&31,5),(p&1)*255};
            assert(got[3]==expected[3]);
            for(unsigned k=0;k<3;k++){unsigned e=abs((int)got[k]-(int)expected[k]);assert(e<=4);if(e>max_error)max_error=e;squared+=e*e;}
            checks++;
        }
    }
    unsigned edges=0;
    for(unsigned trial=0;trial<35;trial++)for(unsigned align=0;align<4;align++){
        unsigned w=trial==34?512:1+trial*23%133,h=trial==34?127:1+trial*31%129,nw=8,nh=8;
        while(nw<w)nw*=2;while(nh<h)nh*=2;
        unsigned bytes=((w+7)/8)*((h+7)/8)*32,n=nw*nh;
        unsigned char *allocation=malloc(bytes+align),*src=allocation+align;
        uint16_t *out=malloc(n*2+32);memset(out,0xa5,n*2+32);
        for(unsigned i=0;i<bytes;i++)src[i]=(i*173+trial*79)&255;
        FhTexture t={.pixels=src,.format=14,.width=w,.height=h};
        assert(!fh_texture_convert_cmpr16(out+8,n*2-1,nw,nh,&t,bytes));
        assert(!fh_texture_convert_cmpr16(out+8,n*2,nw,nh,&t,bytes-1));
        assert(fh_texture_convert_cmpr16(out+8,n*2,nw,nh,&t,bytes));
        for(unsigned y=0;y<nh;y++)for(unsigned x=0;x<nw;x++){
            unsigned sx=x<w?x:w-1,sy=y<h?y:h-1;
            const unsigned char *p=src+((sy/8)*((w+7)/8)+sx/8)*32+(sy%8/4*2+sx%8/4)*8;
            unsigned expected[4];pixel(p,(p[4+sy%4]>>(6-sx%4*2))&3,expected);
            unsigned v=out[8+((y/8)*(nw/8)+x/8)*64+spread(x,y)];
            assert((v&1)*255==expected[3]);
            unsigned rgb[3]={expand(v>>11,5),expand((v>>6)&31,5),expand((v>>1)&31,5)};
            for(unsigned k=0;k<3;k++)assert(abs((int)rgb[k]-(int)expected[k])<=4);
            edges++;
        }
        for(unsigned i=0;i<8;i++)assert(out[i]==0xa5a5 && out[8+n+i]==0xa5a5);
        free(out);free(allocation);
    }
    printf("CMPR16: %u palette cases, %u edge/alignment pixels, exact alpha, max RGB error %u/255, RGB MSE %.6f\n",checks,edges,max_error,squared/(checks*3));
}
