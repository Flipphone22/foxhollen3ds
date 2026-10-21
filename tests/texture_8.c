#include "fh_graphics.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned morton(unsigned x,unsigned y){unsigned r=0;for(unsigned b=0;b<3;b++)r|=((x>>b)&1)<<(2*b)|((y>>b)&1)<<(2*b+1);return r;}
int main(void){
 unsigned checks=0;
 for(unsigned trial=0;trial<100;trial++)for(unsigned offset=0;offset<8;offset++){
  unsigned w=1+trial*37%129,h=1+trial*59%131,nw=8,nh=8;
  if(trial==97){w=320;h=240;}if(trial==98){w=640;h=406;}if(trial==99){w=1024;h=1024;}
  while(nw<w)nw*=2;while(nh<h)nh*=2;
  size_t bytes=fh_texture_source_bytes(1,w,h),outbytes=(size_t)nw*nh;
  unsigned char *allocation=malloc(bytes+offset),*source=allocation+offset,*out=malloc(outbytes+32);
  assert(allocation && out);for(size_t i=0;i<bytes;i++)source[i]=(i*73+(i/256)*29+trial*11)&255;memset(out,0xa5,outbytes+32);
  FhTexture texture={.pixels=source,.width=w,.height=h,.format=1};
  assert(!fh_texture_convert(out+16,outbytes-1,nw,nh,&texture,bytes));
  assert(!fh_texture_convert(out+16,outbytes,nw,nh,&texture,bytes-1));
  assert(fh_texture_convert(out+16,outbytes,nw,nh,&texture,bytes));
  for(unsigned y=0;y<nh;y++)for(unsigned x=0;x<nw;x++){
   unsigned sx=x<w?x:w-1,sy=y<h?y:h-1;
   unsigned expected=source[((sy/4)*((w+7)/8)+sx/8)*32+(sy%4)*8+(sx%8)];
   assert(out[16+((y/8)*(nw/8)+x/8)*64+morton(x,y)]==expected);checks++;
  }
  for(unsigned i=0;i<16;i++){assert(out[i]==0xa5);assert(out[16+nw*nh+i]==0xa5);}
  free(allocation);free(out);
 }
 printf("%u exact I8 pixels; eight source alignments, partial tiles, replicated borders, size rejection and guards\n",checks);return 0;
}
