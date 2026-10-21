#define _GNU_SOURCE
#include "fh_replay.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
static int parse(const char*s,FhReplayPlan*p){char error[128];FILE*f=fmemopen((void*)s,strlen(s),"r");assert(f);int ok=fh_replay_parse(f,p,error,sizeof(error));fclose(f);return ok;}
#define HEAD "FHREPLAY 1\nSAVE 24640 12345678\nLIMIT 300\n"
/* Independent plant: PADClamp's octagonal range and the game's 60-degree
 * viewfinder response (engine/68). This does not replace integrated playback. */
static void view_step(FhReplaySnapshot*s,FhPadState pad,float dt){
    int x=abs(pad.x),y=abs(pad.y);
    x=x<=15?0:(x-15)*72/112;y=y<=15?0:(y-15)*72/112;
    int d=x>=y?40*x+32*y:40*y+32*x;
    if(d>2880){x=2880*x/d;y=2880*y/d;}
    if(pad.x<0)x=-x;if(pad.y<0)y=-y;
    s->yaw_speed+=(6*x-s->yaw_speed)*(1-powf(.88f,dt));
    if(fabsf(s->yaw_speed)<5)s->yaw_speed=0;
    int yaw=(int)(s->camera_yaw+s->yaw_speed*dt);
    while(yaw>32767)yaw-=65536;while(yaw<-32768)yaw+=65536;
    s->camera_yaw=yaw;
    s->camera_pitch=(int)(s->camera_pitch+(128*y-s->camera_pitch)*(1-powf(.9375f,dt)));
}
static void test_view(void){
    FhReplayPlan p;FhPadState pad;
    assert(parse(HEAD "AIM_VIEW 0 -8192 256 60 100\nCHECK_VIEW 0 -8192 256\nEND\n",&p));
    for(int mode=0;mode<5;mode++){
        FhReplayCursor c={0};FhReplaySnapshot s={.player=1,.sequence=-1,.viewfinder=1,.camera_yaw=-32500};
        double sim=0;unsigned count=0;
        while(!c.status&&count++<2000){
            float dt=mode==4?(count%3==0?4.7f:1.2f):(float)(1<<mode);
            fh_replay_advance(&p,&c,&s,sim,sim/60,&pad);
            assert(!pad.buttons&&!pad.cx&&!pad.cy&&!pad.left&&!pad.right);
            if(c.status)break;
            view_step(&s,pad,dt);sim+=dt;
        }
        assert(c.status==1&&abs(s.camera_yaw)<=256&&abs(s.camera_pitch+8192)<=256);
    }
    FhReplayCursor c={0};FhReplaySnapshot s={.player=1,.sequence=-1};
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(!pad.x&&!pad.y&&!c.status);
    fh_replay_advance(&p,&c,&s,1,101,&pad);assert(c.status==3);
    c=(FhReplayCursor){0};s.viewfinder=1;s.camera_yaw=0;s.camera_pitch=-8192;
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(!c.status);
    s.sequence=7;fh_replay_advance(&p,&c,&s,50,1,&pad);assert(!pad.x&&!pad.y&&!c.status);
    s.sequence=-1;fh_replay_advance(&p,&c,&s,60,2,&pad);assert(!c.status);
    fh_replay_advance(&p,&c,&s,119,3,&pad);assert(!c.status);
    fh_replay_advance(&p,&c,&s,120,4,&pad);assert(c.status==1);
    assert(parse(HEAD "CHECK_VIEW 32760 0 256\nEND\n",&p));
    c=(FhReplayCursor){0};s.camera_yaw=-32760;s.camera_pitch=0;
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(c.status==1);
    c=(FhReplayCursor){0};s.yaw_speed=100;
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(c.status==2);
    c=(FhReplayCursor){0};s.yaw_speed=NAN;
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(c.status==2);
    const char*bad[]={HEAD "AIM_VIEW 0 -8192 1 60 60\nEND\n",HEAD "AIM_VIEW 0 nan 256 60 60\nEND\n",HEAD "AIM_VIEW 0 -10000 256 60 60\nEND\n",HEAD "AIM_VIEW 40000 0 256 60 60\nEND\n",HEAD "AIM_VIEW 0 0 256 0 60\nEND\n",HEAD "AIM_VIEW 0 0 256 60 121\nEND\n",HEAD "CHECK_VIEW 0 0 256 extra\nEND\n"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)assert(!parse(bad[i],&p));
    puts("View replay: 60/30/15/7.5 FPS and uneven response, cutscene gating, continuous settle, wrap, drift, invalid state and deadlines passed");
}
int main(void){
    test_view();
    FhReplayPlan p;FhReplaySnapshot s={.player=1,.sequence=-1,.x=1,.y=2,.z=3};FhPadState pad;
    assert(parse("FHREPLAY 1\nSAVE 0 0\nLIMIT 7200\nPLAY_RECORDING\nEND\n",&p));
    assert(!parse("FHREPLAY 1\nSAVE 0 0\nLIMIT 7201\nPLAY_RECORDING\nEND\n",&p));
    const char *script=HEAD "WAIT_PLAYER 1 2 3 0.1 30\nHOLD 60 100 50 0 0 0 0 0 1\nHOLD 60 0 0 0 0 0 0 0 1\nCHECK_PLAYER 1 2 3 0.1\nEND\n";
    assert(parse(script,&p));
    for(unsigned delta=1;delta<=4;delta*=2){
        FhReplayCursor c={0};unsigned pressed=0,neutral=0;
        for(unsigned t=0;t<=120;t+=delta){
            fh_replay_advance(&p,&c,&s,t,t/60.,&pad);
            if(c.status)break;
            if(pad.buttons==0x100)pressed+=delta;else neutral+=delta;
        }
        assert(c.status==1&&pressed==60&&neutral==60&&c.max_lateness==0);
    }
    const char *bad[]={"",HEAD "END\n",HEAD "HOLD nan 0 0 0 0 0 0 0 1\nEND\n",HEAD "HOLD 1 0 128 0 0 0 0 0 0\nEND\n",HEAD "WAIT_PLAYER 0 0 0 -1 30\nEND\n",HEAD "HOLD 1 2000 0 0 0 0 0 0 0\nEND\n",HEAD "WAIT_TITLE 601\nEND\n",HEAD "WAIT_TITLE 30 junk\nEND\n",HEAD "WAIT_TITLE 30\nEND\nHOLD 2 0 0 0 0 0 0 0 0\n"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++)assert(!parse(bad[i],&p));
    assert(parse(HEAD "HOLD 1 100 0 0 0 0 0 0 0\nHOLD 1 0 0 0 0 0 0 0 0\nHOLD 1 200 0 0 0 0 0 0 0\nEND\n",&p));
    FhReplayCursor c={0};unsigned expected[]={0x100,0,0x200};
    for(unsigned i=0;i<3;i++){fh_replay_advance(&p,&c,&s,i*120,i*2,&pad);assert(!c.status&&pad.buttons==expected[i]);}
    assert(c.max_lateness==119); /* Never silently drop an edge after a slow frame. */
    assert(parse(script,&p));c=(FhReplayCursor){0};s.sequence=5;
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(!c.status&&c.step==0&&!pad.buttons);
    s.sequence=-1;fh_replay_advance(&p,&c,&s,0,1,&pad);assert(c.step==1&&pad.buttons==0x100);
    s.x=99;fh_replay_advance(&p,&c,&s,60,2,&pad);fh_replay_advance(&p,&c,&s,120,3,&pad);assert(c.status==2);
    c=(FhReplayCursor){0};fh_replay_advance(&p,&c,&s,0,0,&pad);fh_replay_advance(&p,&c,&s,0,31,&pad);assert(c.status==3);
    c=(FhReplayCursor){0};fh_replay_advance(&p,&c,&s,NAN,0,&pad);assert(c.status==4);
    c=(FhReplayCursor){0};fh_replay_advance(&p,&c,&s,0,301,&pad);assert(c.status==3);
    assert(parse(HEAD "LOAD_PLAYER 1 2 3 1 30\nHOLD 60 0 0 0 0 0 0 0 1\nEND\n",&p));
    c=(FhReplayCursor){0};s=(FhReplaySnapshot){.sequence=-1};
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(pad.buttons==0x1000);
    fh_replay_advance(&p,&c,&s,1,.5,&pad);assert(!pad.buttons);
    fh_replay_advance(&p,&c,&s,2,1,&pad);assert(pad.buttons==0x100);
    s.player=1;s.sequence=2;
    fh_replay_advance(&p,&c,&s,3,1.5,&pad);assert(!pad.buttons&&!c.status);
    s.sequence=-1;s.x=1;s.y=2;s.z=3;
    fh_replay_advance(&p,&c,&s,4,2,&pad);assert(!pad.buttons&&c.step==1);
    puts("Replay: 60/30/15 FPS durations, malformed recipes, pulse preservation, cutscene gates, divergence, deadlines and invalid clocks passed");
}
