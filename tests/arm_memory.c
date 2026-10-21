#include "fh_memory_bytes.h"
#include "fh_hash_bytes.h"
#include <stdint.h>
#ifdef FH_TEST_SDK_MEMORY_COPY
#include <string.h>
static void *(*volatile copy_bytes)(void*,const void*,size_t)=memcpy;
#else
#define copy_bytes fh_memory_copy
#endif
volatile unsigned test_checks;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;){}}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){(void)n;for(;;){}}
static void check(int value){test_checks++;if(!value)test_fail(test_checks);}
static int reference(const unsigned char*a,const unsigned char*b,unsigned n){for(unsigned i=0;i<n;i++)if(a[i]!=b[i])return (int)a[i]-b[i];return 0;}
static unsigned hash_reference(unsigned hash,const unsigned char*p,unsigned n){
 while(n>=4){unsigned word=p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;hash=(hash^word)*16777619u;p+=4;n-=4;}
 while(n--)hash=(hash^*p++)*16777619u;return hash;
}
void test_main(void){
 unsigned char a[300] __attribute__((aligned(32))),b[300] __attribute__((aligned(32))),out[300] __attribute__((aligned(32)));
 for(unsigned x=0;x<8;x++)for(unsigned y=0;y<8;y++)for(unsigned n=0;n<=260;n+=n<40?1:11){
  for(unsigned i=0;i<300;i++){a[i]=(i*73+19)&255;b[i]=(i*53+31)&255;out[i]=0x5a;}
  for(unsigned i=0;i<n;i++)b[y+i]=a[x+i];
  check(fh_memory_compare(a+x,b+y,n)==0);
  check(fh_hash_bytes(2166136261u,a+x,n)==hash_reference(2166136261u,a+x,n));
  for(unsigned k=0;n && k<3;k++){
   unsigned at=k==0?0:k==1?n-1:n/2;b[y+at]^=0x81;
   check(fh_memory_compare(a+x,b+y,n)==reference(a+x,b+y,n));
   check(fh_memory_compare(b+y,a+x,n)==reference(b+y,a+x,n));b[y+at]^=0x81;
  }
  check(copy_bytes(out+y,a+x,n)==out+y);
  check(!reference(out+y,a+x,n));
  for(unsigned i=0;i<y;i++)check(out[i]==0x5a);
  for(unsigned i=y+n;i<300;i++)check(out[i]==0x5a);
  fh_memory_zero(out+y,n);
  for(unsigned i=0;i<300;i++)check(out[i]==(i>=y && i<y+n?0:0x5a));
 }
 /* The runner maps [0x10000,0x1010000). Reads/writes beyond this endpoint
  * fault in Unicorn; every tail length and both operand orders are covered. */
 unsigned char *end=(unsigned char*)0x1010000;
 for(unsigned n=0;n<128;n++)for(unsigned offset=0;offset<8;offset++){
  for(unsigned i=0;i<n;i++)end[-(int)n+i]=a[offset+i];
  check(fh_memory_compare(end-n,a+offset,n)==0);
  check(fh_hash_bytes(2166136261u,end-n,n)==hash_reference(2166136261u,a+offset,n));
  check(fh_memory_compare(a+offset,end-n,n)==0);
  check(copy_bytes(end-n,a+offset,n)==end-n);
  check(copy_bytes(out+offset,end-n,n)==out+offset);
  check(!reference(out+offset,a+offset,n));
  fh_memory_zero(end-n,n);for(unsigned i=0;i<n;i++)check(end[-(int)n+i]==0);
 }
 test_pass();
}
