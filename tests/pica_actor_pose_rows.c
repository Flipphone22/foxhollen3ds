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
static FhPicaActorPoseRows rows,saved_rows;
static FhPicaActorGeometry geometry={.attributes=attributes,.layout=layout,.draw=draw};
static FhPicaActorMaterial material;
static FhPicaActorParameters parameters={.material=&material};
static uint32_t random_state=0x10203040;
static float finite_value(void){random_state=random_state*1664525+1013904223;return ((int)(random_state%65536)-32768)/64.f;}
void test_main(void){
    CHECK(__builtin_offsetof(FhPicaActorPose,prepared_rows)==28);
    for(unsigned frame=0;frame<24;frame++){
        unsigned joint_count=frame%3==0?256:frame%3==1?24:0;
        for(unsigned i=0;i<256*16;i++)joints[i]=finite_value();
        for(unsigned i=0;i<12;i++)world[i]=finite_value();
        for(unsigned i=0;i<257*3;i++)tails[i]=finite_value();
        FhPicaActorCommandState sa={0},sb={0};
        FhPicaActorPose pose={joints,world,tails,bones,joint_count,22,0,NULL};
        memset(&rows,0x55,sizeof rows);
        measured_begin(1);int initialized=fh_pica_actor_pose_rows_begin(&rows,joints,world,tails,joint_count);measured_end();
        CHECK(initialized);
        for(unsigned step=0;step<12;step++){
            pose.count=1+(frame+step)%22;
            pose.changed=(1u<<pose.count)-1;
            if(step%3)pose.changed&=0x155555;
            for(unsigned i=0;i<pose.count;i++)bones[i]=joint_count?(i*7+step%3)%(joint_count+1):0;
            if(step==11)bones[pose.count-1]=joint_count; /* world slot, including 256 */
            parameters.colors[step%4]=frame*1000+step;
            parameters.uniforms[step%12]=0x3f800000+step;
            unsigned ca=0,cb=0;pose.prepared_rows=NULL;
            measured_begin(0);unsigned na=reference_append(a,2048,&ca,&sa,&geometry,&parameters,&pose,0);measured_end();
            pose.prepared_rows=&rows;
            measured_begin(1);unsigned nb=fh_pica_actor_command_append(b,2048,&cb,&sb,&geometry,&parameters,&pose,0);measured_end();
            CHECK(na&&nb&&na==nb&&ca==cb&&!memcmp(a,b,ca*4)&&!memcmp(&sa,&sb,sizeof sa));
            saved_rows=rows;FhPicaActorCommandState before=sb;uint32_t first=b[0];cb=0;
            CHECK(!fh_pica_actor_command_append(b,1,&cb,&sb,&geometry,&parameters,&pose,0));
            CHECK(!cb&&b[0]==first&&!memcmp(&rows,&saved_rows,sizeof rows)&&!memcmp(&before,&sb,sizeof sb));
            pose.joints=joints+16;
            CHECK(!fh_pica_actor_command_append(b,2048,&cb,&sb,&geometry,&parameters,&pose,0));
            CHECK(!cb&&b[0]==first&&!memcmp(&rows,&saved_rows,sizeof rows)&&!memcmp(&before,&sb,sizeof sb));
            pose.joints=joints;
        }
        hits+=rows.hits;misses+=rows.misses;
    }
    CHECK(hits>misses);test_pass();
}
