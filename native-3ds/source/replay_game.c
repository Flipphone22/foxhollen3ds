#include "fh_replay.h"
#include "fh_input_tape.h"
#include "fh_replay_random_trace.h"
#include "game/objects/object.h"
#include "sys/objects.h"
#include "main/frame_timing.h"
#include "main/camera_interface.h"
#include "main/dll/dll_0044_cameramodeviewfinder.h"
extern unsigned char gGameLoopInitComplete,gGameLoopMapLoadPending,gTitleMenuReadyForInput;
extern int curUiDll;
#if defined(FH_REPLAY_BUILD) || defined(FH_CPU_PANEL_BUILD)
static double sim_clock;
unsigned fh_replay_game_frames;
void __real_waitNextFrame(void);
void __wrap_waitNextFrame(void){unsigned phase=fh_replay_phase(FH_PHASE_FRAME_SETUP);__real_waitNextFrame();sim_clock+=timeDelta;fh_replay_game_frames++;fh_replay_phase(phase);}
/* Same signatures as existing emulator wrappers; preserve return values and
 * arguments. Exclusive phases also handle VI/presentation nested in a draw. */
#define REPLAY_VOID(name,kind) \
 void __real_##name(void); \
 void __wrap_##name(void){unsigned p=fh_replay_phase(FH_PHASE_##kind);__real_##name();fh_replay_phase(p);}
REPLAY_VOID(audioUpdate,AUDIO_UPDATE)
REPLAY_VOID(loadDataFiles,LOAD_FILES)
REPLAY_VOID(uiDll_runFrameEndAndLoadNext,UI_END)
REPLAY_VOID(gameTextRun,TEXT)
REPLAY_VOID(Sfx_UpdateLoopedObjectSounds,SFX)
REPLAY_VOID(trackIntersect,COLLISION)
REPLAY_VOID(drawReflectionTexture,REFLECTION_SMALL)
REPLAY_VOID(updateReflectionTextures,REFLECTION_UPDATE)
REPLAY_VOID(Rcp_UpdateDistortionTextures,DISTORTION)
REPLAY_VOID(renderParticles,PARTICLES)
int __real_uiDll_runFrameStartAndLoadNext(void);
int __wrap_uiDll_runFrameStartAndLoadNext(void){unsigned p=fh_replay_phase(FH_PHASE_UI_START);int r=__real_uiDll_runFrameStartAndLoadNext();fh_replay_phase(p);return r;}
void __real_Obj_UpdateAllObjects(unsigned char flags);
void __wrap_Obj_UpdateAllObjects(unsigned char flags){unsigned p=fh_replay_phase(FH_PHASE_OBJECT_UPDATE);__real_Obj_UpdateAllObjects(flags);fh_replay_phase(p);}
void __real_updateEnvironment(int mode);
void __wrap_updateEnvironment(int mode){unsigned p=fh_replay_phase(FH_PHASE_ENVIRONMENT);__real_updateEnvironment(mode);fh_replay_phase(p);}
void __real_sceneRender(int a,int b,int c,int d,int e,int f);
void __wrap_sceneRender(int a,int b,int c,int d,int e,int f){unsigned p=fh_replay_phase(FH_PHASE_SCENE_RENDER);__real_sceneRender(a,b,c,d,e,f);fh_replay_phase(p);}
void __real_curUiDllDraw(int a,int b,int c,int d);
void __wrap_curUiDllDraw(int a,int b,int c,int d){unsigned p=fh_replay_phase(FH_PHASE_UI_DRAW);__real_curUiDllDraw(a,b,c,d);fh_replay_phase(p);}
void __real_objRender(int a,int b,int c,int d,GameObject *obj,int flag);
void __wrap_objRender(int a,int b,int c,int d,GameObject *obj,int flag){unsigned p=fh_replay_phase(FH_PHASE_OBJECT_RENDER);__real_objRender(a,b,c,d,obj,flag);fh_replay_phase(p);}
#undef REPLAY_VOID
double fh_replay_sim_clock(void){return sim_clock;}
#endif
void fh_replay_snapshot(FhReplaySnapshot*s){
    *s=(FhReplaySnapshot){.delta=timeDelta,.ui=curUiDll,.title=gTitleMenuReadyForInput,.loading=gGameLoopMapLoadPending,.sequence=-1};
    GameObject *p=gGameLoopInitComplete&&!gGameLoopMapLoadPending?Obj_GetPlayerObject():0;
    if(p&&p->anim.classId==1){s->player=1;s->x=p->anim.worldPosX;s->y=p->anim.worldPosY;s->z=p->anim.worldPosZ;s->sequence=p->seqIndex;}
    if(s->player&&gCameraInterface&&*gCameraInterface){
        CameraObject *camera=(*gCameraInterface)->getCamera();
        if(camera){
            s->camera_x=camera->anim.worldPosX;s->camera_y=camera->anim.worldPosY;s->camera_z=camera->anim.worldPosZ;
            s->camera_yaw=camera->anim.rotX;s->camera_pitch=camera->anim.rotY;
            if(camera->anim.targetObj==(void*)p&&gCameraModeViewfinderState&&
               gCameraModeViewfinderState->phase==CAMERA_MODE_VIEWFINDER_PHASE_ACTIVE){
                s->viewfinder=1;s->yaw_speed=gCameraModeViewfinderState->yawSpeed;
            }
        }
    }
}

static double input_sim_clock;
uint64_t fh_input_movie_time(unsigned site,uint64_t time){
#ifdef FH_REPLAY_BUILD
    if(fh_replay_active())return fh_replay_movie_time(site,time);
    fh_record_movie_time(site,time);return time;
#else
    fh_record_movie_time(site,time);return time;
#endif
}
int fh_input_dvd_complete(unsigned file,unsigned offset,unsigned bytes,int result){
    FhTapeIo io={file,offset,bytes,result};
#ifdef FH_REPLAY_BUILD
    if(fh_replay_active())return fh_replay_dvd_complete(&io);
    fh_record_dvd_complete(&io);return 1;
#else
    fh_record_dvd_complete(&io);return 1;
#endif
}
void fh_input_clock_advance(float delta){input_sim_clock+=delta;}
double fh_input_clock(void){return input_sim_clock;}
float fh_input_frame_delta(float delta){
#ifdef FH_REPLAY_BUILD
    if(fh_replay_active()){fh_replay_random_trace_frame(fh_input_clock());return fh_replay_frame_delta(delta);}
    fh_record_frame(delta);return delta;
#else
    fh_record_frame(delta);
    return delta;
#endif
}
