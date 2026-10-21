#include "fh_graphics.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned morton(unsigned x,unsigned y){unsigned r=0;for(unsigned b=0;b<3;b++)r|=((x>>b)&1)<<(2*b)|((y>>b)&1)<<(2*b+1);return r;}
int main(void){
 unsigned checks=0;
 for(unsigned format=3;format<=4;format++)for(unsigned trial=0;trial<100;trial++)for(unsigned offset=0;offset<8;offset++){
  unsigned w=1+trial*37%129,h=1+trial*59%131,nw=8,nh=8;
  if(trial==97){w=320;h=240;}if(trial==98){w=640;h=406;}if(trial==99){w=1024;h=1024;}
  while(nw<w)nw*=2;while(nh<h)nh*=2;
  size_t bytes=fh_texture_source_bytes(format,w,h),outbytes=(size_t)nw*nh*2;
  unsigned char *allocation=malloc(bytes+offset),*source=allocation+offset;uint16_t *out=malloc(outbytes+32);
  assert(allocation && out);for(size_t i=0;i<bytes;i++)source[i]=(i*73+trial*11)&255;memset(out,0xa5,outbytes+32);
  FhTexture texture={.pixels=source,.width=w,.height=h,.format=format};
  assert(fh_texture_convert(out+8,outbytes,nw,nh,&texture,bytes));
  for(unsigned y=0;y<nh;y++)for(unsigned x=0;x<nw;x++){
   unsigned sx=x<w?x:w-1,sy=y<h?y:h-1;
   const unsigned char*p=source+((sy/4)*((w+3)/4)+sx/4)*32+(sy%4)*8+(sx%4)*2;
   unsigned expected=format==4?((unsigned)p[0]<<8)|p[1]:p[0]|((unsigned)p[1]<<8);
   assert(out[8+((y/8)*(nw/8)+x/8)*64+morton(x,y)]==expected);checks++;
  }
  for(unsigned i=0;i<8;i++){assert(out[i]==0xa5a5);assert(out[8+nw*nh+i]==0xa5a5);}
  free(allocation);free(out);
 }
 printf("%u exact RGB565/IA8 pixels; eight source alignments, partial tiles, replicated borders and output guards\n",checks);return 0;
}
