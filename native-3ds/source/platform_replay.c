#include "fh_graphics_options.h"
#ifdef FH_UNIFIED_APP
#include "fh_startup_runner.h"
#endif
#include "fh_pica_model.h"
#include "fh_hardware.h"
#ifdef FH_REPLAY_BUILD
#include <3ds.h>
#include "fh_pica_scene.h"
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/iosupport.h>
#include <zlib.h>
#include "fh_replay.h"
#include "fh_audio_bench_mode.h"
#include "fh_replay_random_trace.h"
#include "fh_input_tape.h"
#include "fh_gpu_work.h"
#include "platform_replay_hotspot.h"
int fh_replay_gpu_display(unsigned words[4],unsigned *failed,unsigned *ipc_us){
    *failed=hotspot_failed;*ipc_us=0;
    if(!hotspot_count)return 0;
    HotspotSample*s=hotspot_samples+hotspot_count-1;
    memcpy(words,s->words,sizeof(s->words));*ipc_us=s->ipc_ticks*1000000/268111856;
    return s->valid;
}
/* Pinned Melee card_storage.c uses separate profile directories. Each replay
 * instead gets a new owned directory; the immutable fixture and user card are
 * never opened for writing. All copying/log persistence is outside measurement. */
#define BASE "sdmc:/3ds/foxhollow"
#define CASES BASE "/replays/cases"
static FhReplayPlan plan;
static FhReplayCursor cursor;
static FhTapeCursor tape_cursor;
static unsigned has_tape,recording_version;
static char selected[32],run_dir[192];
static unsigned active,record_count,last_step=~0u,finished;
unsigned fh_replay_active(void){return active;}
unsigned fh_replay_native_worker;
unsigned fh_replay_render_mode=9;
unsigned fh_replay_texture_vram; /* Opt-in until physical safety/performance validation. */
static u64 start_wall;
static double last_sim=-1;
static FhPadState current_pad;
static FhReplayClock phase_clock;
static Thread phase_owner;
unsigned fh_replay_phase_at(unsigned phase,uint64_t now){
#ifdef FH_CPU_PANEL_BUILD
    extern unsigned fh_cpu_panel_phase_at(unsigned,uint64_t);
    if(!active)return fh_cpu_panel_phase_at(phase,now);
#endif
    if(!active || threadGetCurrent()!=phase_owner || phase>=FH_PHASE_COUNT)return FH_PHASE_COUNT;
    return fh_replay_clock_enter(&phase_clock,phase,now);
}
unsigned fh_replay_phase(unsigned phase){
#ifdef FH_CPU_PANEL_BUILD
    extern unsigned fh_cpu_panel_phase(unsigned);
    if(!active)return fh_cpu_panel_phase(phase);
#endif
    if(!active || threadGetCurrent()!=phase_owner || phase>=FH_PHASE_COUNT)return FH_PHASE_COUNT;
    return fh_replay_clock_enter(&phase_clock,phase,svcGetSystemTick());
}
static const char *phase_names[]={
#define PHASE_NAME(symbol,name) #name,
    FH_REPLAY_PHASES(PHASE_NAME)
#undef PHASE_NAME
};
typedef struct {
    u64 ticks;double sim;FhReplaySnapshot state;
    unsigned step,measure,frame,draws,indices;FhPadState pad;
    uint64_t phase_ticks[FH_PHASE_COUNT],gpu_ticks,gpu_sample;
    unsigned gpu_submitted,gpu_completed,gpu_pending,gpu_failed,gpu_unmatched,gpu_unsupported,phase_errors;
} ReplayRow;
static ReplayRow *rows;
static PrintConsole menu_console;
extern unsigned fh_minimal_frames,fh_minimal_draws,fh_minimal_indices;
extern unsigned fh_replay_game_frames;
static unsigned rendering_off(void){return fh_render_floor && !fh_minimal_enabled;}
static int safe_name(const char*s){
    unsigned n=0;for(;s[n];n++)if(n>=31||!((s[n]>='a'&&s[n]<='z')||(s[n]>='A'&&s[n]<='Z')||(s[n]>='0'&&s[n]<='9')||s[n]=='-'||s[n]=='_'))return 0;
    return n!=0;
}
static int copy_file(const char *from,const char *to){
    FILE *a=fopen(from,"rb");if(!a)return 0;
    FILE *b=fopen(to,"wb");if(!b){fclose(a);return 0;}
    unsigned char buffer[8192];size_t n;int ok=1;
    while((n=fread(buffer,1,sizeof(buffer),a))!=0)if(fwrite(buffer,1,n,b)!=n){ok=0;break;}
    if(ferror(a)||fflush(b))ok=0;
    if(fclose(a))ok=0;
    if(fclose(b))ok=0;
    return ok;
}
static int save_matches(const char *path){
    FILE *f=fopen(path,"rb");if(!f)return 0;
    unsigned char b[8192],header[64]={0};unsigned total=0;uLong crc=crc32(0,0,0);size_t n;
    while((n=fread(b,1,sizeof(b),f))!=0){
        if(!total){if(n<64){fclose(f);return 0;}memcpy(header,b,64);}
        if(n>plan.save_bytes-total){fclose(f);return 0;}total+=(unsigned)n;crc=crc32(crc,b,n);
    }
    int ok=!ferror(f)&&total==plan.save_bytes&&(unsigned)crc==plan.save_crc;
    if(ok&&plan.save_bytes)ok=64u+8192u*((unsigned)header[56]*256+header[57])==total;
    fclose(f);return ok;
}
static int validate_case(const char*name,char*error,unsigned size){
    char path[256];snprintf(path,sizeof(path),CASES "/%s/scenario.fhr",name);
    FILE *f=fopen(path,"rb");if(!f){snprintf(error,size,"Cannot read scenario");return 0;}
    int ok=fh_replay_parse(f,&plan,error,size);fclose(f);if(!ok)return 0;
    snprintf(path,sizeof(path),CASES "/%s/fixture.gci",name);
    if(!save_matches(path)){snprintf(error,size,"Save size/CRC/header mismatch");return 0;}
    snprintf(error,size,"Valid: %u steps; save checked",plan.count);return 1;
}
static int archive_logs(const char *prefix){
    const char *names[]={"native.log","game.log","memory.jsonl","minimal-render.csv","sd-io.csv","sd-slow.csv","sd-bursts.csv","log-io-summary.json","audio.csv"};
    char from[256],to[256];int ok=1;
    for(unsigned i=0;i<sizeof(names)/sizeof(*names);i++){
        snprintf(from,sizeof(from),BASE "/%s",names[i]);struct stat st;if(stat(from,&st))continue;
        if(st.st_size<0||st.st_size>64*1024*1024){ok=0;continue;}
        snprintf(to,sizeof(to),"%s/%s%s",run_dir,prefix,names[i]);
        /* Menu preparation precedes every game log fopen (platform_3ds.c).
         * Retain old logs by same-volume rename into this unique run, rather
         * than copying a long session's telemetry through the slow SD path.
         * Final archival may still have live handles and keeps copy semantics. */
        if(!strcmp(prefix,"previous-")){
            struct stat target;
            if(!stat(to,&target)||errno!=ENOENT||rename(from,to))ok=0;
        }else if(!copy_file(from,to))ok=0;
    }
    return ok;
}
static int prepare_run(char *error,unsigned size){
    char path[256],source[256];
    mkdir(BASE "/replays",0777);mkdir(BASE "/replays/runs",0777);
    int made=0;
    for(unsigned attempt=0;attempt<32;attempt++){
        snprintf(run_dir,sizeof(run_dir),BASE "/replays/runs/%s-%016llx-%u",selected,(unsigned long long)svcGetSystemTick(),attempt);
        if(!mkdir(run_dir,0777)){made=1;break;}if(errno!=EEXIST)break;
    }
    if(!made){snprintf(error,size,"Cannot create test run directory");return 0;}
    snprintf(path,sizeof(path),"%s/saves",run_dir);
    if(mkdir(path,0777)){snprintf(error,size,"Cannot isolate test saves");return 0;}
    snprintf(source,sizeof(source),CASES "/%s/fixture.gci",selected);
    snprintf(path,sizeof(path),"%s/saves/fixture.gci",run_dir);
    if(plan.save_bytes&&(!copy_file(source,path)||!save_matches(path))){snprintf(error,size,"Test save copy failed");return 0;}
    snprintf(source,sizeof(source),CASES "/%s/scenario.fhr",selected);
    snprintf(path,sizeof(path),"%s/scenario.fhr",run_dir);
    printf("\nPreparing isolated replay...\nPreserving previous logs...\n");
    gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
    if(!copy_file(source,path)||!archive_logs("previous-")){snprintf(error,size,"Cannot preserve prior logs");return 0;}
    has_tape=0;
    for(unsigned i=0;i<plan.count;i++)if(plan.steps[i].kind==FH_R_TAPE)has_tape++;
    if(has_tape){
        printf("Reading recorded inputs...\n");gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();
        snprintf(source,sizeof(source),CASES "/%s/recording.fht",selected);
        snprintf(path,sizeof(path),"%s/recording.fht",run_dir);
        if(has_tape!=1||!copy_file(source,path)||!fh_tape_stream_open(path,0)){
            snprintf(error,size,"Cannot validate/prefetch input tape");return 0;
        }
    }
    if(has_tape)recording_version=fh_tape_stream_ring()->version;
    rows=calloc(FH_REPLAY_ROWS,sizeof(*rows));
    if(!rows){if(has_tape)fh_tape_stream_close(0);snprintf(error,size,"Cannot allocate bounded replay log");return 0;}
    /* Publish the isolated card only after all fallible preparation.
     * Cancelling a failed case must never redirect subsequent normal play. */
    snprintf(path,sizeof(path),"%s/saves",run_dir);
    if(!fh_card_set_directory(path)){
        if(has_tape)fh_tape_stream_close(0);
        free(rows);rows=NULL;
        snprintf(error,size,"Cannot select isolated test saves");return 0;
    }
    fh_graphics_options_set(plan.graphics_options);
    fh_graphics_effects_set(plan.graphics_effects);
    fh_graphics_original_requested=plan.original_geometry;
    fh_graphics_textures_requested=plan.original_textures;
    fh_graphics_gpu_visible=hotspot_requested;
    fh_replay_random_trace_begin();
    active=1;phase_owner=threadGetCurrent();start_wall=svcGetSystemTick();fh_replay_clock_enter(&phase_clock,FH_PHASE_OTHER,start_wall);return 1;
}
int fh_replay_menu(volatile unsigned *retraces){
    /* Keep the explicit startup choice; audio has not started yet. */
    FILE *hot_config=fopen(BASE "/replay-hotspot.txt","r");
    if(hot_config){hotspot_requested=fgetc(hot_config)=='1';fclose(hot_config);}
    FILE *render_config=fopen(BASE "/render_disabled.txt","r");
    if(render_config)fclose(render_config); /* Retired backend setting is ignored. */
    /* A replay package may select its diagnostic stage without changing the
     * ordinary game's rendering configuration. */
#ifndef FH_UNIFIED_APP
    FILE *replay_visuals=fopen("sdmc:/3ds/foxhollow-replay/visuals.txt","r");
    if(replay_visuals){int mode=fgetc(replay_visuals);if(mode>='7'&&mode<='9')fh_replay_render_mode=mode-'0';fclose(replay_visuals);}
#endif
    FILE *vram_config=fopen(BASE "/texture_vram.txt","r");
    if(vram_config){fh_replay_texture_vram=fgetc(vram_config)=='1';fclose(vram_config);}
    if(fh_hardware_is_new){
    FILE *worker_config=fopen(BASE "/native_worker.txt","r");
    if(worker_config){fh_replay_native_worker=fgetc(worker_config)=='1';fclose(worker_config);}
    }
    const devoptab_t *saved_out=devoptab_list[STD_OUT],*saved_err=devoptab_list[STD_ERR];
    char names[16][32];unsigned count=0,index=0;DIR *dir=opendir(CASES);struct dirent *entry;
    if(dir){while(count<16&&(entry=readdir(dir)))if(safe_name(entry->d_name)){
        char path[256];struct stat st;snprintf(path,sizeof(path),CASES "/%.31s",entry->d_name);
        if(!stat(path,&st)&&S_ISDIR(st.st_mode))strcpy(names[count++],entry->d_name);
    }closedir(dir);}
    for(unsigned a=0;a<count;a++)for(unsigned b=a+1;b<count;b++)if(strcmp(names[a],names[b])>0){char temp[32];strcpy(temp,names[a]);strcpy(names[a],names[b]);strcpy(names[b],temp);}
    gfxInit(GSP_RGBA8_OES,GSP_RGB565_OES,false);
    for(unsigned i=0;i<2;i++){u16 w,h;u8*p=gfxGetFramebuffer(GFX_TOP,GFX_LEFT,&w,&h);memset(p,0,w*h*4);GSPGPU_FlushDataCache(p,w*h*4);gfxScreenSwapBuffers(GFX_TOP,false);gspWaitForVBlank();}
    consoleInit(GFX_BOTTOM,&menu_console);
#ifdef FH_UNIFIED_APP
    fh_startup_console_bind();
#endif
    char message[112]="Choose a test.\nNormal saves remain separate.";int result=0,redraw=1;
    while(aptMainLoop()){
        if(redraw){consoleClear();printf("Foxhollow benchmarks %.12s\n\n",FH_REPLAY_TAG);for(unsigned i=0;i<count;i++)printf("%c %s\n",i==index?'>':' ',names[i]);
            if(!count)printf("No cases in replays/cases\n");
            printf("\nA: validate + run  X: validate\nB: back\n");
            printf("L: visuals %s\n",fh_replay_render_mode==9?"PICA ACTORS + HUD":fh_replay_render_mode==8?"PICA TEXTURES":"PICA GEOMETRY");
            printf("Native renderer; fidelity incomplete\n");

            if(!fh_hardware_is_new)printf("Renderer: main (Old 3DS)\n");
            else printf("Y: renderer %s\n",fh_replay_native_worker?"SHARED (experimental)":"MAIN (baseline)");
            printf("Select: GPU profiling %s\n",hotspot_requested?"ON (timing perturbed)":"OFF");
            printf("R: texture VRAM %s\n",fh_replay_texture_vram?"ON (experimental)":"OFF (baseline)");
            printf("Start: audio %s\n",fh_audio_bench_mode()?"DSP":"CPU (reference)");
            printf("\n%s\n",message);redraw=0;}
        hidScanInput();u32 down=hidKeysDown();if(down&KEY_B)break;
        if(down&KEY_L){fh_replay_render_mode=fh_replay_render_mode==7?8:fh_replay_render_mode==8?9:7;redraw=1;}
        if(down&KEY_START){fh_audio_bench_select(!fh_audio_bench_mode());redraw=1;}
        if(down&KEY_SELECT){hotspot_requested^=1;redraw=1;}
        if(down&KEY_R){fh_replay_texture_vram^=1;redraw=1;}
        if(fh_hardware_is_new && (down&KEY_Y)){fh_replay_native_worker^=1;redraw=1;}
        if(count&&(down&(KEY_UP|KEY_DOWN))){index=(index+(down&KEY_UP?count-1:1))%count;redraw=1;}
        if(count&&(down&(KEY_A|KEY_X))){
            if(validate_case(names[index],message,sizeof(message))&&(down&KEY_A)){
                if(fh_replay_render_mode!=2)fh_replay_native_worker=0;
                strcpy(selected,names[index]);if(prepare_run(message,sizeof(message))){result=1;break;}
            }redraw=1;
        }
        gfxFlushBuffers();gfxSwapBuffers();gspWaitForVBlank();++*retraces;
    }
    fflush(stdout);fflush(stderr);
    devoptab_list[STD_OUT]=saved_out;devoptab_list[STD_ERR]=saved_err;
    gfxExit();return result; /* Startup displays the retained choice. */
}
static u64 io_hold_start,io_wait_ticks,io_read_start;
static unsigned io_waits;
/* Main-thread callback publication follows the tape, not SD speed. Real
 * reads can execute ahead, but completed buffers stay DVD-owned until release.
 * A blocked trace is an explicit failed run, never an infinite menu wait. */
int fh_replay_dvd_complete(const FhTapeIo*io){
    if(!active||!has_tape||recording_version<4||cursor.status||!fh_tape_stream_ring())return 1;
    FhReplaySnapshot s;fh_replay_snapshot(&s);
    int release=fh_tape_release_io(fh_tape_stream_ring(),&tape_cursor,&s,io);
    fh_tape_stream_notify();
    if(tape_cursor.status)cursor.status=tape_cursor.status;
    if(release){io_hold_start=io_read_start=0;return 1;}
    u64 now=svcGetSystemTick();if(!io_hold_start)io_hold_start=now;
    if(now-io_hold_start>10ULL*268111856){cursor.status=tape_cursor.status=11;return 1;}
    return 0;
}
static void replay_io_timeout(const char *where){
    FhTapeSample next={0};fh_tape_peek(fh_tape_stream_ring(),&next);
    char line[256];snprintf(line,sizeof(line),"Replay IO timeout: %s clock=%u pad=%u io=%u phase=%u tick=%.9f file=%u offset=%u bytes=%u result=%d\n",where,tape_cursor.clock_count,tape_cursor.pad_count,tape_cursor.io_count,next.phase,next.tick,next.io.file,next.io.offset,next.io.bytes,next.io.result);fh_platform_log(line);
    extern void fh_dvd_replay_diagnostic(void);fh_dvd_replay_diagnostic();
}
/* If this completion is next in the tape, finish its real read at the
 * current DVD pump. Deferring publication to a later PAD/CLOCK boundary can
 * let audio/asset initialization run against the wrong completion state. */
int fh_replay_dvd_wait(unsigned file,unsigned offset,unsigned bytes){
    FhTapeSample next;
    if(!active||!has_tape||recording_version<4||cursor.status||
       !fh_tape_peek(fh_tape_stream_ring(),&next)||next.phase!=FH_TAPE_IO||
       next.io.file!=file||next.io.offset!=offset||next.io.bytes!=bytes){io_read_start=0;return 0;}
    u64 now=svcGetSystemTick();if(!io_read_start){io_read_start=now;io_waits++;}
    if(now-io_read_start>10ULL*268111856){replay_io_timeout("read");cursor.status=tape_cursor.status=11;io_read_start=0;return 0;}
    svcSleepThread(100000);io_wait_ticks+=svcGetSystemTick()-now;return 1;
}
static void replay_drain_io(unsigned pad_poll){
    if(recording_version<4||cursor.status)return;
    FhTapeRing*r=fh_tape_stream_ring();FhTapeSample next;
    FhReplaySnapshot s;fh_replay_snapshot(&s);
    if(!tape_cursor.started)fh_tape_ordered(r,&tape_cursor,FH_TAPE_ANCHOR,&s,NULL,NULL);
    if(tape_cursor.status){cursor.status=tape_cursor.status;return;}
    u64 start=svcGetSystemTick();unsigned waited=0;
    while(!cursor.status&&fh_tape_peek(r,&next)&&next.phase==FH_TAPE_IO){
        extern void fh_dvd_pump(void);
        /* A loading-loop PAD poll can happen before the game has queued the
         * next recorded read. Returning the published input lets that loop
         * observe the previous completion and enqueue its next request.
         * CLOCK/TIME boundaries must still drain every preceding completion. */
        extern int fh_dvd_pending(void);
        if(pad_poll&&!fh_dvd_pending())break;
        unsigned before=tape_cursor.io_count;fh_dvd_pump();
        if(tape_cursor.io_count==before){
            waited=1;
            if(svcGetSystemTick()-start>10ULL*268111856){replay_io_timeout("drain");cursor.status=tape_cursor.status=11;break;}
            svcSleepThread(100000); /* Yield to the actual reader, no simulated update. */
        }
    }
    if(waited){io_waits++;io_wait_ticks+=svcGetSystemTick()-start;}
}
uint64_t fh_replay_movie_time(unsigned site,uint64_t time){
    if(!active||!has_tape||recording_version<5||cursor.status)return time;
    replay_drain_io(0);if(cursor.status)return time;
    FhReplaySnapshot s;fh_replay_snapshot(&s);
    time=fh_tape_time(fh_tape_stream_ring(),&tape_cursor,&s,site,time);
    fh_tape_stream_notify();if(tape_cursor.status)cursor.status=tape_cursor.status;
    return time;
}
void fh_replay_pad(FhPadState*out){
    if(!active)return;
    if(cursor.status){memset(out,0,sizeof(*out));return;} /* Preserve the first failure and its single log row. */
    replay_drain_io(1);
    if(cursor.status){memset(out,0,sizeof(*out));return;}
    double sim=fh_replay_sim_clock();
    if(sim!=last_sim||(has_tape&&fh_tape_stream_ring()->version>=3)){
        FhReplaySnapshot state;fh_replay_snapshot(&state);u64 now=svcGetSystemTick();
        double wall=(double)(now-start_wall)/268111856.;
        fh_replay_advance(&plan,&cursor,&state,sim,wall,&current_pad);
        if(!cursor.status&&cursor.step<plan.count&&plan.steps[cursor.step].kind==FH_R_TAPE){
            if(fh_tape_stream_ring()->version>=3)
                fh_tape_ordered(fh_tape_stream_ring(),&tape_cursor,FH_TAPE_PAD,&state,&current_pad,NULL);
            else fh_tape_play(fh_tape_stream_ring(),&tape_cursor,sim,&state,&current_pad);
            fh_tape_stream_notify();
            if(tape_cursor.max_late>cursor.max_lateness)cursor.max_lateness=tape_cursor.max_late;
            if(tape_cursor.status==1){cursor.step++;cursor.started=0;fh_replay_advance(&plan,&cursor,&state,sim,wall,&current_pad);}
            else if(tape_cursor.status)cursor.status=tape_cursor.status;
        }
        unsigned measure=cursor.step<plan.count?plan.steps[cursor.step].measure:0;
        if((measure&&(!has_tape||!record_count||sim-rows[record_count-1].sim>=60))||last_step!=cursor.step||cursor.status||(cursor.step<plan.count&&plan.steps[cursor.step].kind==FH_R_AIM)){
            if(record_count==FH_REPLAY_ROWS){cursor.status=5;memset(&current_pad,0,sizeof(current_pad));}
            else {
                ReplayRow*r=rows+record_count++;
                *r=(ReplayRow){.ticks=now,.sim=sim,.state=state,.step=cursor.step,.measure=measure,.frame=rendering_off()?fh_replay_game_frames:fh_minimal_frames,
                    .draws=__atomic_load_n(&fh_minimal_draws,__ATOMIC_RELAXED),.indices=__atomic_load_n(&fh_minimal_indices,__ATOMIC_RELAXED),.pad=current_pad};
                fh_replay_clock_snapshot(&phase_clock,now,r->phase_ticks);r->phase_errors=phase_clock.errors;
                FhGpuWork gpu;fh_gpu_work_snapshot(&gpu);r->gpu_ticks=gpu.busy_ticks;r->gpu_sample=gpu.sampled_ticks;
                r->gpu_submitted=gpu.submitted;r->gpu_completed=gpu.completed;r->gpu_pending=gpu.pending;
                r->gpu_failed=gpu.failed;r->gpu_unmatched=gpu.unmatched;r->gpu_unsupported=gpu.unsupported;
            }
        }
        hotspot_poll(measure && !cursor.status,rendering_off()?fh_replay_game_frames:fh_minimal_frames,cursor.step);
        last_step=cursor.step;last_sim=sim;
    }
    *out=current_pad;
}
float fh_replay_frame_delta(float delta){
    if(!active||!has_tape||!fh_tape_stream_ring()||fh_tape_stream_ring()->version<3||cursor.status)return delta;
    replay_drain_io(0);if(cursor.status)return delta;
    FhReplaySnapshot state;fh_replay_snapshot(&state);
    fh_tape_ordered(fh_tape_stream_ring(),&tape_cursor,FH_TAPE_CLOCK,&state,NULL,&delta);
    fh_tape_stream_notify();
    if(tape_cursor.status==1){cursor.step++;cursor.started=0;cursor.status=1;}
    else if(tape_cursor.status)cursor.status=tape_cursor.status;
    return delta;
}
int fh_replay_finished(void){
    if(active && ((fh_graphics_unavailable_seen&plan.graphics_options)||(fh_effects_unavailable_seen&plan.graphics_effects)) && cursor.status<=1)
        cursor.status=12; /* Requested visual workload could not be admitted. */
    if(active&&!cursor.status&&(double)(svcGetSystemTick()-start_wall)/268111856.>plan.wall_limit)cursor.status=3;
    return active&&cursor.status;
}
void fh_replay_input_stop(void){
    if(has_tape&&fh_tape_stream_ring()){
        if(!fh_tape_stream_close(0)&&cursor.status==1)cursor.status=8;
    }
}
void fh_replay_finish(void){
    fh_replay_input_stop();

    if(!active||finished)return;
    finished=1;
    if(((fh_graphics_unavailable_seen&plan.graphics_options)||(fh_effects_unavailable_seen&plan.graphics_effects)) && cursor.status<=1)cursor.status=12;
    if(!cursor.status)cursor.status=6; /* Select/APT cancellation, not success. */
    char path[256];snprintf(path,sizeof(path),"%s/replay.csv",run_dir);FILE*f=fopen(path,"wb");
    int ok=f!=NULL;
    if(!fh_replay_random_trace_finish(run_dir))ok=0;
    if(!hotspot_finish(run_dir))ok=0;
    if(f){fputs("ticks,sim_ticks,step,measured,render_frame,draws,indices,ui,player,x,y,z,sequence,buttons,stick_x,stick_y,c_x,c_y,camera_x,camera_y,camera_z,camera_yaw,camera_pitch,viewfinder,yaw_speed\n",f);
        for(unsigned i=0;i<record_count;i++){ReplayRow*r=rows+i;fprintf(f,"%llu,%.9f,%u,%u,%u,%u,%u,%d,%u,%.7g,%.7g,%.7g,%d,%u,%d,%d,%d,%d,%.7g,%.7g,%.7g,%d,%d,%u,%.7g\n",(unsigned long long)r->ticks,r->sim,r->step,r->measure,r->frame,r->draws,r->indices,r->state.ui,r->state.player,r->state.x,r->state.y,r->state.z,r->state.sequence,r->pad.buttons,r->pad.x,r->pad.y,r->pad.cx,r->pad.cy,r->state.camera_x,r->state.camera_y,r->state.camera_z,r->state.camera_yaw,r->state.camera_pitch,r->state.viewfinder,r->state.yaw_speed);}
        if(ferror(f))ok=0;
        if(fclose(f))ok=0;
    }
    snprintf(path,sizeof(path),"%s/frame-timing.csv",run_dir);f=fopen(path,"wb");
    if(!f)ok=0;
    else{
        fputs("ticks,render_frame,step,measured,phase_errors",f);
        for(unsigned j=0;j<FH_PHASE_COUNT;j++)fprintf(f,",%s_ticks",phase_names[j]);
        fputs(",gpu_sample_ticks,gpu_busy_ticks,gpu_submitted,gpu_completed,gpu_pending,gpu_failed,gpu_unmatched,gpu_unsupported\n",f);
        for(unsigned i=0;i<record_count;i++){
            ReplayRow*r=rows+i;fprintf(f,"%llu,%u,%u,%u,%u",(unsigned long long)r->ticks,r->frame,r->step,r->measure,r->phase_errors);
            for(unsigned j=0;j<FH_PHASE_COUNT;j++)fprintf(f,",%llu",(unsigned long long)r->phase_ticks[j]);
            fprintf(f,",%llu,%llu,%u,%u,%u,%u,%u,%u\n",(unsigned long long)r->gpu_sample,(unsigned long long)r->gpu_ticks,
                r->gpu_submitted,r->gpu_completed,r->gpu_pending,r->gpu_failed,r->gpu_unmatched,r->gpu_unsupported);
        }
        if(ferror(f))ok=0;
        if(fclose(f))ok=0;
    }
    fflush(stdout);if(!archive_logs(""))ok=0;
    snprintf(path,sizeof(path),"%s/result.json",run_dir);f=fopen(path,"wb");
    extern unsigned fh_minimal_worker_executed(void);
    if(f){fprintf(f,"{\"graphics_effects\":%u,\"graphics_effects_unavailable\":%u,\"recording_io_events\":%u,\"recording_io_waits\":%u,\"recording_io_wait_ms\":%.6f,",fh_graphics_effects,fh_effects_unavailable_seen,tape_cursor.io_count,io_waits,io_wait_ticks*1000./268111856.);fprintf(f,"\"recording_clock_steps\":%u,\"recording_pad_states\":%u,\"recording_pad_reuses\":%u,\"recording_version\":%u,\"recording_failed_phase\":%u,\"recording_transition_wait_ticks\":%.9f,\"recording_wait_count\":%u,\"recording_failed_tick\":%.9f,\"recording_expected_ui\":%d,\"recording_actual_ui\":%d,\"graphics_options\":%u,",tape_cursor.clock_count,tape_cursor.pad_count,tape_cursor.pad_reuses,recording_version,tape_cursor.failed_sample.phase,tape_cursor.boot_wait_ticks,tape_cursor.wait_count,tape_cursor.failed_tick,tape_cursor.failed_sample.ui,tape_cursor.failed_state.ui,fh_graphics_options);fprintf(f,"\"graphics_unavailable\":%u,",fh_graphics_unavailable_seen);fprintf(f,"\"pica_actor_geometry\":%s,",fh_pica_scene_actors?"true":"false");fprintf(f,"\"pica_scene_textures\":%s,\"pica_scene_geometry\":%s,",fh_pica_scene_textured?"true":"false",fh_pica_scene_enabled?"true":"false");fprintf(f,"\"gpu_hotspot_requested\":%s,\"world_only_reconstruction\":%s,\"world_textures\":%s,\"world_models\":%s,\"counter_kind\":\"%s\",\"rendering_off\":%s,\"stripped_visual_preparation\":%s,\"texture_vram_requested\":%u,\"worker_requested\":%u,\"worker_executed\":%u,\"schema\":1,\"timing_schema\":2,\"source_tag\":\"%s\",\"case\":\"%s\",\"status\":%u,\"step\":%u,\"rows\":%u,\"log_bytes\":%u,\"save_crc32\":%u,\"save_bytes\":%u,\"max_input_lateness_ticks\":%.9f,\"logs_complete\":%s,\"ticks_hz\":268111856,\"normal_saves_modified\":false}\n",hotspot_requested?"true":"false",fh_render_budget_world?"true":"false",fh_render_budget_textures?"true":"false",fh_render_budget_models?"true":"false",rendering_off()?"game_iterations":"render_submissions",rendering_off()?"true":"false",fh_render_budget_strip?"true":"false",fh_replay_texture_vram,fh_replay_native_worker,fh_minimal_worker_executed(),FH_REPLAY_TAG,selected,cursor.status,cursor.step,record_count,(unsigned)(FH_REPLAY_ROWS*sizeof(*rows)),plan.save_crc,plan.save_bytes,cursor.max_lateness,ok?"true":"false");fclose(f);}
    free(rows);rows=NULL;
}
#endif
