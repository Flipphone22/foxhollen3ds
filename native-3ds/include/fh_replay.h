#ifndef FH_REPLAY_H
#define FH_REPLAY_H
#include <stdio.h>
#include "fh_platform.h"
#include "fh_replay_timing.h"
enum { FH_REPLAY_STEPS=128, FH_REPLAY_ROWS=4096 };
enum { FH_R_HOLD=1,FH_R_TITLE,FH_R_UI,FH_R_PLAYER,FH_R_CHECK,FH_R_END,FH_R_LOAD,FH_R_AIM,FH_R_VIEW,FH_R_TAPE,FH_R_WALK };
typedef struct {
    float delta,x,y,z; int ui,sequence; unsigned player,title,loading;
    float camera_x,camera_y,camera_z,yaw_speed;
    int camera_yaw,camera_pitch; unsigned viewfinder;
} FhReplaySnapshot;
typedef struct {
    unsigned kind,measure; float duration,timeout,x,y,z,radius; int ui;
    FhPadState pad;
} FhReplayStep;
typedef struct {
    unsigned save_bytes,save_crc,count,wall_limit,graphics_options,original_geometry,original_textures,graphics_effects;
    FhReplayStep steps[FH_REPLAY_STEPS];
} FhReplayPlan;
typedef struct {
    unsigned step,started,emitted,status,pulse;
    double start_sim,start_wall,max_lateness,pulse_wall,aligned_since;
} FhReplayCursor;
/* status: 0 running, 1 completed, 2 checkpoint failure, 3 deadline, 4 bad clock. */
int fh_replay_parse(FILE*,FhReplayPlan*,char*,unsigned);
void fh_replay_advance(const FhReplayPlan*,FhReplayCursor*,const FhReplaySnapshot*,double,double,FhPadState*);
void fh_replay_snapshot(FhReplaySnapshot*);
#ifdef FH_REPLAY_BUILD
extern unsigned fh_replay_native_worker;
unsigned fh_replay_active(void);
double fh_replay_sim_clock(void);
int fh_replay_menu(volatile unsigned *retraces);
void fh_replay_pad(FhPadState*);
int fh_replay_finished(void);
void fh_replay_finish(void);
void fh_replay_input_stop(void);
#endif
#endif
