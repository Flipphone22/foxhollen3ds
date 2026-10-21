#include "fh_graphics.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned morton(unsigned x,unsigned y){unsigned r=0;for(unsigned b=0;b<3;b++)r|=((x>>b)&1)<<(2*b)|((y>>b)&1)<<(2*b+1);return r;}
int main(void){
 unsigned dims[][2]={{1,1},{2,3},{7,5},{8,8},{9,17},{15,31},{16,32},{31,33},{127,129},{320,240},{640,480},{1024,8}};
 unsigned checks=0;
 for(unsigned t=0;t<sizeof(dims)/sizeof(*dims);t++)for(unsigned offset=0;offset<8;offset++){
  unsigned w=dims[t][0],h=dims[t][1],nw=8,nh=8;while(nw<w)nw*=2;while(nh<h)nh*=2;
  size_t bytes=(size_t)w*h*4,outbytes=(size_t)nw*nh*4;
  unsigned char *allocation=malloc(bytes+offset),*source=allocation+offset;uint32_t *out=malloc(outbytes+32);
  assert(allocation && out);for(size_t i=0;i<bytes;i++)source[i]=(i*73+t*11)&255;memset(out,0xa5,outbytes+32);
  FhTexture texture={.pixels=source,.width=w,.height=h,.format=0x46};
  assert(fh_texture_convert(out+4,outbytes,nw,nh,&texture,bytes));
  for(unsigned y=0;y<nh;y++)for(unsigned x=0;x<nw;x++){
   unsigned sx=x<w?x:w-1,sy=y<h?y:h-1;const unsigned char*p=source+(sy*w+sx)*4;
   uint32_t expected=(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];
   assert(out[4+((y/8)*(nw/8)+x/8)*64+morton(x,y)]==expected);checks++;
  }
  for(unsigned i=0;i<4;i++){assert(out[i]==0xa5a5a5a5);assert(out[4+nw*nh+i]==0xa5a5a5a5);}
  free(allocation);free(out);
 }
 printf("%u exact RGBA pixels; eight source alignments, partial tiles, replicated borders and output guards\n",checks);return 0;
}
