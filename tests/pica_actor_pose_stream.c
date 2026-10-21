#include <3ds.h>
#include <string.h>
#include "fh_pica_actor_commands.h"
volatile unsigned checks,failure,hits,misses;
__attribute__((noinline,noreturn)) void test_pass(void){for(;;)__asm__ volatile("nop");}
__attribute__((noinline,noclone,noreturn)) void test_fail(unsigned n){failure=n;for(;;)__asm__ volatile("nop");}
#define CHECK(x) do{if(!(x))test_fail(__LINE__);checks++;}while(0)
__attribute__((noinline)) void measured_begin(unsigned mode){__asm__ volatile(""::"r"(mode):"memory");}
__attribute__((noinline)) void measured_end(void){__asm__ volatile("":::"memory");}
extern unsigned reference_append(uint32_t*,unsigned,unsigned*,FhPicaActorCommandState*,const FhPicaActorGeometry*,const FhPicaActorParameters*,const FhPicaActorPose*,unsigned);
static float joints[256*16],world[12],tails[257*3];
static uint16_t bones[22];
static uint32_t attributes[12],layout[40],draw[30],a[2048],b[2048];
static FhPicaActorGeometry geometry={.attributes=attributes,.layout=layout,.draw=draw};
static FhPicaActorMaterial material;
static FhPicaActorParameters parameters={.material=&material};
static uint32_t seed=0x10203040;
static unsigned random32(void){seed=seed*1664525+1013904223;return seed;}
static float finite_value(void){return ((int)(random32()%65536)-32768)/64.f;}
void test_main(void){
 for(unsigned frame=0;frame<24;frame++){
  unsigned joint_count=frame%3==0?256:frame%3==1?24:0;
  for(unsigned i=0;i<256*16;i++)joints[i]=finite_value();
  for(unsigned i=0;i<12;i++)world[i]=finite_value();
  for(unsigned i=0;i<257*3;i++)tails[i]=finite_value();
  FhPicaActorCommandState sa={0},sb={0};
  FhPicaActorPose pose={joints,world,tails,bones,joint_count,22,0};
  for(unsigned step=0;step<24;step++){
   pose.count=1+(frame+step)%22;pose.changed=(1u<<pose.count)-1;
   if(step%3)pose.changed&=step&1?0x155555:0x2aaaaa;
   if(step==23)pose.changed=0;
   for(unsigned i=0;i<pose.count;i++)bones[i]=joint_count?random32()%(joint_count+1):0;
   if(step%4==0)bones[pose.count-1]=joint_count;
   parameters.colors[step%4]=random32();parameters.uniforms[step%12]=random32();
   unsigned start=step&6,ca=start,cb=start;
   memset(a,0xa5,sizeof a);memset(b,0xa5,sizeof b);
   measured_begin(frame<4?0:2);unsigned na=reference_append(a,2048,&ca,&sa,&geometry,&parameters,&pose,0);measured_end();
   measured_begin(frame<4?1:2);unsigned nb=fh_pica_actor_command_append(b,2048,&cb,&sb,&geometry,&parameters,&pose,0);measured_end();
   CHECK(na&&nb&&na==nb&&ca==cb&&!memcmp(a,b,sizeof a)&&!memcmp(&sa,&sb,sizeof sa));
   FhPicaActorCommandState before=sb;uint32_t first=b[start];cb=start;
   CHECK(!fh_pica_actor_command_append(b,start+1,&cb,&sb,&geometry,&parameters,&pose,0));
   CHECK(cb==start && b[start]==first && !memcmp(&before,&sb,sizeof sb));
   /* Reused pose addresses with new values must not retain previous rows. */
   joints[0]+=1;world[3]-=1;
   pose.changed=(1u<<pose.count)-1;ca=cb=start;
   na=reference_append(a,2048,&ca,&sa,&geometry,&parameters,&pose,0);
   nb=fh_pica_actor_command_append(b,2048,&cb,&sb,&geometry,&parameters,&pose,0);
   CHECK(na&&nb&&na==nb&&ca==cb&&!memcmp(a,b,sizeof a)&&!memcmp(&sa,&sb,sizeof sa));
  }
 }
 test_pass();
}
