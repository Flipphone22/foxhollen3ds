#include "fh_pica_command_chain.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define H(r,m) ((r)|((m)<<16))
static const uint32_t packet[30]={
  0x300,H(0x25e,2),1,H(0x25f,15),0x80001000,H(0x227,15),6,H(0x228,15),
  0,H(0x22a,15),0x100,H(0x229,2),0x100,H(0x253,2),0,H(0x245,1),
  1,H(0x22f,15),1,H(0x245,1),0,H(0x229,2),0,H(0x253,2),
  1,H(0x231,15),0,H(0x25e,8),0,H(0x25e,8)};
static uint32_t leaf[16384] __attribute__((aligned(16)));
static unsigned leaf_words=32;
static uint32_t words[8192] __attribute__((aligned(16))),before[8192];
static FhPicaChainReturn fix[512];
static unsigned checks;
/* Independent interpreter: both channels redirect rather than call/return.
 * Returned range must reach FINALIZE, and effective register writes must match
 * the original inline packets in order. Exact lengths/ranges checked each hop.
 */
static void execute(unsigned total,unsigned calls){
 const uint32_t *w=words;unsigned end=total,at=0,draws=0,step=0,index=0;
 uint32_t size[2]={0},addr[2]={0};
 while(++step<40000){
  assert(at+2<=end);uint32_t v=w[at],h=w[at+1];at+=2;
  assert(!(h&0xfff00000u));unsigned reg=h&65535,mask=(h>>16)&15;
  if(!mask)continue;
  if(reg==0x10){assert(w==words&&draws==calls&&index==0);checks++;return;}
  if(reg==0x238||reg==0x239){size[reg-0x238]=v*8;continue;}
  if(reg==0x23a||reg==0x23b){addr[reg-0x23a]=v*8;continue;}
  if(reg==0x23c||reg==0x23d){
   unsigned channel=reg-0x23c;assert(v==1&&!(addr[channel]&15)&&!(size[channel]&15));
   if(channel==0){assert(w==words&&addr[0]==0x20000000&&size[0]==leaf_words*4);w=leaf;end=leaf_words;at=0;}
   else{assert(w==leaf&&addr[1]>=0x21000000);at=(addr[1]-0x21000000)/4;
    assert(at<total&&size[1]==(total-at)*4);end=total;w=words;}
   continue;
  }
  assert(v==packet[2*index]&&h==packet[2*index+1]);
  if(++index==15){index=0;draws++;}
 }
 assert(!"bounded interpreter did not finish");
}
int main(void){
 assert(fh_pica_chain_draw(leaf,packet));assert(!memcmp(leaf,packet,120));
 assert(leaf[30]==1&&leaf[31]==H(0x23d,15));
 uint32_t bad[30];
 for(unsigned i=0;i<30;i++){
  memcpy(bad,packet,120);bad[i]^=0x4000;uint32_t saved[32];memcpy(saved,leaf,128);
  int ok=fh_pica_chain_draw(leaf,bad);
  if(i!=4&&i!=6){assert(!ok);assert(!memcmp(saved,leaf,128));}checks++;
 }
 assert(fh_pica_chain_draw(leaf,packet));assert(!fh_pica_chain_draw(leaf,leaf));
 for(unsigned calls=1;calls<=400;calls++)for(unsigned parity=0;parity<2;parity++){
  memset(words,0x91,sizeof words);unsigned at=parity*2;
  if(at){words[0]=0;words[1]=H(0x25e,0);}
  for(unsigned i=0;i<calls;i++){
   unsigned old=at;assert(fh_pica_chain_call(words,8192,&at,0x20000000,fix+i)==12+(old&2));
   assert(!(fix[i].word&3));checks++;
  }
  words[at++]=0x12345678;words[at++]=H(0x10,15);words[at++]=0;words[at++]=0;
  assert(fh_pica_chain_seal(words,at,0x21000000,fix,calls));execute(at,calls);
  memcpy(before,words,sizeof words);
  assert(!fh_pica_chain_seal(words,at,0x21000000,fix,calls));
  assert(!memcmp(words,before,sizeof words));checks++;
 }
 for(unsigned cap=0;cap<20;cap++)for(unsigned start=0;start<8;start++){
  memset(words,0x73,sizeof words);memcpy(before,words,sizeof words);
  unsigned at=start;FhPicaChainReturn f={0xdeadbeef};
  unsigned n=fh_pica_chain_call(words,cap,&at,0x20000000,&f);
  int valid=!(start&1)&&start<=cap&&12+(start&2)<=cap-start;
  assert(!!n==valid);
  if(!n){assert(at==start&&f.word==0xdeadbeef&&!memcmp(words,before,sizeof words));}
  checks++;
 }
 /* Last relocation corruption must not partially seal earlier valid calls. */
 for(unsigned word=0;word<12;word++){
  unsigned at=0;assert(fh_pica_chain_call(words,8192,&at,0x20000000,fix));
  assert(fh_pica_chain_call(words,8192,&at,0x20000000,fix+1));
  words[at++]=0x12345678;words[at++]=H(0x10,15);words[at++]=0;words[at++]=0;
  words[fix[1].word+word]=0xffffffffu;memcpy(before,words,sizeof words);
  assert(!fh_pica_chain_seal(words,at,0x21000000,fix,2));assert(!memcmp(words,before,sizeof words));checks++;
 }
 for(unsigned what=0;what<6;what++){
  unsigned at=0;assert(fh_pica_chain_call(words,8192,&at,0x20000000,fix));
  assert(fh_pica_chain_call(words,8192,&at,0x20000000,fix+1));
  words[at++]=0x12345678;words[at++]=H(0x10,15);words[at++]=0;words[at++]=0;
  if(what==0)fix[1].word=0;
  if(what==1)fix[1].word=at;
  if(what==2)fix[1].word|=2;
  if(what==3)words[fix[1].word+4]=0x21000000/8;
  memcpy(before,words,sizeof words);
  assert(!fh_pica_chain_seal(words,what==4?at-2:at,what==5?0x21000008:0x21000000,fix,2));
  assert(!memcmp(words,before,sizeof words));checks++;
 }
 for(unsigned bytes=16;bytes<=65536;bytes*=2){
  unsigned at=0;assert(fh_pica_chain_group_call(words,8192,&at,0x20000000,bytes,fix));
  words[at++]=0x12345678;words[at++]=H(0x10,15);words[at++]=0;words[at++]=0;
  assert(fh_pica_chain_seal(words,at,0x21000000,fix,1));
  if(bytes>=128){
   leaf_words=bytes/4;
   for(unsigned j=0;j<leaf_words;j+=2){leaf[j]=0;leaf[j+1]=H(0x25e,0);}
   assert(fh_pica_chain_draw(leaf+leaf_words-32,packet));execute(at,1);
  }
  checks++;
 }
 const unsigned bad_sizes[]={0,8,127,129,65544,65552,0xfffffff0};
 for(unsigned i=0;i<sizeof(bad_sizes)/sizeof(*bad_sizes);i++){
  memcpy(before,words,sizeof words);unsigned at=0;FhPicaChainReturn f={77};
  assert(!fh_pica_chain_group_call(words,8192,&at,0x20000000,bad_sizes[i],&f));
  assert(!at&&f.word==77&&!memcmp(words,before,sizeof words));checks++;
 }
 printf("{\"passed\":true,\"checks\":%u,\"interpreted_sequences\":810}\n",checks);
}
