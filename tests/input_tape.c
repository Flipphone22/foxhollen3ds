#include "fh_input_tape.h"
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static FhTapeRing ring;
static void*produce(void*unused){(void)unused;for(unsigned i=0;i<100000;i++){FhTapeSample s={.tick=i,.ui=(int)i};while(!fh_tape_push(&ring,&s)){};}return NULL;}
static void*consume(void*unused){(void)unused;for(unsigned i=0;i<100000;i++){FhTapeSample s;while(!fh_tape_peek(&ring,&s)){};assert(s.tick==i&&s.ui==(int)i);fh_tape_pop(&ring);}return NULL;}
int main(void){
    unsigned char wire[64],again[64];FhTapeSample s={.tick=123.125,.pad={.buttons=0x140,.x=-127,.y=127,.cx=-42,.cy=9,.left=255,.right=7,.analog_a=255},.x=-12.5,.y=1024,.z=0.25,.ui=14,.sequence=-1,.player=1,.flags=1},d;
    assert(fh_tape_encode(wire,&s)&&fh_tape_decode(&d,wire)&&fh_tape_encode(again,&d));assert(!memcmp(wire,again,64));
    wire[52]=6;assert(!fh_tape_decode(&d,wire));wire[52]=0;wire[8]=255;wire[9]=255;assert(!fh_tape_decode(&d,wire));
    s.tick=NAN;assert(!fh_tape_encode(wire,&s));s.tick=0;
    ring.head=ring.tail=~0u-200;
    for(unsigned i=0;i<FH_TAPE_RING;i++){s.tick=i;assert(fh_tape_push(&ring,&s));}assert(!fh_tape_push(&ring,&s));
    for(unsigned i=0;i<FH_TAPE_RING;i++){assert(fh_tape_peek(&ring,&d)&&d.tick==i);fh_tape_pop(&ring);}assert(!fh_tape_peek(&ring,&d));
    memset(&ring,0,sizeof(ring));pthread_t a,b;assert(!pthread_create(&a,NULL,produce,NULL)&&!pthread_create(&b,NULL,consume,NULL));pthread_join(a,NULL);pthread_join(b,NULL);
    const double rates[]={1,2,3,0.75,1.9};
    for(unsigned r=0;r<5;r++){
        memset(&ring,0,sizeof(ring));
        for(unsigned i=0;i<4;i++){s=(FhTapeSample){.tick=i*60,.pad={.buttons=i%2?0x100:0}};assert(fh_tape_push(&ring,&s));}
        s=(FhTapeSample){.tick=240,.flags=FH_TAPE_END};assert(fh_tape_push(&ring,&s));
        FhTapeCursor c={0};FhReplaySnapshot state={0};FhPadState out;
        for(double time=0;time<245;time+=rates[r]){fh_tape_play(&ring,&c,time,&state,&out);assert(!c.status||c.status==1);if(c.status)break;assert(out.buttons==(((unsigned)(time/60)%2)?0x100:0));}
        assert(c.status==1&&c.max_late<rates[r]+1e-8);
    }
    memset(&ring,0,sizeof(ring));FhTapeCursor c={0};FhPadState pad;FhReplaySnapshot state={0};fh_tape_play(&ring,&c,0,&state,&pad);assert(c.status==7);
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};s=(FhTapeSample){.flags=FH_TAPE_CHECK,.ui=99};assert(fh_tape_push(&ring,&s));fh_tape_play(&ring,&c,0,&state,&pad);assert(c.status==2);
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};for(unsigned i=0;i<4;i++){s=(FhTapeSample){.tick=i,.pad={.buttons=i%2?0x100:0}};assert(fh_tape_push(&ring,&s));}fh_tape_play(&ring,&c,0,&state,&pad);fh_tape_play(&ring,&c,2,&state,&pad);assert(c.status==6);
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};
    s=(FhTapeSample){.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
    s=(FhTapeSample){.tick=300,.ui=4,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
    s=(FhTapeSample){.tick=434,.ui=4,.pad={.buttons=0x100}};assert(fh_tape_push(&ring,&s));
    s=(FhTapeSample){.tick=450,.ui=4,.player=1,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
    fh_tape_play(&ring,&c,0,&state,&pad);fh_tape_play(&ring,&c,301,&state,&pad);
    assert(!c.status&&c.boot_wait&&ring.tail==1&&!pad.buttons);
    fh_tape_play(&ring,&c,701,&state,&pad);assert(!c.status&&c.boot_wait_ticks==400&&ring.tail==1);
    state.ui=4;fh_tape_play(&ring,&c,702,&state,&pad);assert(!c.status&&!c.boot_wait&&c.checks==2&&c.boot_wait_ticks==401);
    fh_tape_play(&ring,&c,834,&state,&pad);assert(!pad.buttons);
    fh_tape_play(&ring,&c,835,&state,&pad);assert(pad.buttons==0x100);
    fh_tape_play(&ring,&c,851,&state,&pad);assert(c.status==2); /* Later state mismatch stays fatal. */
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};
    s=(FhTapeSample){.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));s.tick=300;s.ui=4;s.pad.y=-6;assert(fh_tape_push(&ring,&s));
    fh_tape_play(&ring,&c,0,&state,&pad);fh_tape_play(&ring,&c,300,&state,&pad);fh_tape_play(&ring,&c,3901,&state,&pad);assert(c.status==3);
    for(unsigned cadence=0;cadence<5;cadence++){
        memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};double ticks=0;
        s=(FhTapeSample){.phase=FH_TAPE_ANCHOR,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
        for(unsigned i=0;i<150;i++){
            float dt=i%7==0?0:((i%3)+1)*0.75f;ticks+=(double)dt;
            s=(FhTapeSample){.tick=ticks,.phase=FH_TAPE_CLOCK,.delta=dt};assert(fh_tape_push(&ring,&s));
            s=(FhTapeSample){.tick=ticks,.phase=FH_TAPE_PAD,.flags=FH_TAPE_CHECK,.pad={.buttons=i%2?0x100:0,.x=(int)(i%41)-20}};assert(fh_tape_push(&ring,&s));
            if(i%5==0)assert(fh_tape_push(&ring,&s)); /* Repeated poll at identical time. */
        }
        s=(FhTapeSample){.tick=ticks,.flags=FH_TAPE_END};assert(fh_tape_push(&ring,&s));
        double actual=0,wall=0;float scroll=0,velocity=0,expected_scroll=0,expected_velocity=0;
        for(unsigned i=0;i<150;i++){
            float dt=99;wall+=rates[cadence];assert(fh_tape_ordered(&ring,&c,FH_TAPE_CLOCK,&state,NULL,&dt));
            assert(dt==(i%7==0?0:((i%3)+1)*0.75f));actual+=dt;
            assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));assert(pad.buttons==(i%2?0x100:0));
            scroll+=velocity;velocity+=0.025f*(pad.x*0.08f-velocity);
            expected_scroll+=expected_velocity;expected_velocity+=0.025f*(((int)(i%41)-20)*0.08f-expected_velocity);
            if(i%5==0)assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));
            if(i<149){unsigned before=ring.tail;for(unsigned extra=0;extra<cadence*3;extra++)assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));assert(ring.tail==before);}
        }
        assert(actual==ticks&&wall>0&&scroll==expected_scroll&&velocity==expected_velocity);
        assert(c.clock_count==150&&c.pad_count==180&&c.pad_reuses==cadence*3*149);
        float dt=99;assert(!fh_tape_ordered(&ring,&c,FH_TAPE_CLOCK,&state,NULL,&dt)&&c.status==1);
    }
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};
    s=(FhTapeSample){.phase=FH_TAPE_ANCHOR,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
    s=(FhTapeSample){.phase=FH_TAPE_CLOCK,.delta=2,.tick=2};assert(fh_tape_push(&ring,&s));
    assert(!fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL)&&c.status==9);
    for(unsigned delays=0;delays<5;delays++){
        memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};
        s=(FhTapeSample){.phase=FH_TAPE_ANCHOR,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
        s=(FhTapeSample){.phase=FH_TAPE_CLOCK,.delta=1,.tick=1};assert(fh_tape_push(&ring,&s));
        FhTapeIo io={42,1024,32,32};s=(FhTapeSample){.phase=FH_TAPE_IO,.tick=1,.io=io};
        assert(fh_tape_encode(wire,&s)&&fh_tape_decode(&d,wire)&&!memcmp(&d.io,&io,sizeof(io)));
        assert(fh_tape_push(&ring,&s));
        for(unsigned i=0;i<=delays;i++)assert(!fh_tape_release_io(&ring,&c,&state,&io));
        assert(ring.tail==1&&!c.io_count&&!c.status);float dt;
        assert(fh_tape_ordered(&ring,&c,FH_TAPE_CLOCK,&state,NULL,&dt)&&dt==1);
        assert(fh_tape_release_io(&ring,&c,&state,&io)&&c.io_count==1&&!c.status);
        assert(fh_tape_push(&ring,&s));io.offset++;assert(fh_tape_release_io(&ring,&c,&state,&io)&&c.status==10);
    }
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};
    s=(FhTapeSample){.phase=FH_TAPE_ANCHOR,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
    s=(FhTapeSample){.phase=FH_TAPE_TIME,.time={2,123,8,0}};
    assert(fh_tape_encode(wire,&s)&&fh_tape_decode(&d,wire)&&d.time.high==8);
    FhTapeSample poll={.phase=FH_TAPE_PAD,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&poll));
    assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));assert(fh_tape_push(&ring,&s));
    unsigned tail=ring.tail;for(unsigned i=0;i<100;i++)assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));
    assert(ring.tail==tail&&c.pad_reuses==100);
    assert(fh_tape_time(&ring,&c,&state,2,999)==(8ULL<<32)+123&&!c.status);
    assert(fh_tape_push(&ring,&s));assert(fh_tape_time(&ring,&c,&state,1,999)==999&&c.status==9);
    /* Hardware trace: completed boot read wakes a loading loop, whose
     * additional PAD poll precedes queuing the next read. No packet or
     * simulation step can be consumed while waiting for that real request. */
    memset(&ring,0,sizeof(ring));c=(FhTapeCursor){0};state=(FhReplaySnapshot){0};
    s=(FhTapeSample){.phase=FH_TAPE_ANCHOR,.flags=FH_TAPE_CHECK};assert(fh_tape_push(&ring,&s));
    s=(FhTapeSample){.phase=FH_TAPE_PAD,.flags=FH_TAPE_CHECK,.pad={.buttons=0x100}};assert(fh_tape_push(&ring,&s));
    assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));
    s=(FhTapeSample){.phase=FH_TAPE_CLOCK,.tick=1,.delta=1};assert(fh_tape_push(&ring,&s));
    float io_delta;assert(fh_tape_ordered(&ring,&c,FH_TAPE_CLOCK,&state,NULL,&io_delta));
    s=(FhTapeSample){.phase=FH_TAPE_IO,.tick=1,.io={1728,0,64,64}};assert(fh_tape_push(&ring,&s));
    tail=ring.tail;for(unsigned i=0;i<100;i++){assert(fh_tape_ordered(&ring,&c,FH_TAPE_PAD,&state,&pad,NULL));assert(pad.buttons==0x100);}
    assert(ring.tail==tail&&c.ordered_tick==1&&c.pad_count==1&&c.clock_count==1&&c.pad_reuses==100);
    assert(fh_tape_release_io(&ring,&c,&state,&s.io)&&!c.status);
    printf("PASS: wire/ring/threading; legacy cadence rejection; ordered 60/30/20/irregular wall rates preserve exact calls, zero deltas, repeated polls and frame-based scroll; wrong call order rejected; ring=%zu bytes\n",sizeof(ring));
}
