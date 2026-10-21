#define _GNU_SOURCE
#include "fh_replay.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
void source_walk_direction(FhPadState,int,float*,float*);
static int parse(const char*s,FhReplayPlan*p){char e[128];FILE*f=fmemopen((void*)s,strlen(s),"r");assert(f);int ok=fh_replay_parse(f,p,e,sizeof(e));fclose(f);return ok;}
#define HEAD "FHREPLAY 1\nSAVE 0 0\nLIMIT 300\n"
int main(void){
    FhReplayPlan p;FhPadState pad;
    assert(parse(HEAD "WALK_TO 100 0 -200 8 40 120\nEND\n",&p));
    assert(!p.steps[0].measure);
    unsigned directions=0;
    for(int yaw=-32768;yaw<32768;yaw+=1024)for(int angle=0;angle<360;angle+=5){
        float a=angle*3.14159265358979323846f/180;
        p.steps[0].x=200*cosf(a);p.steps[0].z=200*sinf(a);
        FhReplaySnapshot s={.player=1,.sequence=-1,.ui=1,.camera_yaw=yaw},before=s;
        FhReplayCursor c={0};fh_replay_advance(&p,&c,&s,0,0,&pad);
        assert(!c.status&&!pad.buttons&&!pad.cx&&!pad.cy&&!pad.left&&!pad.right);
        assert(!memcmp(&s,&before,sizeof(s)));
        float x,z;source_walk_direction(pad,yaw,&x,&z);
        assert((x*cosf(a)+z*sinf(a))/hypotf(x,z)>.998f);directions++;
    }
    assert(parse(HEAD "WALK_TO 100 0 -200 8 40 120\nCHECK_PLAYER 100 0 -200 8\nEND\n",&p));
    for(unsigned mode=0;mode<5;mode++){
        FhReplaySnapshot s={.player=1,.sequence=-1,.ui=1,.title=1};FhReplayCursor c={0};double sim=0;unsigned n=0;
        while(!c.status&&n++<2000){
            float dt=mode==4?(n%3?1.1f:4.7f):(float)(mode+1);
            fh_replay_advance(&p,&c,&s,sim,sim/60,&pad);
            if(c.status)break;
            float x,z;source_walk_direction(pad,s.camera_yaw,&x,&z);
            s.x+=x*dt;s.z+=z*dt;
            s.camera_yaw=(int16_t)(s.camera_yaw+(int)(37*dt));sim+=dt;
        }
        assert(c.status==1&&hypotf(s.x-100,s.z+200)<=8);
    }
    for(unsigned mode=0;mode<7;mode++){
        FhReplaySnapshot s={.player=1,.sequence=-1,.ui=1};FhReplayCursor c={0};
        switch(mode){case 0:s.loading=1;break;case 1:s.sequence=3;break;case 2:s.viewfinder=1;break;
            case 3:s.player=0;break;case 4:s.ui=2;break;case 5:s.x=NAN;break;case 6:s.camera_yaw=40000;break;}
        fh_replay_advance(&p,&c,&s,0,0,&pad);assert(!c.status&&!pad.x&&!pad.y&&!pad.buttons);
        fh_replay_advance(&p,&c,&s,7201,121,&pad);assert(c.status==3&&!pad.x&&!pad.y);
    }
    /* Obstacles and wrong floor never become success through elapsed time. */
    FhReplaySnapshot s={.player=1,.sequence=-1,.ui=1};FhReplayCursor c={0};
    fh_replay_advance(&p,&c,&s,0,0,&pad);fh_replay_advance(&p,&c,&s,7201,121,&pad);assert(c.status==3);
    s.x=100;s.z=-200;s.y=100;c=(FhReplayCursor){0};
    fh_replay_advance(&p,&c,&s,0,0,&pad);assert(!c.status&&!pad.x&&!pad.y);
    const char*bad[]={"WALK_TO nan 0 0 8 40 60","WALK_TO 0 0 0 1 40 60","WALK_TO 0 0 0 8 57 60",
        "WALK_TO 0 0 0 8 23 60","WALK_TO 0 0 0 8 40 601","WALK_TO 0 0 0 8 40 0","WALK_TO 0 0 0 8 inf 60","WALK_TO 0 0 0 8 40 60 extra"};
    for(unsigned i=0;i<sizeof(bad)/sizeof(*bad);i++){char b[256];snprintf(b,sizeof(b),HEAD "%s\nEND\n",bad[i]);assert(!parse(b,&p));}
    printf("PASS %u original-heading/matrix directions; 5 changing-camera cadences; invalid state, blocked route, height, timeout and parser bounds\n",directions);
}
