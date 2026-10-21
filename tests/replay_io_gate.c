#include "fh_input_tape.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef uint64_t u64;
static unsigned active=1,has_tape=1,recording_version=5,io_waits,timeouts;
static u64 io_hold_start,io_wait_ticks,io_read_start,now=1;
static FhReplayCursor cursor;
static FhTapeCursor tape_cursor;
static FhTapeRing ring;
static u64 svcGetSystemTick(void){return now;}
static void svcSleepThread(long ns){assert(ns==100000);now+=26812;}
void fh_replay_snapshot(FhReplaySnapshot*s){memset(s,0,sizeof(*s));}
FhTapeRing *fh_tape_stream_ring(void){return &ring;}
void fh_tape_stream_notify(void){}
static void replay_io_timeout(const char*where){assert(!strcmp(where,"read"));timeouts++;}
/* ACTUAL_FUNCTIONS */
static void reset(void){
    memset(&ring,0,sizeof(ring));memset(&cursor,0,sizeof(cursor));
    memset(&tape_cursor,0,sizeof(tape_cursor));tape_cursor.started=1;
    io_read_start=io_hold_start=io_wait_ticks=io_waits=timeouts=0;now=1;
}
int main(void){
    FhTapeIo io={1728,0,64,64};
    FhTapeSample event={.phase=FH_TAPE_IO,.io=io};
    reset();assert(fh_tape_push(&ring,&event));
    assert(!fh_replay_dvd_wait(1729,0,64)&&!io_waits);
    assert(!fh_replay_dvd_wait(1728,32,64)&&!io_waits);
    assert(!fh_replay_dvd_wait(1728,0,32)&&!io_waits);
    for(unsigned i=0;i<100;i++)assert(fh_replay_dvd_wait(1728,0,64));
    assert(io_waits==1&&io_wait_ticks==2681200&&ring.tail==0&&!cursor.status);
    assert(fh_replay_dvd_complete(&io)&&ring.tail==1&&!io_read_start);
    /* A later read gets its own deadline, even with the same identity. */
    now+=30ULL*268111856;assert(fh_tape_push(&ring,&event));
    assert(fh_replay_dvd_wait(1728,0,64)&&io_waits==2&&!timeouts);
    assert(fh_replay_dvd_complete(&io)&&tape_cursor.io_count==2);
    reset();assert(fh_tape_push(&ring,&event));
    assert(fh_replay_dvd_wait(1728,0,64));now+=11ULL*268111856;
    assert(!fh_replay_dvd_wait(1728,0,64)&&cursor.status==11&&timeouts==1);
    /* A real completed buffer is released for cleanup after failure. */
    assert(fh_replay_dvd_complete(&io));
    reset();event.phase=FH_TAPE_PAD;assert(fh_tape_push(&ring,&event));
    assert(!fh_replay_dvd_wait(1728,0,64));assert(!fh_replay_dvd_complete(&io));
    assert(!ring.tail&&!cursor.status); /* Not yet the recorded completion. */
    puts("PASS actual replay IO gate: exact identity, early hold, real-read wait, per-request reset, bounded failure and cleanup");
}
