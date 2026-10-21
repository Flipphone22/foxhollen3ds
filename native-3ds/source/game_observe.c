#include <stdio.h>
#include "fh_platform.h"
#include "game/objects/object.h"
#include "sys/objects.h"
#include "main/frame_timing.h"
#include "main/dll/FRONT/attract_movie.h"
#include "dlls/objects/488_SB_Galleon.h"
extern GameObject **ObjList_GetObjects(int *first,int *end);
extern void fh_object_check_events(GameObject*);
extern unsigned char gTitleMenuReadyForInput,gTitleMenuSelection,gTitleMenuSelectionFade,gAttractMoviePlaybackEnabled;
extern unsigned char gGameLoopMapLoaded,gGameLoopMapLoadPending,gGameLoopInitComplete,gameState,timeStop;
extern int gGameLoopPendingMapId,gGameLoopPendingUiDllId,gAttractMovieState;
extern volatile int gSaveCardState;
extern unsigned char gSaveGameEnabled;
extern int curUiDll;
extern signed char gSaveSelectPanelIndex,saveFileSelect_currentSlotIndex;
extern unsigned char gSaveSelectLaunchPending,gSaveSelectChapter;
extern unsigned char gEnterSaveNameLength,gEnterSaveNameAutoScrolling;
extern int gEnterSaveNameSelectedIndex;
void fh_game_observe(unsigned retrace) {
    static FILE *file;
    if(!file){file=fopen("sdmc:/3ds/foxhollow/state.jsonl","wb");if(file)setvbuf(file,NULL,_IOFBF,8192);}
    if(!file)return;
    fprintf(file,"{\"retrace\":%u,\"title_ready\":%u,\"title_selection\":%u,\"title_fade\":%u,\"movie_enabled\":%u,\"movie_state\":%d,\"map_loaded\":%u,\"map_pending\":%u,\"pending_map\":%d,\"pending_ui\":%d,\"initialized\":%u,\"game_state\":%u,\"time_stop\":%u,\"delta\":%.6g,\"step\":%u",retrace,gTitleMenuReadyForInput,gTitleMenuSelection,gTitleMenuSelectionFade,gAttractMoviePlaybackEnabled,gAttractMovieState,gGameLoopMapLoaded,gGameLoopMapLoadPending,gGameLoopPendingMapId,gGameLoopPendingUiDllId,gGameLoopInitComplete,gameState,timeStop,timeDelta,framesThisStep);
    fprintf(file,",\"movie_frame\":%d,\"movie_audio_frame\":%d,\"movie_dvd_error\":%d,\"movie_video_error\":%d",gAttractMoviePlayer.curVideoFrameNumber,gAttractMoviePlayer.curAudioFrameNumber,gAttractMoviePlayer.dvdError,gAttractMoviePlayer.videoError);
    GameObject *player=gGameLoopInitComplete?Obj_GetPlayerObject():NULL;
    if(player)fprintf(file,",\"player\":{\"class\":%d,\"x\":%.7g,\"y\":%.7g,\"z\":%.7g,\"sequence\":%d}",player->anim.classId,player->anim.worldPosX,player->anim.worldPosY,player->anim.worldPosZ,player->seqIndex);
    if(player && !gGameLoopMapLoadPending){
        int end=0;GameObject **objects=ObjList_GetObjects(NULL,&end);
        for(int i=0;i<end;i++)fh_object_check_events(objects[i]);
        for(int i=0;i<end;i++){
            const GameObject *object=objects[i];
            if(!object || !object->anim.dll || (*object->anim.dll)->update!=SB_Galleon_update || !object->extra)continue;
            const SBGalleonState *galleon=object->extra;
            fprintf(file,",\"galleon\":{\"stage\":%d,\"phase\":%d,\"damage_phase\":%u,\"phase_counter\":%d,\"camera_state\":%d}",galleon->stage,galleon->phase,galleon->damagePhase,galleon->phaseCounter,galleon->cameraState);
            break;
        }
    }
    fprintf(file,",\"card_state\":%d,\"save_enabled\":%u,\"ui\":%d,\"save_panel\":%d,\"save_slot\":%d,\"launch_pending\":%u,\"chapter\":%u,\"name_length\":%u,\"name_selection\":%d,\"name_scrolling\":%u}\n",gSaveCardState,gSaveGameEnabled,curUiDll,gSaveSelectPanelIndex,saveFileSelect_currentSlotIndex,gSaveSelectLaunchPending,gSaveSelectChapter,gEnterSaveNameLength,gEnterSaveNameSelectedIndex,gEnterSaveNameAutoScrolling);
    /* Keep input automation responsive without a synchronous SD write per frame. */
    if(retrace%4==0)fflush(file);
}
