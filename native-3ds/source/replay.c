/* Bounded offline/console interpreter; no game-state or timing writes. */
#include "fh_replay.h"
#include "fh_graphics_options.h"
#include <math.h>
#include <string.h>
#include <ctype.h>
static int tail(const char *s,int n){if(n<0)return 0;while(s[n]&&isspace((unsigned char)s[n]))n++;return !s[n];}
static int fail(char *error,unsigned bytes,unsigned line,const char *why){snprintf(error,bytes,"Line %u: %s",line,why);return 0;}
/* All 32 graphics bits are now assigned. Reject signed/overflowing
 * decimal input explicitly rather than relying on an unused-bit mask. */
static int graphics_mask(const char *s,unsigned skip,unsigned *out){
    s+=skip;while(isspace((unsigned char)*s))s++;
    if(*s<'0'||*s>'9')return 0;
    uint32_t value=0;
    while(*s>='0'&&*s<='9'){
        unsigned digit=(unsigned)(*s++-'0');
        if(value>(UINT32_MAX-digit)/10)return 0;
        value=value*10+digit;
    }
    if(!tail(s,0))return 0;
    *out=value;return 1;
}
int fh_replay_parse(FILE *f,FhReplayPlan *p,char *error,unsigned bytes){
    char line[256],word[32];unsigned number=0,header=0,save=0,limit=0,end=0,graphics=0,geometry=0,textures=0,effects=0;
    memset(p,0,sizeof(*p));
    p->graphics_options=FH_GRAPHICS_ALL;
    while(fgets(line,sizeof(line),f)){
        number++;if(!strchr(line,'\n')&&!feof(f))return fail(error,bytes,number,"line too long");
        const char *s=line;while(isspace((unsigned char)*s))s++;if(!*s||*s=='#')continue;
        int n=-1,ok=0;unsigned version=0;
        if(!header){if(sscanf(s,"FHREPLAY %u %n",&version,&n)!=1||version!=1||!tail(s,n))return fail(error,bytes,number,"expected FHREPLAY 1");header=1;continue;}
        if(end)return fail(error,bytes,number,"data after END");
        if(sscanf(s,"%31s",word)!=1)return fail(error,bytes,number,"missing command");
        if(!strcmp(word,"SAVE")){
            if(save++||p->count||sscanf(s,"SAVE %u %x %n",&p->save_bytes,&p->save_crc,&n)!=2||!tail(s,n)||(p->save_bytes && (p->save_bytes<8256||p->save_bytes>131136||(p->save_bytes-64)%8192))||(!p->save_bytes&&p->save_crc))return fail(error,bytes,number,"invalid SAVE size/CRC");continue;
        }
        if(!strcmp(word,"LIMIT")){
            if(limit++||p->count||sscanf(s,"LIMIT %u %n",&p->wall_limit,&n)!=1||!tail(s,n)||!p->wall_limit||p->wall_limit>7200)return fail(error,bytes,number,"invalid wall limit");continue;
        }
        if(!strcmp(word,"GEOMETRY")){
            if(geometry++||p->count||sscanf(s,"GEOMETRY %u %n",&p->original_geometry,&n)!=1||!tail(s,n)||p->original_geometry>1)return fail(error,bytes,number,"invalid geometry profile");continue;
        }
        if(!strcmp(word,"TEXTURES")){
            if(textures++||p->count||sscanf(s,"TEXTURES %u %n",&p->original_textures,&n)!=1||!tail(s,n)||p->original_textures>1)return fail(error,bytes,number,"invalid texture profile");continue;
        }
        if(!strcmp(word,"EFFECTS")){
            if(effects++||p->count||!graphics_mask(s,7,&p->graphics_effects)||(p->graphics_effects&~FH_EFFECT_VALID))return fail(error,bytes,number,"invalid extended effects");continue;
        }
        if(!strcmp(word,"GRAPHICS")){
            if(graphics++||p->count||!graphics_mask(s,8,&p->graphics_options)||(p->graphics_options&~FH_GRAPHICS_VALID))return fail(error,bytes,number,"invalid graphics options");continue;
        }
        if(!save||!limit||p->count==FH_REPLAY_STEPS)return fail(error,bytes,number,"missing header fields or too many steps");
        FhReplayStep *a=p->steps+p->count;memset(a,0,sizeof(*a));
        if(!strcmp(word,"HOLD")){
            unsigned buttons,left,right,measure;int x,y,cx,cy;
            ok=sscanf(s,"HOLD %f %x %d %d %d %d %u %u %u %n",&a->duration,&buttons,&x,&y,&cx,&cy,&left,&right,&measure,&n)==9;
            if(ok)ok=isfinite(a->duration)&&a->duration>0&&a->duration<=3600&&!(buttons&~0x1f7fu)&&x>=-127&&x<=127&&y>=-127&&y<=127&&cx>=-127&&cx<=127&&cy>=-127&&cy<=127&&left<=255&&right<=255&&measure<=1;
            if(ok){a->kind=FH_R_HOLD;a->measure=measure;a->pad=(FhPadState){.buttons=buttons,.x=x,.y=y,.cx=cx,.cy=cy,.left=left,.right=right,.analog_a=buttons&0x100?255:0,.analog_b=buttons&0x200?255:0};}
        }else if(!strcmp(word,"WAIT_TITLE")){a->kind=FH_R_TITLE;ok=sscanf(s,"WAIT_TITLE %f %n",&a->timeout,&n)==1;
        }else if(!strcmp(word,"WAIT_UI")){a->kind=FH_R_UI;ok=sscanf(s,"WAIT_UI %d %f %n",&a->ui,&a->timeout,&n)==2&&a->ui>=0&&a->ui<4096;
        }else if(!strcmp(word,"WAIT_PLAYER")){a->kind=FH_R_PLAYER;ok=sscanf(s,"WAIT_PLAYER %f %f %f %f %f %n",&a->x,&a->y,&a->z,&a->radius,&a->timeout,&n)==5;
        }else if(!strcmp(word,"LOAD_PLAYER")){a->kind=FH_R_LOAD;ok=sscanf(s,"LOAD_PLAYER %f %f %f %f %f %n",&a->x,&a->y,&a->z,&a->radius,&a->timeout,&n)==5;
        }else if(!strcmp(word,"CHECK_PLAYER")){a->kind=FH_R_CHECK;ok=sscanf(s,"CHECK_PLAYER %f %f %f %f %n",&a->x,&a->y,&a->z,&a->radius,&n)==4;
        }else if(!strcmp(word,"WALK_TO")){a->kind=FH_R_WALK;ok=sscanf(s,"WALK_TO %f %f %f %f %f %f %n",&a->x,&a->y,&a->z,&a->radius,&a->duration,&a->timeout,&n)==6;
        }else if(!strcmp(word,"AIM_VIEW")){a->kind=FH_R_AIM;ok=sscanf(s,"AIM_VIEW %f %f %f %f %f %n",&a->x,&a->y,&a->radius,&a->duration,&a->timeout,&n)==5;
        }else if(!strcmp(word,"CHECK_VIEW")){a->kind=FH_R_VIEW;ok=sscanf(s,"CHECK_VIEW %f %f %f %n",&a->x,&a->y,&a->radius,&n)==3;
        }else if(!strcmp(word,"PLAY_RECORDING")){a->kind=FH_R_TAPE;a->measure=1;ok=1;n=14;
        }else if(!strcmp(word,"END")){a->kind=FH_R_END;ok=1;n=3;end=1;}
        if(a->kind==FH_R_PLAYER||a->kind==FH_R_CHECK||a->kind==FH_R_LOAD||a->kind==FH_R_WALK)ok=ok&&isfinite(a->x)&&isfinite(a->y)&&isfinite(a->z)&&fabsf(a->x)<1e6f&&fabsf(a->y)<1e6f&&fabsf(a->z)<1e6f&&isfinite(a->radius)&&a->radius>0&&a->radius<=10000;
        if(a->kind==FH_R_TITLE||a->kind==FH_R_UI||a->kind==FH_R_PLAYER||a->kind==FH_R_LOAD||a->kind==FH_R_WALK)ok=ok&&isfinite(a->timeout)&&a->timeout>0&&a->timeout<=600;
        if(a->kind==FH_R_WALK)ok=ok&&a->radius>=5&&a->radius<=100&&isfinite(a->duration)&&a->duration>=24&&a->duration<=56;
        if(a->kind==FH_R_AIM||a->kind==FH_R_VIEW)ok=ok&&isfinite(a->x)&&a->x>=-32768&&a->x<=32767&&isfinite(a->y)&&fabsf(a->y)<=8960&&isfinite(a->radius)&&a->radius>=128&&a->radius<=1024;
        if(a->kind==FH_R_AIM)ok=ok&&isfinite(a->duration)&&a->duration>=1&&a->duration<=600&&isfinite(a->timeout)&&a->timeout>0&&a->timeout<=120;
        if(!ok||!tail(s,n))return fail(error,bytes,number,"invalid step or bounds");p->count++;
    }
    if(ferror(f)||!header||!save||!limit||!end||p->count<2)return fail(error,bytes,number,"incomplete recipe");
    for(unsigned i=0;i<p->count;i++)if(p->steps[i].kind==FH_R_TAPE){
        if(!((p->count==2&&i==0&&p->steps[1].kind==FH_R_END)||
             (p->count==3&&i==1&&p->steps[0].kind==FH_R_TITLE&&p->steps[2].kind==FH_R_END)))
            return fail(error,bytes,number,"recording requires WAIT_TITLE / PLAY_RECORDING / END");
    }
    snprintf(error,bytes,"Validated %u steps",p->count);return 1;
}
static int player_at(const FhReplayStep*a,const FhReplaySnapshot*s){
    double x=s->x-a->x,y=s->y-a->y,z=s->z-a->z;
    return s->player&&!s->loading&&s->sequence<0&&isfinite(x)&&isfinite(y)&&isfinite(z)&&x*x+y*y+z*z<=(double)a->radius*a->radius;
}
static float yaw_error(float target,int current){
    float e=target-current;if(e>32768)e-=65536;if(e<-32768)e+=65536;return e;
}
/* Inverse of the axis deadzone/scaling in PADClamp; diagonal clipping is still
 * performed by the normal input path. No camera, player or timing writes. */
static int raw_axis(float value){
    int n=(int)roundf(value);if(n>72)n=72;if(n<-72)n=-72;
    int magnitude=n<0?-n:n;
    int raw=magnitude?(magnitude*112+71)/72+15:0;
    return n<0?-raw:raw;
}
static int view_ready(const FhReplaySnapshot*s){
    return s->player&&!s->loading&&s->sequence<0&&s->viewfinder&&
        s->camera_yaw>=-32768&&s->camera_yaw<=32767&&
        s->camera_pitch>=-15360&&s->camera_pitch<=15360&&isfinite(s->yaw_speed);
}
static int view_at(const FhReplayStep*a,const FhReplaySnapshot*s){
    return view_ready(s)&&fabsf(yaw_error(a->x,s->camera_yaw))<=a->radius&&
        fabsf(a->y-s->camera_pitch)<=a->radius&&fabsf(s->yaw_speed)<5;
}
void fh_replay_advance(const FhReplayPlan*p,FhReplayCursor*c,const FhReplaySnapshot*s,double sim,double wall,FhPadState*out){
    memset(out,0,sizeof(*out));if(c->status)return;
    if(!isfinite(sim)||!isfinite(wall)||sim<0||wall<0){c->status=4;return;}
    if(wall>p->wall_limit){c->status=3;return;}
    for(unsigned guard=0;guard<p->count;guard++){
        if(c->step>=p->count){c->status=2;return;}
        const FhReplayStep*a=p->steps+c->step;
        if(!c->started){c->started=1;c->emitted=0;c->pulse=0;c->pulse_wall=wall;c->start_sim=sim;c->start_wall=wall;c->aligned_since=-1;}
        if(sim<c->start_sim||wall<c->start_wall){c->status=4;return;}
        if(a->kind==FH_R_END){c->status=1;return;}
        if(a->kind==FH_R_TAPE)return; /* Owned streaming player advances this step. */
        if(a->kind==FH_R_HOLD){
            double elapsed=sim-c->start_sim;
            if(!c->emitted||elapsed<a->duration){*out=a->pad;c->emitted=1;return;}
            double late=elapsed-a->duration;if(late>c->max_lateness)c->max_lateness=late;
        }else if(a->kind==FH_R_WALK){
            /* Test-route approach only, never a measured benchmark window.
             * Original player inputHeading=atan2(stickX,-stickY)-cameraYaw;
             * forward root motion is transformed local -Z. Invert that map
             * using the observed camera, without writing game state/clocks.
             * Waypoints must avoid obstacles; a blocked route times out. */
            if(wall-c->start_wall>a->timeout){c->status=3;return;}
            /* The attract-mode ready flag can remain set in gameplay. Use the
             * live player/UI/loading state, as the existing view controller does. */
            if(!s->player||s->loading||s->sequence>=0||s->viewfinder||s->ui!=1||
               !isfinite(s->x)||!isfinite(s->y)||!isfinite(s->z)||
               s->camera_yaw<-32768||s->camera_yaw>32767)return;
            if(!player_at(a,s)){
                float x=a->x-s->x,z=a->z-s->z,d=hypotf(x,z);
                /* No walking can repair an incorrect floor/height assertion. */
                if(d<1.f)return;
                float yaw=s->camera_yaw*(3.14159265358979323846f/32768.f);
                float sn=sinf(yaw),cs=cosf(yaw);
                /* Keep the requested stick strength until arrival. Smaller
                 * input is not proportional movement: animation/slope response
                 * can stop the player outside the waypoint's 3D radius. */
                float strength=a->duration/d;
                out->x=raw_axis((-x*cs-z*sn)*strength);
                out->y=raw_axis((z*cs-x*sn)*strength);
                return;
            }
        }else if(a->kind==FH_R_AIM){
            if(wall-c->start_wall>a->timeout){c->status=3;return;}
            if(!view_ready(s)){c->aligned_since=-1;return;}
            float error=yaw_error(a->x,s->camera_yaw);
            float speed=fabsf(error)<=a->radius*.5f?0:error/24.f;
            out->x=raw_axis((speed-.5f*s->yaw_speed)/6.f);
            /* Turn first, then tilt: otherwise the pad's diagonal clamp changes
             * both axes while making a large turn. Keep pitch held thereafter. */
            if(fabsf(error)<1024&&fabsf(s->yaw_speed)<30)out->y=raw_axis(a->y/128.f);
            if(!view_at(a,s))c->aligned_since=-1;
            else if(c->aligned_since<0)c->aligned_since=sim;
            if(c->aligned_since<0||sim-c->aligned_since<a->duration)return;
            memset(out,0,sizeof(*out));
        }else if(a->kind==FH_R_VIEW){
            if(!view_at(a,s)){c->status=2;return;}
        }else{
            int matches=a->kind==FH_R_TITLE?s->title:a->kind==FH_R_UI?s->ui==a->ui&&!s->loading:player_at(a,s);
            if(!matches){
                if(a->kind==FH_R_CHECK||(a->kind==FH_R_LOAD&&s->player&&!s->loading&&s->sequence<0))c->status=2;
                else if(wall-c->start_wall>a->timeout)c->status=3;
                else if(a->kind==FH_R_LOAD&&!s->player&&!s->loading){
                    /* Real menu inputs only; stop as soon as a player exists.
                     * Keep each press/release for at least one observation. */
                    if(c->emitted&&wall-c->pulse_wall>=.3){c->pulse=(c->pulse+1)%4;c->pulse_wall=wall;}
                    c->emitted=1;out->buttons=c->pulse==0?0x1000:c->pulse==2?0x100:0;
                    out->analog_a=out->buttons&0x100?255:0;
                }
                return;
            }
        }
        c->step++;c->started=0;
    }
    c->status=2;
}
