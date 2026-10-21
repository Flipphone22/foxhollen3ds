/* Search decisions only: two bits select orientation/colour mode. Colours,
 * alpha and pixel selectors are always computed from the user's source. */
#include "fh_install_texture_hints.h"
#include <mbedtls/sha256.h>
#include <string.h>
#include <zlib.h>
enum {HEADER=96,ENTRY=96,MAX_BYTES=2*1024*1024,MAX_RECORDS=4096};
static unsigned u16(const unsigned char*p){return p[0]|(unsigned)p[1]<<8;}
static unsigned u32(const unsigned char*p){return u16(p)|u16(p+2)<<16;}
static int extent(unsigned n){return n>=8&&n<=1024&&!(n&(n-1));}
static int digest(const void*p,size_t n,const unsigned char*expected){
 unsigned char sha[32];return !mbedtls_sha256_ret(p,n,sha,0)&&!memcmp(sha,expected,32);
}
static int compare(const unsigned char*a,const unsigned char*b){
 int order=memcmp(a,b,32);if(order)return order;
 for(unsigned at=64;at<72;at+=2){unsigned x=u16(a+at),y=u16(b+at);if(x!=y)return x<y?-1:1;}
 for(unsigned at=72;at<74;at++){if(a[at]!=b[at])return a[at]<b[at]?-1:1;}
 return 0;
}
int fh_install_texture_hints_open(FhInstallTextureHints*out,const void*data,size_t bytes,
 const unsigned char blob_sha[32],const unsigned char disc_sha[32],const unsigned char codec_sha[32]){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!data||!blob_sha||!disc_sha||!codec_sha||bytes<HEADER||bytes>MAX_BYTES)return 0;
 const unsigned char*p=data;unsigned count=u32(p+12);
 if(memcmp(p,"FHTXHNT1",8)||u32(p+8)!=1||!count||count>MAX_RECORDS||u32(p+16)!=bytes||
    memcmp(p+28,disc_sha,32)||memcmp(p+60,codec_sha,32)||u32(p+92))return 0;
 unsigned start=HEADER+count*ENTRY;if(start>bytes)return 0;
 if(!digest(p,bytes,blob_sha)||crc32(0,p+HEADER,count*ENTRY)!=u32(p+20)||crc32(0,p+start,bytes-start)!=u32(p+24))return 0;
 unsigned at=start;
 for(unsigned i=0;i<count;i++){
  const unsigned char*r=p+HEADER+i*ENTRY;
  unsigned w=u16(r+64),h=u16(r+66),gw=u16(r+68),gh=u16(r+70),num=u32(r+80),n=u32(r+88);
  if(!w||!h||w>1024||h>1024||!extent(gw)||!extent(gh)||r[72]!=14||(r[73]!=12&&r[73]!=13)||u16(r+74)||
     u32(r+76)!=((w+7)/8)*((h+7)/8)*32||num!=gw*gh/16||n!=(num+3)/4||u32(r+84)!=at||
     n>bytes-at||crc32(0,p+at,n)!=u32(r+92)||(i&&compare(r-ENTRY,r)>=0))return 0;
  if(num%4&&(p[at+n-1]>>(2*(num%4))))return 0;
  at+=n;
 }
 if(at!=bytes)return 0;
 out->data=p;out->bytes=bytes;out->count=count;return 1;
}
int fh_install_texture_hints_find(const FhInstallTextureHints*table,const void*raw,size_t bytes,
 unsigned w,unsigned h,unsigned gw,unsigned gh,unsigned format,FhInstallTextureHint*out){
 if(out)memset(out,0,sizeof(*out));
 if(!out||!raw||!bytes||!w||!h||w>1024||h>1024||!extent(gw)||!extent(gh)||format!=14||bytes!=((w+7)/8)*((h+7)/8)*32)return -1;
 if(!table||!table->data||!table->count)return 0;
 unsigned char key[ENTRY]={0};if(mbedtls_sha256_ret(raw,bytes,key,0))return -1;
 const unsigned dims[]={w,h,gw,gh};for(unsigned i=0;i<4;i++){key[64+2*i]=dims[i];key[65+2*i]=dims[i]>>8;}key[72]=format;
 /* Native format is determined by original alpha, not chosen by the caller.
  * Search the lower bound with format zero, then compare the identity prefix. */
 unsigned lo=0,hi=table->count;
 while(lo<hi){unsigned mid=lo+(hi-lo)/2;const unsigned char*r=table->data+HEADER+mid*ENTRY;
  if(compare(r,key)<0)lo=mid+1;else hi=mid;}
 if(lo==table->count)return 0;
 const unsigned char*r=table->data+HEADER+lo*ENTRY;key[73]=r[73];if(compare(r,key))return 0;
 out->modes=table->data+u32(r+84);out->output_sha=r+32;out->blocks=u32(r+80);
 out->format=r[73];out->bytes=out->blocks*(out->format==13?16:8);return 1;
}
int fh_install_texture_hint_verify(const FhInstallTextureHint*hint,const void*data,size_t bytes,unsigned format){
 return hint&&hint->output_sha&&data&&bytes==hint->bytes&&format==hint->format&&digest(data,bytes,hint->output_sha);
}
