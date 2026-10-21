#include "fh_stack_watermark.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef FH_HOST
#include <assert.h>
#include <sys/mman.h>
#include <unistd.h>
#endif
volatile unsigned checks;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noreturn)) void test_fail(unsigned n){(void)n;for(;;)__asm__ volatile("nop");}
__attribute__((noinline)) unsigned prior(const unsigned char *p,unsigned bytes){unsigned n=0;while(n<bytes&&p[n]==0xa5)n++;return n;}
__attribute__((noinline)) unsigned candidate(const unsigned char *p,unsigned bytes){return fh_stack_watermark_prefix(p,bytes);}
__attribute__((noinline)) void measured_begin(unsigned mode){__asm__ volatile(""::"r"(mode):"memory");}
__attribute__((noinline)) void measured_end(void){__asm__ volatile("":::"memory");}
#ifdef FH_HOST
#define CHECK(x) do{assert(x);checks++;}while(0)
int main(void){
 unsigned char data[520];
 CHECK(candidate(NULL,0)==0);
 for(unsigned align=0;align<8;align++)for(unsigned length=0;length<=256;length++){
  unsigned char *p=data+align;memset(data,0xa5,sizeof data);
  CHECK(candidate(p,length)==length);
  for(unsigned position=0;position<length;position++){
   p[position]=0;CHECK(candidate(p,length)==position);p[position]=0xa5;
  }
 }
 for(unsigned value=0;value<256;value++)for(unsigned position=0;position<64;position++){
  memset(data,0xa5,sizeof data);data[position]=(unsigned char)value;
  CHECK(candidate(data,64)==prior(data,64));
 }
 long page=sysconf(_SC_PAGESIZE);CHECK(page>=4096);
 unsigned char *guard=mmap(NULL,(size_t)page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
 CHECK(guard!=MAP_FAILED);CHECK(!mprotect(guard+page,(size_t)page,PROT_NONE));
 memset(guard,0xa5,(size_t)page);
 for(unsigned n=0;n<=256;n++)CHECK(candidate(guard+page-n,n)==n);
 CHECK(!munmap(guard,(size_t)page*2));
 unsigned seed=17;
 for(unsigned i=0;i<20000;i++){
  seed=seed*1664525+1013904223;unsigned n=seed%512;memset(data,0xa5,sizeof data);
  for(unsigned j=0;j<4&&n;j++){seed=seed*1664525+1013904223;data[seed%n]=(unsigned char)(seed>>24);}
  CHECK(candidate(data,n)==prior(data,n));
 }
 printf("PASS %u exact-prefix cases including protected ends\n",checks);return 0;
}
#else
#define CHECK(x) do{if(!(x))test_fail(__LINE__);checks++;}while(0)
static unsigned char data[240*1024] __attribute__((aligned(16)));
void test_main(void){
 unsigned lengths[2]={76*1024,236*1024};
 for(unsigned i=0;i<2;i++){
  unsigned n=lengths[i];memset(data,0xa5,sizeof data);
  measured_begin(0);unsigned a=prior(data,n);measured_end();
  measured_begin(1);unsigned b=candidate(data,n);measured_end();CHECK(a==n&&b==a);
  for(unsigned offset=0;offset<8;offset++){
   unsigned at=n-1-offset;data[at]=0;
   CHECK(candidate(data+offset,n-offset)==prior(data+offset,n-offset));data[at]=0xa5;
  }
 }
 test_pass();
}
#endif
