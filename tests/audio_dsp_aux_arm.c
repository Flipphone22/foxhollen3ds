#include "fh_audio_dsp_aux.h"
#include <3ds.h>
#include <string.h>
extern void *ndspVars[16][2];
extern u16 ndspBufferId,ndspBufferCurId,ndspFrameId;
void LightLock_Lock(LightLock *p){(void)p;}
void LightLock_Unlock(LightLock *p){(void)p;}
volatile unsigned checks,failure,finished;
const volatile unsigned owner_bytes=sizeof(FhAudioDspAux);
__attribute__((noinline,noreturn)) void done(void){for(;;){}}
#define CHECK(x) do{checks++;if(!(x)){failure=__LINE__;done();}}while(0)
static struct {unsigned before[8];int32_t samples[2][4][160];unsigned after[8];} memory[2];
static int32_t original[2][2][4][160];
static FhAudioDspAux owner;
static unsigned callbacks[2];
static uint32_t returned[2];
static void tagged(void *context,int32_t samples[4][160],uint32_t token) {
    (void)samples;returned[(uintptr_t)context]=token;
}
static void step(unsigned frame,uint32_t publish,uint32_t expected) {
    ndspFrameId=frame;ndspBufferId=ndspBufferCurId=frame&1;
    if(publish)fh_audio_dsp_aux_tag(&owner,publish);
    returned[0]=returned[1]=~0u;
    fh_audio_dsp_aux_service();
    CHECK(returned[0]==expected && returned[1]==expected);
}
static void callback(void *p,int32_t samples[4][160]) {
    unsigned bus=(uintptr_t)p;callbacks[bus]++;
    for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)samples[c][i]^=(int32_t)(0x502030+bus*16+c);
}
void run(void) {
    CHECK(owner_bytes==sizeof(owner));
    CHECK(!fh_audio_dsp_aux_install(NULL));CHECK(!fh_audio_dsp_aux_install(&owner));
    owner.process[0]=owner.process[1]=callback;owner.context[1]=(void *)1;
    CHECK(fh_audio_dsp_aux_install(&owner));CHECK(!fh_audio_dsp_aux_install(&owner));
    unsigned expected[2]={0};
    for(unsigned test=0;test<64;test++) {
        unsigned read=test&1,write=(test>>1)&1,mask=(test>>2)&3;
        for(unsigned region=0;region<2;region++) {
            memset(&memory[region],0xa5,sizeof(memory[region]));ndspVars[7][region]=memory[region].samples;
            for(unsigned bus=0;bus<2;bus++)for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++)
                original[region][bus][c][i]=memory[region].samples[bus][c][i]=(int32_t)(region*50000+bus*7000+c*500+i+test*7);
        }
        ndspBufferId=read;ndspBufferCurId=write;
        for(unsigned bus=0;bus<2;bus++)ndspAuxSetEnable(bus,(mask&(1<<bus))!=0);
        fh_audio_dsp_aux_service();
        for(unsigned bus=0;bus<2;bus++) {
            if(mask&(1<<bus))expected[bus]++;
            CHECK(callbacks[bus]==expected[bus] && owner.calls[bus]==expected[bus]);
        }
        for(unsigned region=0;region<2;region++) {
            for(unsigned i=0;i<8;i++)CHECK(memory[region].before[i]==0xa5a5a5a5 && memory[region].after[i]==0xa5a5a5a5);
            for(unsigned bus=0;bus<2;bus++)for(unsigned c=0;c<4;c++)for(unsigned i=0;i<160;i++) {
                int32_t wanted=original[region][bus][c][i];
                if(region==write && (mask&(1<<bus)))wanted=original[read][bus][c][i]^(int32_t)(0x502030+bus*16+c);
                CHECK(memory[region].samples[bus][c][i]==wanted);
            }
        }
    }
    CHECK(!owner.faults);ndspBufferId=2;fh_audio_dsp_aux_service();CHECK(owner.faults==1);
    ndspBufferId=0;ndspBufferCurId=2;fh_audio_dsp_aux_service();CHECK(owner.faults==2);
    ndspBufferCurId=0;ndspVars[7][0]=NULL;fh_audio_dsp_aux_service();CHECK(owner.faults==3);
    fh_audio_dsp_aux_uninstall();fh_audio_dsp_aux_service();CHECK(owner.faults==3);
    memset(&owner,0,sizeof(owner));
    owner.tagged_process[0]=owner.tagged_process[1]=tagged;owner.context[1]=(void *)1;
    ndspVars[7][0]=memory[0].samples;ndspVars[7][1]=memory[1].samples;
    ndspAuxSetEnable(0,true);ndspAuxSetEnable(1,true);
    CHECK(fh_audio_dsp_aux_install(&owner));
    for(unsigned i=0;i<200;i++)step(100+i,1000+i,i<2?0:998+i);
    /* A skipped service whose bank still contains a two-frame-old tag must
     * not silently accept it. Reestablish two publications after the gap. */
    step(301,2000,0);step(302,2001,0);step(303,2002,2000);
    step(304,0,2001);step(305,2004,2002);step(306,2005,0);
    step(306,3000,0);step(307,3001,0);step(308,3002,3000);
    /* SDK counters skip zero/one at wrap. */
    step(65534,4000,0);step(65535,4001,0);step(2,4002,4000);step(3,4003,4001);
    step(0,5000,0);step(1,5001,0);step(2,5002,0);step(3,5003,0);step(4,5004,5002);
    ndspBufferId=2;ndspBufferCurId=0;ndspFrameId=5;
    fh_audio_dsp_aux_tag(&owner,6000);fh_audio_dsp_aux_service();CHECK(owner.faults==1);
    step(6,6001,0);step(7,6002,0);step(8,6003,6001);
    CHECK(owner.next_token==0);
    ndspAuxSetEnable(1,false);
    ndspFrameId=9;ndspBufferId=ndspBufferCurId=1;
    fh_audio_dsp_aux_tag(&owner,7000);returned[1]=~0u;fh_audio_dsp_aux_service();
    CHECK(returned[0]==6002 && returned[1]==~0u);
    ndspAuxSetEnable(1,true);step(10,7001,6003);
    ndspFrameId=11;ndspBufferId=ndspBufferCurId=1;
    fh_audio_dsp_aux_tag(&owner,7002);fh_audio_dsp_aux_service();
    CHECK(returned[0]==7000 && returned[1]==0);
    step(12,7003,7001);
    fh_audio_dsp_aux_uninstall();
    CHECK(fh_audio_dsp_aux_install(&owner));step(13,8000,0);step(14,8001,0);step(15,8002,8000);
    fh_audio_dsp_aux_uninstall();
    finished=1;done();
}
