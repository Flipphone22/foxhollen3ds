/* SPSC queue and explicit little-endian wire format, shared by both models. */
#include "fh_input_tape.h"
#include <math.h>
#include <string.h>
static unsigned get32(const unsigned char*p){return (unsigned)p[0]|(unsigned)p[1]<<8|(unsigned)p[2]<<16|(unsigned)p[3]<<24;}
static void put32(unsigned char*p,unsigned n){for(unsigned i=0;i<4;i++)p[i]=(unsigned char)(n>>(i*8));}
static int valid(const FhTapeSample*s){
    const FhPadState*p=&s->pad;
    if(s->phase==FH_TAPE_TIME)return isfinite(s->tick)&&s->tick>=0&&s->tick<=216000&&
        !s->flags&&!s->delta&&s->time.site>=1&&s->time.site<=2&&!s->time.reserved;
    if(s->phase==FH_TAPE_IO)return isfinite(s->tick)&&s->tick>=0&&s->tick<=216000&&
        !s->flags&&!s->delta&&s->io.file&&s->io.file<=8192&&s->io.offset<=0x7fffffff&&
        s->io.bytes<=0x7fffffff&&(s->io.result==(int)s->io.bytes||s->io.result==-1||s->io.result==-6);
    return isfinite(s->tick)&&s->tick>=0&&s->tick<=216000 &&
        !(p->buttons&~0x1f7f)&&p->x!=-128&&p->y!=-128&&p->cx!=-128&&p->cy!=-128&&
        !p->error&&!p->reserved&&!p->extended && isfinite(s->x)&&isfinite(s->y)&&isfinite(s->z)&&
        fabsf(s->x)<=1e6f&&fabsf(s->y)<=1e6f&&fabsf(s->z)<=1e6f&&s->player<=1&&s->loading<=1&&
        (s->flags==0||s->flags==FH_TAPE_CHECK||s->flags==FH_TAPE_END) &&
        s->phase<=FH_TAPE_ANCHOR && isfinite(s->delta)&&s->delta>=0&&s->delta<=6 &&
        (s->phase==FH_TAPE_CLOCK||s->delta==0);
}
int fh_tape_push(FhTapeRing*r,const FhTapeSample*s){
    unsigned head=__atomic_load_n(&r->head,__ATOMIC_RELAXED),tail=__atomic_load_n(&r->tail,__ATOMIC_ACQUIRE);
    if(head-tail==FH_TAPE_RING)return 0;
    r->sample[head%FH_TAPE_RING]=*s;
    unsigned used=head-tail+1;if(used>r->peak)r->peak=used;
    __atomic_store_n(&r->head,head+1,__ATOMIC_RELEASE);return 1;
}
int fh_tape_peek(FhTapeRing*r,FhTapeSample*s){
    unsigned tail=__atomic_load_n(&r->tail,__ATOMIC_RELAXED);
    if(tail==__atomic_load_n(&r->head,__ATOMIC_ACQUIRE))return 0;
    *s=r->sample[tail%FH_TAPE_RING];return 1;
}
void fh_tape_pop(FhTapeRing*r){__atomic_add_fetch(&r->tail,1,__ATOMIC_RELEASE);}
int fh_tape_encode(unsigned char*b,const FhTapeSample*s){
    if(!valid(s))return 0;
    memset(b,0,FH_TAPE_WIRE);uint64_t tick;memcpy(&tick,&s->tick,8);put32(b,(unsigned)tick);put32(b+4,(unsigned)(tick>>32));
    if(s->phase==FH_TAPE_IO||s->phase==FH_TAPE_TIME){put32(b+8,s->io.file);put32(b+12,s->io.offset);put32(b+16,s->io.bytes);put32(b+20,(unsigned)s->io.result);put32(b+52,s->phase);return 1;}
    put32(b+8,s->pad.buttons);b[12]=s->pad.x;b[13]=s->pad.y;b[14]=s->pad.cx;b[15]=s->pad.cy;
    b[16]=s->pad.left;b[17]=s->pad.right;b[18]=s->pad.analog_a;b[19]=s->pad.analog_b;
    const float values[]={s->x,s->y,s->z};for(unsigned i=0;i<3;i++){unsigned v;memcpy(&v,values+i,4);put32(b+20+4*i,v);}
    put32(b+32,(unsigned)s->ui);put32(b+36,(unsigned)s->sequence);put32(b+40,s->player);put32(b+44,s->loading);put32(b+48,s->flags);
    put32(b+52,s->phase);unsigned delta;memcpy(&delta,&s->delta,4);put32(b+56,delta);return 1;
}
int fh_tape_decode(FhTapeSample*s,const unsigned char*b){
    memset(s,0,sizeof(*s));uint64_t tick=get32(b)|((uint64_t)get32(b+4)<<32);memcpy(&s->tick,&tick,8);
    if(get32(b+52)==FH_TAPE_IO||get32(b+52)==FH_TAPE_TIME){
        for(unsigned i=24;i<64;i++)if((i<52||i>=56)&&b[i])return 0;
        s->phase=get32(b+52);s->io=(FhTapeIo){get32(b+8),get32(b+12),get32(b+16),(int)get32(b+20)};return valid(s);
    }
    unsigned buttons=get32(b+8);if(buttons>65535)return 0;
    s->pad=(FhPadState){.buttons=buttons,.x=(int8_t)b[12],.y=(int8_t)b[13],.cx=(int8_t)b[14],.cy=(int8_t)b[15],.left=b[16],.right=b[17],.analog_a=b[18],.analog_b=b[19]};
    float*values[]={&s->x,&s->y,&s->z};for(unsigned i=0;i<3;i++){unsigned v=get32(b+20+4*i);memcpy(values[i],&v,4);}
    s->ui=(int)get32(b+32);s->sequence=(int)get32(b+36);s->player=get32(b+40);s->loading=get32(b+44);s->flags=get32(b+48);
    s->phase=get32(b+52);unsigned delta=get32(b+56);memcpy(&s->delta,&delta,4);
    if(s->flags!=FH_TAPE_END&&get32(b+60))return 0;
    return valid(s);
}
static int menu_idle(const FhPadState*p){
    return !p->buttons&&!p->left&&!p->right&&!p->analog_a&&!p->analog_b &&
        p->x>=-8&&p->x<=8&&p->y>=-8&&p->y<=8&&p->cx>=-8&&p->cx<=8&&p->cy>=-8&&p->cy<=8;
}
/* Version 3 consumes the recorded API call order, including zero-duration
 * frames and repeated PAD polls. Host frame rate never selects or drops an
 * input packet. This preserves per-update menu/physics work; wall timings
 * remain measured separately and are not a claim of realtime determinism. */
int fh_tape_ordered(FhTapeRing*r,FhTapeCursor*c,unsigned phase,const FhReplaySnapshot*s,FhPadState*out,float*delta){
    if(out)memset(out,0,sizeof(*out));
    if(c->status)return 0;
    if(__atomic_load_n(&r->error,__ATOMIC_ACQUIRE)){c->status=8;return 0;}
    FhTapeSample a;
    for(unsigned pass=0;pass<2;pass++){
        if(!fh_tape_peek(r,&a)){c->status=7;return 0;}
        if(a.flags==FH_TAPE_END){fh_tape_pop(r);c->status=1;return 0;}
        /* Repeated reads while waiting for IO do not advance simulation.
         * They observe the same published controller state. Poll frequency
         * is host/SD dependent, so it must not consume another game step. */
        if(c->started&&phase==FH_TAPE_PAD&&((a.phase==FH_TAPE_IO&&c->pad_count)||((a.phase==FH_TAPE_CLOCK||a.phase==FH_TAPE_TIME)&&c->pad_valid))){*out=c->pad;c->pad_reuses++;return 1;}
        if((!c->started && (a.phase!=FH_TAPE_ANCHOR||a.tick!=0)) ||
           (c->started && a.phase!=phase))goto order_error;
        if(a.phase==FH_TAPE_ANCHOR || a.phase==FH_TAPE_PAD){
            if(a.tick!=c->ordered_tick)goto order_error;
            if(a.flags&FH_TAPE_CHECK){
                float x=s->x-a.x,y=s->y-a.y,z=s->z-a.z;
                if(s->ui!=a.ui||s->player!=a.player||s->loading!=a.loading||
                   (a.player&&!a.loading&&(s->sequence!=a.sequence||x*x+y*y+z*z>160.f*160.f))){
                    c->status=2;goto failure;
                }
            }
        }else if(a.phase==FH_TAPE_CLOCK){
            if(a.tick!=c->ordered_tick+(double)a.delta)goto order_error;
        }else goto order_error;
        fh_tape_pop(r);
        if(!c->started){c->started=1;if(phase==FH_TAPE_ANCHOR)return 1;continue;}
        if(phase==FH_TAPE_CLOCK){c->ordered_tick=a.tick;*delta=a.delta;c->pad_valid=0;c->clock_count++;}
        else {*out=a.pad;c->pad=a.pad;c->pad_valid=1;c->pad_count++;}
        return 1;
    }
order_error:
    c->status=9; /* Event order/clock mismatch, never a synthetic extra update. */
failure:
    c->failed_sample=a;c->failed_state=*s;c->failed_tick=c->ordered_tick;
    return 0;
}
/* A completed read remains owned by DVD until its recorded publication.
 * The caller still performed the real read; identity/results must agree. */
int fh_tape_release_io(FhTapeRing*r,FhTapeCursor*c,const FhReplaySnapshot*s,const FhTapeIo*io){
    if(c->status)return 1; /* On failure release real resources, never fake success. */
    if(!c->started&&!fh_tape_ordered(r,c,FH_TAPE_ANCHOR,s,NULL,NULL))return 1;
    FhTapeSample a;
    if(__atomic_load_n(&r->error,__ATOMIC_ACQUIRE)){c->status=8;return 1;}
    if(!fh_tape_peek(r,&a)){c->status=7;return 1;}
    if(a.flags==FH_TAPE_END){c->status=1;return 1;}
    if(a.phase!=FH_TAPE_IO)return 0;
    if(a.tick!=c->ordered_tick||memcmp(&a.io,io,sizeof(*io))){
        c->status=10;c->failed_sample=a;c->failed_state=*s;c->failed_tick=c->ordered_tick;return 1;
    }
    fh_tape_pop(r);c->io_count++;return 1;
}
uint64_t fh_tape_time(FhTapeRing*r,FhTapeCursor*c,const FhReplaySnapshot*s,unsigned site,uint64_t actual){
    if(c->status)return actual;
    if(!c->started&&!fh_tape_ordered(r,c,FH_TAPE_ANCHOR,s,NULL,NULL))return actual;
    FhTapeSample a;
    if(__atomic_load_n(&r->error,__ATOMIC_ACQUIRE)){c->status=8;return actual;}
    if(!fh_tape_peek(r,&a)){c->status=7;return actual;}
    if(a.flags==FH_TAPE_END){c->status=1;return actual;}
    if(a.phase!=FH_TAPE_TIME||a.time.site!=site||a.tick!=c->ordered_tick){
        c->status=9;c->failed_sample=a;c->failed_state=*s;c->failed_tick=c->ordered_tick;return actual;
    }
    fh_tape_pop(r);return a.time.low|((uint64_t)a.time.high<<32);
}
void fh_tape_play(FhTapeRing*r,FhTapeCursor*c,double sim,const FhReplaySnapshot*s,FhPadState*out){
    memset(out,0,sizeof(*out));if(c->status)return;
    if(__atomic_load_n(&r->error,__ATOMIC_ACQUIRE)){c->status=8;return;}
    if(!isfinite(sim)||sim<0||(c->started&&sim<c->last)){c->status=4;return;}
    if(!c->started){c->origin=sim;c->started=1;}
    if(c->boot_wait){double waited=sim-c->last;c->origin+=waited;c->boot_wait_ticks+=waited;c->wait_elapsed+=waited;}
    c->last=sim;
    double tick=sim-c->origin;unsigned edges=0;FhTapeSample a;
    for(;;){
        if(!fh_tape_peek(r,&a)){c->status=7;return;} /* Prefetch starvation: invalidate, never wait. */
        if(a.tick>tick)break;
        /* Loading and noninteractive menu fades finish at different wall
         * times. At a recorded change of UI, wait for that exact state before
         * consuming further input. Only idle, playerless transitions qualify;
         * no gameplay position/sequence or same-UI mismatch is softened.
         * +/-8 admits idle hardware stick noise, not directional menu input.
         * Simulation and IO continue, checks remain queued, wall timeout and
         * explicit wait counters retain the full cost. Never synthesize taps. */
        FhPadState neutral={0};
        if(a.flags==FH_TAPE_CHECK && c->checks && a.tick>0 &&
           a.ui!=c->checked_ui && a.ui!=s->ui && !a.player && !s->player && !a.loading &&
           menu_idle(&a.pad) && menu_idle(&c->pad)){
            if(!c->boot_wait){c->wait_elapsed=0;c->wait_count++;}
            c->boot_wait=1;
            *out=c->pad;
            if(c->wait_elapsed>3600){c->status=3;c->failed_sample=a;c->failed_state=*s;c->failed_tick=tick;}
            return;
        }
        c->boot_wait=0;
        fh_tape_pop(r);
        if(a.flags==FH_TAPE_END){c->status=1;return;}
        edges+=c->seen&&a.pad.buttons!=c->pad.buttons;
        if(edges>1){c->status=6;return;} /* Never silently lose a short tap. */
        c->seen=1;c->pad=a.pad;
        if(memcmp(&a.pad,&neutral,sizeof(neutral)))c->input_seen=1;
        double late=tick-a.tick;if(late>c->max_late)c->max_late=late;
        if(a.flags&FH_TAPE_CHECK){
            c->checks++;
            float x=s->x-a.x,y=s->y-a.y,z=s->z-a.z;
            if(s->ui!=a.ui||s->player!=a.player||s->loading!=a.loading||
               (a.player&&!a.loading&&(s->sequence!=a.sequence||x*x+y*y+z*z>160.f*160.f))){c->status=2;c->failed_sample=a;c->failed_state=*s;c->failed_tick=tick;return;}
            c->checked_ui=a.ui;
        }
    }
    *out=c->pad;
}
