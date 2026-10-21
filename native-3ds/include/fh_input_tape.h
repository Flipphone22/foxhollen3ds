#ifndef FH_INPUT_TAPE_H
#define FH_INPUT_TAPE_H
#include "fh_replay.h"
enum { FH_TAPE_RING=4096,FH_TAPE_WIRE=64,FH_TAPE_CHECK=1,FH_TAPE_END=2 };
enum { FH_TAPE_PAD=1,FH_TAPE_CLOCK=2,FH_TAPE_ANCHOR=3,FH_TAPE_IO=4,FH_TAPE_TIME=5 };
typedef struct { unsigned file,offset,bytes; int result; } FhTapeIo;
typedef struct { double tick; FhPadState pad; float x,y,z; int ui,sequence; unsigned player,loading,flags,phase; float delta; union { FhTapeIo io; struct { unsigned site,low,high,reserved; } time; }; } FhTapeSample;
typedef struct { unsigned head,tail,error,done,peak,version; FhTapeSample sample[FH_TAPE_RING]; } FhTapeRing;
typedef struct { unsigned started,status,seen,checks,boot_wait,input_seen,wait_count,pad_valid,clock_count,pad_count,pad_reuses,io_count; int checked_ui;
    double origin,last,max_late,boot_wait_ticks,wait_elapsed,failed_tick,ordered_tick;
    FhPadState pad; FhTapeSample failed_sample; FhReplaySnapshot failed_state;
} FhTapeCursor;
int fh_tape_push(FhTapeRing*,const FhTapeSample*);
int fh_tape_peek(FhTapeRing*,FhTapeSample*);
void fh_tape_pop(FhTapeRing*);
int fh_tape_encode(unsigned char[FH_TAPE_WIRE],const FhTapeSample*);
int fh_tape_decode(FhTapeSample*,const unsigned char[FH_TAPE_WIRE]);
void fh_tape_play(FhTapeRing*,FhTapeCursor*,double,const FhReplaySnapshot*,FhPadState*);
int fh_tape_ordered(FhTapeRing*,FhTapeCursor*,unsigned,const FhReplaySnapshot*,FhPadState*,float*);
int fh_tape_release_io(FhTapeRing*,FhTapeCursor*,const FhReplaySnapshot*,const FhTapeIo*);
uint64_t fh_tape_time(FhTapeRing*,FhTapeCursor*,const FhReplaySnapshot*,unsigned,uint64_t);
uint64_t fh_input_movie_time(unsigned,uint64_t);
uint64_t fh_replay_movie_time(unsigned,uint64_t);
void fh_record_movie_time(unsigned,uint64_t);
int fh_input_dvd_complete(unsigned file,unsigned offset,unsigned bytes,int result);
int fh_replay_dvd_complete(const FhTapeIo*);
void fh_record_dvd_complete(const FhTapeIo*);
float fh_input_frame_delta(float);
void fh_record_frame(float);
float fh_replay_frame_delta(float);
void fh_input_clock_advance(float);
double fh_input_clock(void);
void fh_record_touch(unsigned held,int x,int y);
void fh_record_pad(const FhPadState*);
unsigned fh_record_draw(uint16_t*,unsigned initial);
unsigned fh_record_active(void);
void fh_record_finish(void);
int fh_card_record_fixture(const char*,unsigned*,unsigned*);
/* Only the owned stream thread accesses the tape FILE. Startup/stop may wait;
 * gameplay enqueue/consume never performs filesystem I/O or waits. */
int fh_tape_stream_open(const char*,unsigned writing);
FhTapeRing *fh_tape_stream_ring(void);
int fh_tape_stream_close(double duration);
void fh_tape_stream_invalidate(void);
void fh_tape_stream_notify(void);
#endif
